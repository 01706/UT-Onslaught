#include "UTOnslaught.h"
#include "UTOnslaughtGameMode.h"
#include "UTOnslaughtGameState.h"
#include "UTOnslaughtHUD.h"
#include "UTCharacterOnslaught.h"
#include "UTPlayerController.h"
#include "UTOnslaughtSquadAI.h"
#include "UTOnslaughtPlayerController.h"
#include "UTPlayerStart.h"
#include "UTWeaponLocker.h"
#include "UTOnslaughtFlag.h"
#include "UTOnslaughtEnhancementInterface.h"

DEFINE_LOG_CATEGORY(LogUTOnslaught);

AUTOnslaughtGameMode::AUTOnslaughtGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UTOnsluaght GameMode defaults
	bUseTeamStarts = true;
	TimeLimit = 15;
	bGameHasImpactHammer = false;
	bPlayersStartWithArmor = false;
	HUDClass = AUTOnslaughtHUD::StaticClass();
	GameStateClass = AUTOnslaughtGameState::StaticClass();
	SquadType = AUTOnslaughtSquadAI::StaticClass();
	PlayerControllerClass = AUTOnslaughtPlayerController::StaticClass();
	CustomLinkSetupIni= FPaths::GeneratedConfigDir() + TEXT("OnslaughtCustomLinkSetup.ini");

	PlayerPawnObject.Reset();
	PlayerPawnObject = FStringAssetReference(TEXT("Class'/UTOnslaught/Character/DefaultCharacter_Onslaught.DefaultCharacter_Onslaught_C'"));

	MapPrefix = TEXT("WAR");
	DisplayName = NSLOCTEXT("UTGameMode", "WAR", "Onslaught");

}

void AUTOnslaughtGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_LOG(LogUTOnslaught, Warning, TEXT("Starting UTOnslaught"));
		
	// Find all UTOnslaughtNodeObjective classes within the level
	for (TActorIterator<AUTOnslaughtNodeObjective> Itr(GetWorld()); Itr; ++Itr)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Found: %s"), *Itr->GetName());

		// Check that the found object is a parent or subclass of UTOnslaughtNodeObjective
		if (Itr->IsA(AUTOnslaughtNodeObjective::StaticClass()))
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Added to power nodes"));
			// Add the found node to the PowerNodes array
			PowerNodes.Add(*Itr);
		}
		else
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Nope"));
			// The found object was not parent or subclass of UTOnslaughtObjective, skip
			continue;
		}
	}

	if (PowerNodes.Num() == 0)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Map doesn't have any PowerNodes!"));
		return;
	}

	UE_LOG(LogUTOnslaught, Warning, TEXT("Map has %d PowerNodes!"), PowerNodes.Num())
	
	// Get the level blueprint, should be a parent of AUTOnslaughtLevelScriptActor
	OnslaughtLvlBP = Cast<AUTOnslaughtLevelScriptActor>(GetWorld()->GetLevelScriptActor());

	// Do we have the level blueprint
	if (OnslaughtLvlBP)
	{
		// Is there any link setups?
		if (OnslaughtLvlBP->LinkSetups.Num() >= 1)
		{		
			// TODO: Made a boolean within the level blueprint, when true, SaveLinkSetupToConfig will be called
			//SaveLinkSetupToConfig();

			// Get ?LinkSetup= from URL options
			FString LinkSetupOption = UGameplayStatics::ParseOption(Options, TEXT("LinkSetup"));

			if (LinkSetupOption.IsEmpty())
			{
				//  No setup name given, try and use the default
				LinkSetupOption = FString(TEXT("Default"));
			}
			
			// Applies this setup
			ApplyLinkSetup(LinkSetupOption);

		}
		else
		{
			// No setup found
			UE_LOG(LogUTOnslaught, Warning, TEXT("No link setups within this map"));
		}
	}
	else
	{
		// Level blueprint not found
		UE_LOG(LogUTOnslaught, Warning, TEXT("Level blueprint NOT found, is the level blueprint a parent of AUTOnslaughtLevelScriptActor?"));
	}

	SetPowerCores();

	FindCloseActors();

	Super::InitGame(MapName, Options, ErrorMessage);
}

/*
[MapName]
LinkSetup=(
	SetupName= ,
	NodeLinks=(
		(FromNode= , ToNode=),
	),
	StandaloneNodes = (
	),
	NodeStartingOwners = (
	),
)
*/
void AUTOnslaughtGameMode::SaveLinkSetupToConfig()
{
	FString MapSelection = GetWorld()->GetMapName();
	// Removes UEDPIE_X_ prefix when in the editor
#if UE_EDITOR
	MapSelection = GetWorld()->RemovePIEPrefix(MapSelection);
#endif

	if (OnslaughtLvlBP && OnslaughtLvlBP->LinkSetups.Num() > 0)
	{
		// Create a empty selection within config file ( [MapName] )
		GConfig->EmptySection(*MapSelection, *CustomLinkSetupIni);

		TArray<FString> Setups;
		for (int32 i = 0; i < OnslaughtLvlBP->LinkSetups.Num(); i++)
		{
			const FString SetupString = PrepareLinkSetupStringForIni( OnslaughtLvlBP->LinkSetups[i].ToString() );
			
			Setups.Add(*SetupString);
		}

		GConfig->SetArray(*MapSelection, TEXT("LinkSetups"), Setups, CustomLinkSetupIni);
	}
}

FString AUTOnslaughtGameMode::PrepareLinkSetupStringForIni(const FString& LinkSetupString)
{
	return LinkSetupString.Replace(TEXT("{"), TEXT("(")).Replace(TEXT("}"), TEXT(")")).Replace(LINE_TERMINATOR, TEXT("\\") LINE_TERMINATOR);
}

FString AUTOnslaughtGameMode::GetLinkSetupStringFormIni(const FString& LinkSetupString)
{
	return LinkSetupString.Replace(TEXT("("), TEXT("{")).Replace(TEXT(")"), TEXT("}")).Replace(TEXT("\\") LINE_TERMINATOR, LINE_TERMINATOR);
}

void AUTOnslaughtGameMode::ApplyLinkSetup(FString SetupName)
{

	// First remove any previous link setups
	RemoveLinkSetup();

	UE_LOG(LogUTOnslaught, Warning, TEXT("Searching for setup %s"), *SetupName);

	// Try and find the given link setup by name
	int32 i = OnslaughtLvlBP->LinkSetups.IndexOfByPredicate([&](const FLinkSetup InItem)
	{
		return InItem.SetupName == SetupName;
	});
	
	// Link setup not found
	if (i == INDEX_NONE)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Unable to find link setup %s within the level blueprint"), *SetupName);
		FLinkSetup FoundSetup;

		UE_LOG(LogUTOnslaught, Warning, TEXT("Starting search for %s within custom setup ini"), *SetupName);
		if (LoadLinkSetupFromConfig(SetupName, FoundSetup))
		{
			// Add the new setup to the level blueprint, set i to this setup
			i = OnslaughtLvlBP->LinkSetups.Add(FoundSetup);
		}
		else
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Unable to find link setup within custom setup ini"));
			ApplyLinkSetup(TEXT("Default"));
		}
	}
	
	if (i != INDEX_NONE)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Activating link setup: %s (%d)"), *SetupName, i);

		// Find all the nodes and kill all their links and starting owners
		UE_LOG(LogUTOnslaught, Warning, TEXT("Clearing %d power nodes of their linked nodes"), PowerNodes.Num());

		for (int32 j = 0; j < PowerNodes.Num(); j++)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Loop %d"), j)
			// Only clear the linked nodes if there is anything in there, otherwise it crashes :(
			if (PowerNodes[j]->LinkedNodes.Num() > 0)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Clearing linked %d"), j)
				PowerNodes[j]->LinkedNodes.Empty();
			}

			//UE_LOG(LogUTOnslaught, Warning, TEXT("Clearing Starting Owner Core"))
			PowerNodes[j]->StartingOwnerCore = nullptr;
		}

		// --  Apply Links
		UE_LOG(LogUTOnslaught, Warning, TEXT("Start appying links"));
	
		// Create a temp variable to the hold the setup 
		FLinkSetup Setup = OnslaughtLvlBP->LinkSetups[i];
		
		// Loop all node links in the setup
		for (int32 j = 0; j < Setup.NodeLinks.Num(); j++)
		{			
			//UE_LOG(LogUTOnslaught, Warning, TEXT("link %d"), j)
			// Check to see if both FromNode and ToNode are set
			if (Setup.NodeLinks[j].FromNode != NULL && Setup.NodeLinks[j].ToNode != NULL)
			{
				//TODO: Perform a check to see if the To and From node reference exsists in the map, they may of been deleted and the link setup not updated
				//TODO: Does this need to be done now? When node is deleted from level the editor cleans up references already and we are already checking existence of nodes with loading from file

				// If the number of links from this node is less then the maxiumum alowed
				if (Setup.NodeLinks[j].FromNode->LinkedNodes.Num() < AUTOnslaughtNodeObjective::MaxNumLinks)
				{
					Setup.NodeLinks[j].FromNode->LinkedNodes.Add(Setup.NodeLinks[j].ToNode);
				}
				else
				{
					UE_LOG(LogUTOnslaught, Warning, TEXT("To many links specifed for %s"), *Setup.NodeLinks[j].FromNode->GetName());
				}			
			}
		} // For Setup.NodeLinks

		// Apply standalone nodes
		UE_LOG(LogUTOnslaught, Warning, TEXT("Applying standalone nodes"));
		for (int32 k = 0; k < PowerNodes.Num(); k++)
		{
			// Try and find this node within the list of stand alone nodes
			PowerNodes[k]->bStandalone = (Setup.StandaloneNodes.Find(PowerNodes[k]) != INDEX_NONE);
		}

		// Apply starting owners
		UE_LOG(LogUTOnslaught, Warning, TEXT("Applying starting owners"));
		for (int32 x = 0; x < Setup.NodeStartingOwners.Num(); x++)
		{
			// Check this node existest
			if (Setup.NodeStartingOwners[x].Node != nullptr)
			{
				// Set this nodes starting owner core to the one specified
				Setup.NodeStartingOwners[x].Node->StartingOwnerCore = Setup.NodeStartingOwners[x].StartingOwnerCore;
				
				// Sets the team as the same as the starting owner core
				Setup.NodeStartingOwners[x].Node->TeamNum = Setup.NodeStartingOwners[x].StartingOwnerCore->TeamNum;
				
				// This node is no longer neutral
				Setup.NodeStartingOwners[x].Node->bIsNeutral = false;

				// Check if there is any linked nodes
				if (Setup.NodeStartingOwners[x].Node->LinkedNodes.Num() == 0)
				{
					// No linked nodes, this node is a stand alone
					Setup.NodeStartingOwners[x].Node->bStandalone = true;
				}
			}
		}

		// Apply min player count

		// Deactive actors

		// Active actors

		// Set ActiveSetupName to this setup
		ActiveSetupName = Setup.SetupName;

	}
}

bool AUTOnslaughtGameMode::LoadLinkSetupFromConfig(const FString SetupName, FLinkSetup& OutSetup)
{

	FString MapSelection = GetWorld()->GetMapName();
	// Removes UEDPIE_X_ prefix when in the editor
#if UE_EDITOR
	MapSelection = GetWorld()->RemovePIEPrefix(MapSelection);
#endif

	TArray<FString> LinkSetupsIni;
	GConfig->GetArray(*MapSelection, TEXT("LinkSetups"), LinkSetupsIni, CustomLinkSetupIni);

	if (LinkSetupsIni.Num() > 0)
	{
		// Loop all the setups found within the custom link setup file
		for (int32 i = 0; i < LinkSetupsIni.Num(); i++)
		{
			FLinkSetup LinkSetup;

			// Try and get a link setup from string, does that link setup name match the one we are looking for
			if (NewLinkSetupFromString(LinkSetupsIni[i], LinkSetup) && LinkSetup.SetupName == SetupName)
			{
				OutSetup =  LinkSetup;
				return true;
			}
		}

		UE_LOG(LogUTOnslaught, Warning, TEXT("Unable to find setup %s for map %s within %s"), *SetupName, *MapSelection, *CustomLinkSetupIni);
		return false;
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Unable to load any custom link setup's for %s within %s"), *MapSelection, *CustomLinkSetupIni);
		return false;
	}
}

bool AUTOnslaughtGameMode::NewLinkSetupFromString(const FString SetupString, FLinkSetup& OutSetup)
{

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create( GetLinkSetupStringFormIni(SetupString) );
	FLinkSetup ReturnSetup;

	// Attempt to decode the JSON string into a usable object
	FJsonSerializer::Deserialize(JsonReader, JsonObject);

	if (JsonObject.IsValid())
	{
		ReturnSetup.SetupName = JsonObject->GetStringField(TEXT("SetupName"));

		TArray< TSharedPtr<FJsonValue> > NodeLinks = JsonObject->GetArrayField(TEXT("NodeLinks"));
		for (int32 i = 0; i < NodeLinks.Num(); i++)
		{
			TSharedPtr<FJsonObject> RequestItem = NodeLinks[i]->AsObject();

			FFullNodes* ReturnNodeLinks = new FFullNodes();

			// Loop all AUTOnslaughtNodeObjective actors in the world to check if Actor name in the config is in this world
			for (TActorIterator<AUTOnslaughtNodeObjective> Itr(GetWorld()); Itr; ++Itr)
			{
				if (Itr->GetName() == RequestItem->GetStringField(TEXT("FromNode")))
				{
					ReturnNodeLinks->FromNode = *Itr;
				}

				if (Itr->GetName() == RequestItem->GetStringField(TEXT("ToNode")))
				{
					ReturnNodeLinks->ToNode = *Itr;
				}
			}

			// If both FromNode and ToNode named actors have been found, add this link to the returned setup
			if (ReturnNodeLinks->FromNode != nullptr && ReturnNodeLinks->ToNode != nullptr)
			{
				ReturnSetup.NodeLinks.Add(*ReturnNodeLinks);
			}
			else
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Failed to find a actor name within the world - NodeLinks"));
				return false;
			}
		} // For NodeLinks


		// Standalown
		TArray< TSharedPtr<FJsonValue> > Standalown = JsonObject->GetArrayField(TEXT("Standalown"));
		for (int32 i = 0; i < Standalown.Num(); i++)
		{
			FString RequestItem = Standalown[i]->AsString();

			// Loop all AUTOnslaughtNodeObjective actors in the world to check if Actor name in the config is in this world
			for (TActorIterator<AUTOnslaughtNodeObjective> Itr(GetWorld()); Itr; ++Itr)
			{				 
				if (Itr->GetName() == RequestItem)
				{
					ReturnSetup.StandaloneNodes.Add(*Itr);
				}
			}
		}// For Standalown

		//  NodeStartingOwners
		TArray< TSharedPtr<FJsonValue> > NodeStartingOwners = JsonObject->GetArrayField(TEXT("NodeStartingOwners"));
		for (int32 i = 0; i < NodeStartingOwners.Num(); i++)
		{
			TSharedPtr<FJsonObject> RequestItem = NodeLinks[i]->AsObject();

			FNodeStartingOwner* ReturnNodeStartingOwners = new FNodeStartingOwner();

			// Loop all AUTOnslaughtNodeObjective actors in the world to check if Actor name in the config is in this world
			for (TActorIterator<AUTOnslaughtNodeObjective> Itr(GetWorld()); Itr; ++Itr)
			{
				if (Itr->GetName() == RequestItem->GetStringField(TEXT("StartingOwnerCore")))
				{
					AUTOnslaughtPowerCore* StartingCore = Cast<AUTOnslaughtPowerCore>(*Itr);

					if (StartingCore)
						ReturnNodeStartingOwners->StartingOwnerCore = StartingCore;
				}

				if (Itr->GetName() == RequestItem->GetStringField(TEXT("Node")))
				{
					ReturnNodeStartingOwners->Node = *Itr;
				}
			}

			// If both FromNode and ToNode named actors have been found, add this link to the returned setup
			if (ReturnNodeStartingOwners->StartingOwnerCore != nullptr && ReturnNodeStartingOwners->Node != nullptr)
			{
				ReturnSetup.NodeStartingOwners.Add(*ReturnNodeStartingOwners);
			}
			else
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Failed to find a actor name within the world - NodeStartingOwners"));
				return false;
			}

		} // For NodeStartingOwners
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Failed to deserialize setup string into json object"));
		return false;
	}


	// Return the created link setup
	OutSetup = ReturnSetup;
	return true;
}

void AUTOnslaughtGameMode::RemoveLinkSetup()
{
}

void AUTOnslaughtGameMode::SetPowerCores()
{
	// Clear power core array
	PowerCore.Empty();

	for (int32 i = 0; i < PowerNodes.Num(); i++)
	{
		// is this node a power core?
		AUTOnslaughtPowerCore* PC = Cast<AUTOnslaughtPowerCore>(PowerNodes[i]);
		if (PC != NULL)
		{
			if (PowerCore.Num() < 2)
			{
				// If there is less then two power cores that we know of add this power core to the list
				PowerCore.Add(PC);
			}
			else
			{
				// Too many cores
				UE_LOG(LogUTOnslaught, Warning, TEXT("To many power cores, %s being ignored"), *PC->GetName());
			}
		}

		// Clear core distance - will be reinitialized in the PowerCores' InitializeForThisRound
		PowerNodes[i]->FinalCoreDistance[0] = 255;
		PowerNodes[i]->FinalCoreDistance[1] = 255;

		// initialize links - make sure they are all two way
		//UE_LOG(LogUTOnslaught, Warning, TEXT("GM: Calling InitLinks(NodeObjective) on %s"), *PowerNodes[i]->GetName())
		PowerNodes[i]->InitLinks();

	}
	
	// Sort the found power cores by team number smallest to largest to corresponds to array index
	PowerCore.Sort([&](const AUTOnslaughtPowerCore& A, const AUTOnslaughtPowerCore& B) {return B.TeamNum > A.TeamNum; });

	UE_LOG(LogUTOnslaught, Warning, TEXT("Initialize PowerCore for team 0 (Red) %s"), *PowerCore[0]->GetName());
	PowerCore[0]->Reset();
	PowerCore[0]->InitializeForThisRound(0);
	
	UE_LOG(LogUTOnslaught, Warning, TEXT("Initialize PowerCore for team 1 (Blue) %s"), *PowerCore[1]->GetName());
	PowerCore[1]->Reset();
	PowerCore[1]->InitializeForThisRound(1);
	
}

void AUTOnslaughtGameMode::FindCloseActors()
{
	//UE_LOG(LogUTOnslaught, Warning, TEXT("Find close actors"));

	// Remove any references to close actors
	for (int32 i = 0; i < PowerNodes.Num(); i++)
	{	
		PowerNodes[i]->InitCloseActors();
	}

	// Find player starts 
	for (TActorIterator<AUTPlayerStart> Itr(GetWorld()); Itr; ++Itr)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Player start found: %s"), *Itr->GetName());
		AUTOnslaughtObjective* ClosestObjective = ClosestObjectiveTo(*Itr);

		// Was a objective found found near this player start
		if (ClosestObjective != nullptr)
		{
			// Add player start to objective
			ClosestObjective->PlayerStarts.Add(*Itr);

			// Set teleport destination if objective is a node
			AUTOnslaughtNodeObjective* Node = Cast<AUTOnslaughtNodeObjective>(ClosestObjective);
			if (Node)
				Node->bIsTeleportDestination = true;

			//UE_LOG(LogUTOnslaught, Warning, TEXT("Player start found: Assigning player start %s to node %s"), *Itr->GetName(), *Node->GetName());
		}
	} // Player starts

	// Find weapon lockers
	for (TActorIterator<AUTWeaponLocker> Itr(GetWorld()); Itr; ++Itr)
	{

	}

	// Find vehicle factory

	// Find teleporters

	// Find node enhancements (actors with IUTOnslaughtEnhancementInterface interface)
	TArray<AActor*> EnhancedActors;

	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UUTOnslaughtEnhancementInterface::StaticClass(), EnhancedActors);

	for (int32 i = 0; i < EnhancedActors.Num(); i++)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Node enhancement found: %s"), *EnhancedActors[i]->GetName());

		AUTOnslaughtNodeObjective* ClosestNode = ClosestNodeTo(EnhancedActors[i]);

		if (EnhancedActors[i]->GetClass()->ImplementsInterface(UUTOnslaughtEnhancementInterface::StaticClass())) // Note I'm using the UInterface here, not the IInterface!
		{
			IUTOnslaughtEnhancementInterface* SomeObjectInterface = Cast<IUTOnslaughtEnhancementInterface>(EnhancedActors[i]);

			if (SomeObjectInterface->Execute_GetControllingNode(EnhancedActors[i]) == nullptr && ClosestNode != nullptr)
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("GM: Calling SetControllingNode"));
				SomeObjectInterface->Execute_SetControllingNode(EnhancedActors[i], ClosestNode);
			}
		}
	}
}

AUTOnslaughtObjective* AUTOnslaughtGameMode::ClosestObjectiveTo(AActor* A)
{
	AUTOnslaughtObjective* BestObjective;
	BestObjective = nullptr;

	float BestRating = FLT_MAX;

	for (int32 i = 0; i < PowerNodes.Num(); i++)
	{
		// Can this objective have associated player starts
		if (PowerNodes[i]->bAssociatePlayerStarts)
		{
			// Find the distance between this objective and the player start found
			float NewRating = (PowerNodes[i]->GetActorLocation() - A->GetActorLocation()).SizeSquared();

			// Is this player start closer to this objective then the previous known closest objective
			if (NewRating < BestRating)
			{
				// Set this objective as the curent closest
				BestObjective = PowerNodes[i];
				BestRating = NewRating;
			}
		}
	}

	return BestObjective;
}

AUTOnslaughtNodeObjective* AUTOnslaughtGameMode::ClosestNodeTo(AActor* A)
{
	AUTOnslaughtNodeObjective* BestNode;
	BestNode = nullptr;

	float BestRating = FLT_MAX;

	for (int32 i = 0; i < PowerNodes.Num(); i++)
	{
		float NewRating = (PowerNodes[i]->GetActorLocation() - A->GetActorLocation()).SizeSquared();

		if (NewRating < BestRating)
		{
			BestRating = NewRating;
			BestNode = PowerNodes[i];
		}
	}

	return BestNode;
}

bool AUTOnslaughtGameMode::HandleDistroyFlag(AUTPlayerController * C)
{
	if (C == nullptr)
	{
		return false;
	}
	
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(C->GetPawn());

	UCapsuleComponent* CharacterCapsule = UTCharacter->GetCapsuleComponent();

	if (UTCharacter && CharacterCapsule)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("DistryFlag: found char and capsule"));

		// Find actors which are overlapping this players capsule
		TArray<UPrimitiveComponent*> OverlappingComponents;
		CharacterCapsule->GetOverlappingComponents(OverlappingComponents);

		//UE_LOG(LogUTOnslaught, Warning, TEXT("DistryFlag: found %d overlaps"), OverlappingComponents.Num());

		// Loop all found actors
		for (auto CompIt = OverlappingComponents.CreateIterator(); CompIt; ++CompIt)
		{

			UPrimitiveComponent* OtherComponent = *CompIt;
			if (OtherComponent && OtherComponent->bGenerateOverlapEvents)
			{
				AUTOnslaughtFlag* Flag = Cast<AUTOnslaughtFlag>(OtherComponent->GetOwner());

				if (Flag && Flag->ObjectState == CarriedObjectState::Dropped && Flag->GetTeamNum() != UTCharacter->GetTeamNum())
				{
					UE_LOG(LogUTOnslaught, Warning, TEXT("DistryFlag: You are a HERO!"));
					// be a hero, suicide and send flag home
					// TODO: Add to hero score!
					UTCharacter->PlayerSuicide();
					Flag->SendHome();
					return true;
				}
			}
		}
	}

	return false;
}

void AUTOnslaughtGameMode::CheckGameTime()
{

	Super::CheckGameTime();
	AUTOnslaughtGameState* OnslaughtGameState = GetWorld()->GetGameState<AUTOnslaughtGameState>();

	if (OnslaughtGameState)
	{
		if (OnslaughtGameState->IsMatchInProgress() && (TimeLimit != 0))
		{
			// Check Red core health
			if (PowerCore[0]->Health <= 0.0f)
			{
				// Red Team Won
				UE_LOG(LogUTOnslaught, Warning, TEXT("CheckGameTime: Blue Won"));
				AUTPlayerState* Winner = Cast<AUTPlayerState>(PowerCore[0]->LastAttacker);
				EndGame(Winner, FName(TEXT("KilledCore")));
			}
			// Check Blue core health
			else if (PowerCore[1]->Health <= 0.0f)
			{
				// Blue Team Won
				UE_LOG(LogUTOnslaught, Warning, TEXT("CheckGameTime: Red Won"));
				AUTPlayerState* Winner = Cast<AUTPlayerState>(PowerCore[1]->LastAttacker);
				EndGame(Winner, FName(TEXT("KilledCore")));
			}
		}
	}
}

void AUTOnslaughtGameMode::UpdateLinks()
{
	for (int32 i = 0; i < PowerNodes.Num(); i++)
	{
		AUTOnslaughtPowerNode* Node = Cast<AUTOnslaughtPowerNode>(PowerNodes[i]);
		if (Node != NULL && 
			!Node->bStandalone && 
			Node->GetTeamNum() != 255 && 
			!Node->bIsSevered && 
			Node->CurrentNodeState != NodeState::Disabled && 
			Node->bInitialState)
		{
			if (!Node->PoweredBy(Node->GetTeamNum()))
			{
				Node->SetSevered();
			}
		}
	}
}

void AUTOnslaughtGameMode::CheckSevering(AUTOnslaughtNodeObjective* CheckNode, uint8 Team)
{
}

AActor* AUTOnslaughtGameMode::ChoosePlayerStart_Implementation(AController* Player)
{

	AUTPlayerState* UTPS = Cast<AUTPlayerState>((Player != NULL) ? Player->PlayerState : NULL);
	AUTOnslaughtObjective* SelectedNode;

	bool bEnemyCanAttackCore;
	bool bPrioritizeNodeWithFlag;
	float BestRating = 255.f;
	float NewRating = 0.0f;
	uint32 EnemyTeam; 
	//bool bSpawnNodeSelected;

	SelectedNode = nullptr;

	if (UTPS != NULL && UTPS->GetTeamNum() != 255)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Starting. Looking for player start for '%s' whoes on team '%d'"), *Player->GetName(), UTPS->GetTeamNum());

		// Check if player has selected a node to respawn at
			// SelectedNode = RespawnChoiceA
		//bSpawnNodeSelected = (SelectedNode != NULL);

		// Determin the Enemy team number
		EnemyTeam = 1 - UTPS->GetTeamNum();

		// Player has not already selected a spawn location, try and find one
		if (SelectedNode == nullptr)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Selected node is null PASS"));

			// Can the enemy attack the spawning players core
			bEnemyCanAttackCore = PowerCore[UTPS->GetTeamNum()]->PoweredBy(EnemyTeam);
			// Should we priotitize a objective with a flag 
			bPrioritizeNodeWithFlag = ShouldPrioritizeNodeWithFlag(UTPS->GetTeamNum(), EnemyTeam, bEnemyCanAttackCore, Player);

			// The Enemy can attack players core, player should spawn as core to defend
			if (bEnemyCanAttackCore && !bPrioritizeNodeWithFlag)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Players core can be attacted, spawn at core and defend!"));
				SelectedNode = PowerCore[UTPS->GetTeamNum()];
			}
			else
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Core not under attact, find a node to spawn at"));

				for (int32 i = 0; i < PowerNodes.Num(); i++)
				{
					// Is this a valid spawn location for players team
					if (PowerNodes[i]->IsValidSpawnFor(UTPS->GetTeamNum()))
					{
						//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: PowerNode '%s' (team '%d') has valid player starts for players team '%d'"), *PowerNodes[i]->GetName(), PowerNodes[i]->GetTeamNum(), UTPS->GetTeamNum());
						
						// Go to a location were there is a flag
						if (bPrioritizeNodeWithFlag)
						{
							//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Prioritzing flag"));
						}

						// Core cannot be attacked, find node cloest to the enemy teams core
						if (!bEnemyCanAttackCore)
						{
							//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Node rating check"));

							// Rating based first on link distance, then available vehicles
							NewRating = PowerNodes[i]->GetSpawnRating(EnemyTeam);
							
							if (NewRating < BestRating)
							{
								BestRating = NewRating;
								SelectedNode = PowerNodes[i];
							}
							else if (NewRating == BestRating) // If we have two nodes at equal link distance, we check geometric distance
							{
								float CoreDistA = (PowerCore[EnemyTeam]->GetActorLocation() - PowerNodes[i]->GetActorLocation()).SizeSquared();
								float CoreDistB = (PowerCore[EnemyTeam]->GetActorLocation() - SelectedNode->GetActorLocation()).SizeSquared();

								if (CoreDistA < CoreDistB)
								{
									SelectedNode = PowerNodes[i];
								}
							}
						}
					}// IsValidSpawnFor
				} // For

				// No valid nodes found, set to team core
				if (SelectedNode == nullptr)
				{
					//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: After checking all nodes selected node is still null, try the core"));
					SelectedNode = PowerCore[UTPS->GetTeamNum()];
				}
			}
		} // SelectedNode == nullptr

		// Get a player start at the found/selected node 
		AActor*  BestPlayerStart = BestPlayerStartAtNode(SelectedNode, Player);

		
		if ((BestPlayerStart == nullptr) && SelectedNode != PowerCore[UTPS->GetTeamNum()])
		{
			// Could not find a player start at the given node, try the team core
			BestPlayerStart = BestPlayerStartAtNode(PowerCore[UTPS->GetTeamNum()], Player);
		}

		if (BestPlayerStart != nullptr)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Best player start for '%s' is %s team '%d'"), *Player->GetName(), *BestPlayerStart->GetName(), UTPS->GetTeamNum());

			return BestPlayerStart;
		}
	}

	//UE_LOG(LogUTOnslaught, Warning, TEXT("CPS: Reached the end, no team set, find a random player start at core 0"));
	return BestPlayerStartAtNode(PowerCore[0], Player);
}

bool AUTOnslaughtGameMode::ShouldPrioritizeNodeWithFlag(uint32 Team, uint32 EnemyTeam, bool bEnemyCanAttackCore, AController* Player)
{
	return false;
}

AActor* AUTOnslaughtGameMode::BestPlayerStartAtNode(AUTOnslaughtObjective* Node, AController* Player)
{
	// Mostly copied from AGameMode::ChoosePlayerStart_Implementation

	APlayerStart* FoundPlayerStart = NULL;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> UnOccupiedStartPoints;
	TArray<APlayerStart*> OccupiedStartPoints;

	for (int32 i = 0; i < Node->PlayerStarts.Num(); i++)
	{
		APlayerStart* PlayerStart = Node->PlayerStarts[i];

		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}
		else
		{
			FVector ActorLocation = PlayerStart->GetActorLocation();
			const FRotator ActorRotation = PlayerStart->GetActorRotation();
			if (!GetWorld()->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{
				UnOccupiedStartPoints.Add(PlayerStart);
			}
			else if (GetWorld()->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				OccupiedStartPoints.Add(PlayerStart);
			}
		}
	}

	if (FoundPlayerStart == NULL)
	{
		if (UnOccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}

	return FoundPlayerStart;
}