#include "UTOnslaught.h"

#include "UTOnslaughtFlag.h"
#include "UTOnslaughtPowerNode.h"
#include "UTOnslaughtGameState.h"
#include "UTOnslaughtGameMessage.h"

AUTOnslaughtPowerNode::AUTOnslaughtPowerNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DamageCapacity = 150.0f;
	ConstructionTime = 10;
	SeveredDamagePerSecound = 1.f;
	OrbCaptureInvulnerabilityDuration = 10.f;
	OrbLockScoringInterval = 5.f;
	OrbHealingPerSecond = 1.f;
	InvulnerableRadius = 600.f;

	LastUnlinkedWarningTime = 0.f;

	Vulnerability = EVulnerability::Vulnerable;

	bInitialState = false;

	// Create the basic actor compenents

	Collision = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("Capsule"));
	// Only player pawn, vehical and GameVolume will trigger a overlap event, everything else is ignored
	Collision->SetCollisionProfileName(FName(TEXT("Pickup")));
	Collision->InitCapsuleSize(150.0f, 130.0f);
	// When over lap begins call OnOverlapBegin
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AUTOnslaughtPowerNode::OnOverlapBegin);
	Collision->SetupAttachment(RootComponent);

	NodeBase = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("NodeBase"));
	NodeBase->SetupAttachment(RootComponent);

	NodeTop = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("NodeTop"));
	NodeTop->SetupAttachment(RootComponent);

	NodeBeam = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("NodeBeam"));
	// Disaable collision, overlap events and this should not effect navigation or cast shadows
	NodeBeam->SetCollisionProfileName(FName(TEXT("NoCollision")));
	NodeBeam->bGenerateOverlapEvents = false;
	NodeBeam->SetCanEverAffectNavigation(false);
	NodeBeam->SetCastShadow(false);
	NodeBeam->SetupAttachment(RootComponent);

	PrimeNodeName = NSLOCTEXT("UTOnslaught", "PrimeNodeName", "Prime Node");
	EnemyPrimeNodeName = NSLOCTEXT("UTOnslaught", "EnemyPrimeNodeName", "Enemy Prime Node");

}

void AUTOnslaughtPowerNode::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AUTOnslaughtPowerNode, Vulnerability);
	DOREPLIFETIME(AUTOnslaughtPowerNode, ControllingFlag);
}

void AUTOnslaughtPowerNode::BeginPlay()
{
	Super::BeginPlay();
	// FIXME: Sometimes crashes game
	//NodeBeamMaterial = NodeBeam->GetMaterial(0);

	if (NodeBeamMaterial == NULL)
		UE_LOG(LogUTOnslaught, Warning, TEXT("Nodebeam material is null"));

	// Create a dynamic material instence from NodeBeamMaterial
	NodeBeamMaterialInstance = NodeBeamMaterial? UMaterialInstanceDynamic::Create(NodeBeamMaterial, this) : nullptr;

	NodeBeam->SetMaterial(0, NodeBeamMaterialInstance);
	
	// If we are the server set the inital node state
	if (Role == ROLE_Authority)
		SetInitialState();
}

void AUTOnslaughtPowerNode::SetInitialState()
{
	FString debugStateText = TEXT("NULL");
	
	if (!bStandalone && StartingOwnerCore == NULL && LinkedNodes.Num() == 0)
	{
		debugStateText = TEXT("Disabled");
		GotoState(NodeState::Disabled);
	} 
	else if (bIsNeutral)
	{
		debugStateText = TEXT("Neutral");
		GotoState(NodeState::Neutral);
	}

	if (!bStandalone && StartingOwnerCore != NULL)
	{
		debugStateText = TEXT("Active");
		GotoState(NodeState::Active);
	}
	else if (bStandalone)
	{
		debugStateText = TEXT("Standalone Neutral");
		GotoState(NodeState::Neutral);
	}

	UE_LOG(LogUTOnslaught, Warning, TEXT("Init state %s for %s"), *debugStateText, *GetName());

	if (CurrentNodeState == NAME_None)
		UE_LOG(LogUTOnslaught, Warning, TEXT("Could not find a initial state for this node %s"), *GetName());

	bInitialState = true;
}

void AUTOnslaughtPowerNode::GotoState(FName NewState)
{
	// debug
	/*
	if (Role == ROLE_Authority)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Server: Going from state '%s' to '%s'"), *CurrentNodeState.ToString(), *NewState.ToString());
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Client: Going from state '%s' to '%s'"), *CurrentNodeState.ToString(), *NewState.ToString());
	}
	*/
	// End of state
	if (Role == ROLE_Authority)
	{		
		if (CurrentNodeState == NodeState::Neutral || 
			CurrentNodeState == NodeState::UnderConstruction || 
			CurrentNodeState == NodeState::Active )
		{
			Vulnerability = EVulnerability::Vulnerable;
			GetWorldTimerManager().ClearTimer(VulnerabilityTimer);
		}

		if (CurrentNodeState == NodeState::Disabled)
		{
			// Re-enable take damage and generate overlap events
			bCanBeDamaged = true;
			Collision->bGenerateOverlapEvents = true;
		}
	}

	CurrentNodeState = NewState;

	// Begin state
	if (Role == ROLE_Authority)
	{
		
		if (NewState == NodeState::Neutral)
		{
			LastAttacker = nullptr;
			TeamNum = 255;
			Health = 0;
			Vulnerability = EVulnerability::Vulnerable;
			//Set timer for CheckInvulnerability
			GetWorldTimerManager().SetTimer(VulnerabilityTimer, this, &AUTOnslaughtPowerNode::CheckInvulnerability, 0.5f, true);
		}
		else if (NewState == NodeState::UnderConstruction)
		{
			// @see SetnodeStateUnderConstruction
		}
		else if (NewState == NodeState::Active)
		{
			LastAttacker = nullptr;
			Health = DamageCapacity;
			LastCaptureTime = GetWorld()->TimeSeconds;

			GetWorldTimerManager().SetTimer(VulnerabilityTimer, this, &AUTOnslaughtPowerNode::CheckInvulnerability, 0.5f, true);

			// Send node constructed message
			AUTTeamGameMode* GM = GetWorld()->GetAuthGameMode<AUTTeamGameMode>();
			if (GM && GetTeamNum() < 2)
			{
				GM->BroadcastLocalized(this, UUTOnslaughtGameMessage::StaticClass(), 2 + GetTeamNum(), NULL, NULL, NULL);
			}

			// Notify near by ...

			// Vehicle factories
			/*
			for (int32 i = 0; i < VehicleFactories.Num(); i++)
			{
				VehicleFactories[i]->Activate(DefenderTeamIndex); // 1-TeamNum?
			}
			*/

			// Weapon Lockers
			/*
			for (int32 i = 0; i < DeployableLockers.Num(); i++)
			{
				DeployableLockers[i]->Activate(DefenderTeamIndex); // 1-TeamNum?
			}
			*/

			// Teleporters
			/*
			for (int32 i = 0; i < NodeTeleporters.Num(); i++)
			{
				NodeTeleporters[i]->SetTeamNum(DefenderTeamIndex); // 1-TeamNum?
			}
			*/

			// Node Enhancements
			for (int32 i = 0; i < Enhancements.Num(); i++)
			{

				if (Enhancements[i]->GetClass()->ImplementsInterface(UUTOnslaughtEnhancementInterface::StaticClass())) // Note I'm using the UInterface here, not the IInterface!
				{
					IUTOnslaughtEnhancementInterface* SomeObjectInterface = Cast<IUTOnslaughtEnhancementInterface>(Enhancements[i]);
					
					SomeObjectInterface->Execute_ActivateEnhancement(Enhancements[i]);
				}

			}

			// Special Objectives
			/*
			for (int32 i = 0; i < ActivatedObjectives.Num(); i++)
			{
				ActivatedObjectives[i]->CheckActivate();
			}
			*/

		}
		else if (NewState == NodeState::Destroyed)
		{
			if (LastAttacker != nullptr)
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Node %s destroyed by %s"), *GetName(), *LastAttacker->GetName());
			}
			else
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Node %s destroyed"), *GetName());
			}
			TeamNum = 255;
			Health = 0;

			UpdateCloseActors();

			// De-activate node enhancements
			for (int32 i = 0; i < Enhancements.Num(); i++)
			{
				if (Enhancements[i]->GetClass()->ImplementsInterface(UUTOnslaughtEnhancementInterface::StaticClass())) // Note I'm using the UInterface here, not the IInterface!
				{
					IUTOnslaughtEnhancementInterface* SomeObjectInterface = Cast<IUTOnslaughtEnhancementInterface>(Enhancements[i]);

					SomeObjectInterface->Execute_DeactivateEnhancement(Enhancements[i]);
				}
			}

		}
		else if (NewState == NodeState::Disabled)
		{
			LastAttacker = nullptr;
			TeamNum = 255;
			Health = 0;

			// Dont take damage or generate overlap events
			bCanBeDamaged = false;
			Collision->bGenerateOverlapEvents = false;

			// Tell node teleporters they're disables
			/*
			for (int32 i = 0; i < NodeTeleporters.Num(); i++)
			{
				NodeTeleporters[i].TurnOff();
			}
			*/
		}

		UpdateLinks();
	}

	OnRep_NodeStateChanged();
}

void AUTOnslaughtPowerNode::OnRep_NodeStateChanged()
{
	if (Role == ROLE_Authority)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Server Rep '%s'"), *CurrentNodeState.ToString() );
	} 
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Client Rep '%s'"), *CurrentNodeState.ToString());
	}

	if (CurrentNodeState == NodeState::Neutral)
		OnNodeStateNeutral();

	if (CurrentNodeState == NodeState::Active)
		OnNodeStateActive();

	if (CurrentNodeState == NodeState::UnderConstruction)
		OnNodeStateUnderConstruction();
	
	if (CurrentNodeState == NodeState::Destroyed)
	{
		OnNodeStateDestroyed();
	}

	// Turn off the node beam effect on disabled nodes and remove from players HUD
	if (CurrentNodeState == NodeState::Disabled)
	{
		
		NodeBeam->SetVisibility(false);

		AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController());
		if ((GetNetMode() != NM_DedicatedServer) && UTPC->MyHUD)
		{
			UTPC->MyHUD->RemovePostRenderedActor(this);
		}
	}
	else 
	{
		// Anything but disabled, check that node beam is visable and this node is displayed on HUD
		if (!NodeBeam->IsVisible())
		{
			NodeBeam->SetVisibility(true);

			AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController());
			if ((GetNetMode() != NM_DedicatedServer) && UTPC->MyHUD)
			{
				UTPC->MyHUD->AddPostRenderedActor(this);
			}
		}
	}
}

void AUTOnslaughtPowerNode::ClientNodeChanged_Implementation(FName ServerNodeState, uint8 ServerTeamNum)
{
	// Only on clients
	if (Role < ROLE_Authority)
	{
		if (ServerTeamNum != TeamNum)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Client (%d) and server team (%d) number out of sync! resetting"), TeamNum, ServerTeamNum);
			TeamNum = ServerTeamNum;
		}
		if (ServerNodeState != CurrentNodeState)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Client (%s) and server (%s) node state out of sync! resetting"), *CurrentNodeState.ToString(), *ServerNodeState.ToString());
			CurrentNodeState = ServerNodeState;
		}

		//UE_LOG(LogUTOnslaught, Warning, TEXT("Client calling OnRep_NodeStateChanged"));
		OnRep_NodeStateChanged();
	}
}

float AUTOnslaughtPowerNode::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
		
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();

	if ((Role < ROLE_Authority) || (EventInstigator == NULL) || (Damage == 0.f) || (GS && (GS->HasMatchEnded())) )
	{
		return 0.0f;
	}
	else if (Damage < 0.0f)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("TakeDamage() called with damage %i of type %s... use HealDamage() to add health"), int32(Damage), *GetNameSafe(DamageEvent.DamageTypeClass));
		return 0.0f;
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Power Node Taking Damage"));

		AUTPlayerController* UTPC = Cast<AUTPlayerController>(EventInstigator);

		// Check if player is attacking own node, check if node is powered by players team
		if (UTPC && UTPC->GetTeamNum() != TeamNum && PoweredBy(UTPC->GetTeamNum()))
		{
			// Node cannt be attacted due to enemey orb near by
			if (Vulnerability == EVulnerability::InvulnerableByNearbyOrb)
			{
				if (ControllingFlag != nullptr && ControllingFlag->Holder != nullptr)
				{
					// Node is Shieled by nearby Orb!
					UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 42, UTPC->PlayerState, NULL, NULL);
				}
				else
				{
					// Node has Temorary Shielding
					UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 43, UTPC->PlayerState, NULL, NULL);
				}

				ShieldDamageCounter += Damage;
				if (ShieldDamageCounter > 200)
				{
					// Play cant attack sound
					ShieldDamageCounter -= 200;
				}
				return 0.0;
			}
			
			// No damage should be taken when node is neutral, disabled or destroyed
			if (CurrentNodeState == NodeState::Neutral || CurrentNodeState == NodeState::Disabled || CurrentNodeState == NodeState::Destroyed)
			{
				return 0.0f;
			}

			if (CurrentNodeState == NodeState::UnderConstruction || CurrentNodeState == NodeState::Active)
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Attcking openents linked node"));

				SetUnderAttack(true);
				if (GetNetMode() != NM_DedicatedServer)
				{
					BecomeUnderAttack();
				}

				// Take damage amount from health
				Health -= Damage;
				
				// Set last attacker
				LastAttacker = UTPC;
				// TODO: Score Damage

				// Health less then 0, distroy the node
				if (Health <= 0.0f)
					GotoState(NodeState::Destroyed);
					
				return Damage;
			}
		}
		else
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Attcking node which is not linked, owned by same team or is neutral"));
		}
	}
		
	//return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	return 1.0f;
}

float AUTOnslaughtPowerNode::HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (Damage > 0.0f)
	{
		if (CurrentNodeState == NodeState::Active)
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Healing node while active"));
			if ( (Health + Damage) >= DamageCapacity )
			{
				if (Role == ROLE_Authority)
					Health = DamageCapacity;

				return DamageCapacity;
			}
			else
			{
				if (Role == ROLE_Authority)
					Health += Damage;
				return Damage;
			}
		}

		if (CurrentNodeState == NodeState::UnderConstruction)
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Healing node while under construction"));
			if ((Health + Damage) >= DamageCapacity)
			{
				if (Role == ROLE_Authority)
					Health = DamageCapacity;
				
				GotoState(NodeState::Active);
				return DamageCapacity;
			}
			else
			{
				if (Role == ROLE_Authority)
					Health += Damage;

				return Damage;
			}
		}
	}

	return 0.0f;
}

void AUTOnslaughtPowerNode::DoOrbHealing()
{
	// Create a blank damage type
	TSubclassOf<UDamageType> const DamageTypeClass = TSubclassOf<UDamageType>(UDamageType::StaticClass());
	FDamageEvent DamageEvent(DamageTypeClass);

	AController* UTPC = Cast<AController>(ControllingFlag->Holder);

	HealDamage(OrbHealingPerSecond, DamageEvent, UTPC? UTPC : nullptr, ControllingFlag? ControllingFlag: nullptr);
}

bool AUTOnslaughtPowerNode::IsHealable(ACharacter* EventInstigator, AActor * DamageCauser)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();

	AUTCharacter* UTC = Cast<AUTCharacter>(EventInstigator);

	// Can only be healed when instigator and 'this' node are on the same team
	if (UTC && GS && GS->OnSameTeam(this, UTC))
	{
		// Also the node needs to be active or under construction
		if (CurrentNodeState == NodeState::Active || CurrentNodeState == NodeState::UnderConstruction)
		{
			return true;
		}
	}

	return false;
}

void AUTOnslaughtPowerNode::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	AUTCharacter* UTCharacter = Cast<AUTCharacter>(OtherActor);
	
	// Check that is was a UT character that overlaped us and we are the server (authority) and node is not disabled
	if (UTCharacter != nullptr && Role == ROLE_Authority && CurrentNodeState != NodeState::Disabled)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Power Node Touched:"));

		if (CurrentNodeState == NodeState::Neutral)
		{
			if (!PerformFlagCapture(UTCharacter))
			{
				if (PoweredBy(UTCharacter->GetTeamNum()))
				{
					UE_LOG(LogUTOnslaught, Warning, TEXT("Node is powered by team and player overlapping is not holding a flag"));
					SetNodeStateUnderConstruction(UTCharacter);
					return;
				} 
				else if ((GetWorld()->TimeSeconds - LastUnlinkedWarningTime) > 5.f && UTCharacter->GetCarriedObject() != nullptr)
				{
					LastUnlinkedWarningTime = GetWorld()->TimeSeconds;
					AUTPlayerController* UTPC = Cast<AUTPlayerController>(UTCharacter);
					if (UTPC)
						UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 6, UTPC->PlayerState, NULL, NULL);
				}
			}
		} 
		else if (CurrentNodeState == NodeState::UnderConstruction || CurrentNodeState == NodeState::Active)
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Node is '%s' checking for flag"), *CurrentNodeState.ToString() );
			
			PerformFlagCapture(UTCharacter);

			return;
		}
	}
}

bool AUTOnslaughtPowerNode::PerformFlagCapture(AUTCharacter* UTC)
{
	UE_LOG(LogUTOnslaught, Warning, TEXT("PerformFlagCapture called"))
	AUTOnslaughtFlag* CarriedFlag = Cast<AUTOnslaughtFlag>(UTC->GetCarriedObject());
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(UTC->Controller);

	if (!PoweredBy(UTC->GetTeamNum()) || CarriedFlag == nullptr)
	{
		if ( (GetWorld()->TimeSeconds - LastUnlinkedWarningTime > 5.f) && (CarriedFlag != nullptr) )
		{
			LastUnlinkedWarningTime = GetWorld()->TimeSeconds;
			
			UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 6, UTPC->PlayerState, NULL, NULL);
		}
		
		UE_LOG(LogUTOnslaught, Warning, TEXT("Node is not powered by same team or player not carrying a flag FlagCap 1"));
		return false;
	}

	// friendly team can only plant orb at constructing nodes, enemy team only at vulnerable nodes
	if (UTC->GetTeamNum() == GetTeamNum())
	{
		if (CurrentNodeState != NodeState::UnderConstruction)
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Node on same team, node not under construction"));
			return false;
		}
	}
	else
	{
		CheckInvulnerability();

		if (Vulnerability != EVulnerability::Vulnerable)
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Node is not vulnerable"));
			// Play shield sound effect on player

			if ( !PoweredBy(UTC->GetTeamNum()) )
			{
				UE_LOG(LogUTOnslaught, Warning, TEXT("Send not powered by node network to player"));
				if ((GetWorld()->TimeSeconds - LastUnlinkedWarningTime) > 5.f)
				{
					LastUnlinkedWarningTime = GetWorld()->TimeSeconds;

					UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 6, UTPC->PlayerState, NULL, NULL);
				}
			}
			else
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Send message based on flag to player"));
				if (ControllingFlag != nullptr && ControllingFlag->Holder != nullptr)
				{
					UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 42, UTPC->PlayerState, NULL, NULL);
				}
				else
				{
					UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 43, UTPC->PlayerState, NULL, NULL);
				}
			}
			return false;
		}
		else
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Increment node flag capture count"));
			// increment node orb capture stat 
			if (Role == ROLE_Authority)
			{
			}
			// Player capture sound on player

			// if CaptureCount == 10
			UTPC->ClientReceiveLocalizedMessage(UUTOnslaughtGameMessage::StaticClass(), 7, UTPC->PlayerState, NULL, NULL);
		}
	}

	UE_LOG(LogUTOnslaught, Warning, TEXT("Performing a flag capture of node, current state '%s'"), *CurrentNodeState.ToString());

	if (Role == ROLE_Authority)
	{
		ControllingFlag = CarriedFlag;
		FindNewHomeForFlag();
		Vulnerability = EVulnerability::InvulnerableToOrbCapture;
		Constructor = UTC;
		TeamNum = UTC->GetTeamNum();
		bCapturedByOrb = true;
		LastCaptureTime = GetWorld()->TimeSeconds;

		AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTC->PlayerState);
		CarriedFlag->Score(TEXT("NodeCapture"), UTC, UTPS); // InTurn calls SendHome when MoveToHome

	}

	UE_LOG(LogUTOnslaught, Warning, TEXT("Server: Capturned via flag setting state to active! %d"), TeamNum);
	GotoState(NodeState::Active);
	
	ClientNodeChanged(CurrentNodeState, TeamNum);

	return true;
}

bool AUTOnslaughtPowerNode::VerifyOrbLock(AUTOnslaughtFlag * CheckedFlag)
{
	// Start x units above the root
	FVector ViewPoint = GetActorLocation();
	ViewPoint.Z += 300.f;

	if (CheckedFlag != nullptr && ( (CheckedFlag->GetActorLocation() - ViewPoint).Size() < InvulnerableRadius)	)
	{
		
		FCollisionQueryParams CollisionParams(FName(TEXT("LineOfSightNode")), true, this);
/*		
		// Debug: Draw a link to actor
		DrawDebugLine(
			GetWorld(),
			ViewPoint,
			CheckedFlag->GetActorLocation(),
			FColor(255, 0, 0),
			false,
			2,
			0,
			10
		);
*/
		bool bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, CheckedFlag->GetActorLocation(), ECC_Visibility, CollisionParams);

		if (!bHit)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag in sight and within %f"), InvulnerableRadius);
			return true;
		}
	}

	//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag not in sight or within %f, flag is %f units way"), InvulnerableRadius, (CheckedFlag->GetActorLocation() - ViewPoint).Size());
	return false;
}

void AUTOnslaughtPowerNode::CheckInvulnerability()
{
	EVulnerability NewVulnerability;
	NewVulnerability = EVulnerability::Vulnerable;

	// Node is always vulnerable when neutral or destroyed
	if (CurrentNodeState == NodeState::Neutral || CurrentNodeState == NodeState::Destroyed)
	{
		if (Vulnerability != EVulnerability::Vulnerable)
		{
			Vulnerability = EVulnerability::Vulnerable;
		}
		return;
	}

	// Do we have a refence to the frendly flag? This can happen when the node was not captured via a flag, try and find this teams flag
	if (ControllingFlag == nullptr)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Control flag not found, trying to find it!"));
		AUTGameState* UTGS = Cast<AUTGameState>(GetWorld()->GetGameState());

		 // Find all the flags in the world and check if its on the same team as us
		for (TActorIterator<AUTOnslaughtFlag> Itr(GetWorld()); Itr; ++Itr)
		{
			if (UTGS && UTGS->OnSameTeam(this, *Itr))
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Control flag found"));
				ControllingFlag = *Itr;
				break;
			}
		}
	}

	if (ControllingFlag != nullptr)
	{
		if (ControllingFlag->Holder != nullptr && VerifyOrbLock(ControllingFlag))
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Flag being held and is near by"));

			NewVulnerability = EVulnerability::InvulnerableByNearbyOrb;

			// Can this node be attacted be the emeney team
			if (PoweredBy(1 - ControllingFlag->Team->TeamIndex))
			{			
				if (bIsPrimeNode)
				{
					// This is a prime node which can be attacked but is being shielded by orb, add to orb holders score 
					if (Vulnerability != NewVulnerability)
					{
						LastInvulnerabilityScoreTime = GetWorld()->TimeSeconds;
					}
					else if (GetWorld()->TimeSeconds - LastInvulnerabilityScoreTime > OrbLockScoringInterval)
					{
						// Add score to flag holder
						ControllingFlag->Holder->Score += 1.0f;
						LastInvulnerabilityScoreTime = GetWorld()->TimeSeconds;
					}
				}
			}
		}
		else if (GetWorld()->TimeSeconds - LastCaptureTime < OrbCaptureInvulnerabilityDuration)
		{
			NewVulnerability = EVulnerability::InvulnerableToOrbCapture;
		}
	}

	if (Vulnerability != NewVulnerability)
	{
		// Update the Vulnerability level and perform the onrep notify on the server, clients will call this when they receive the change (hopfully)
		Vulnerability = NewVulnerability;
		OnRep_Vulnerability();
	}

	if (CurrentNodeState == NodeState::Active)
	{
		if (Vulnerability == EVulnerability::InvulnerableByNearbyOrb && OrbHealingPerSecond > 0)
		{
			// If timere is not active start timer DoOrbHealing
			if (!GetWorldTimerManager().IsTimerActive(OrbHealingTimer))
			{
				GetWorldTimerManager().SetTimer(OrbHealingTimer, this, &AUTOnslaughtPowerNode::DoOrbHealing, 1.0f, true);
			}
		}
		else
		{
			if (GetWorldTimerManager().IsTimerActive(OrbHealingTimer))
			{
				GetWorldTimerManager().ClearTimer(OrbHealingTimer);
			}
		}
	}
	else
	{
		if (GetWorldTimerManager().IsTimerActive(OrbHealingTimer))
		{
			GetWorldTimerManager().ClearTimer(OrbHealingTimer);
		}
	}
}

void AUTOnslaughtPowerNode::SetNodeStateUnderConstruction(AUTCharacter* NewConstructor)
{	
	if (Role == ROLE_Authority)
	{
		UpdateLinks();
		
		// If we are not already under constuction set the team and go to construction state
		if (CurrentNodeState != NodeState::UnderConstruction)
		{	
			UE_LOG(LogUTOnslaught, Warning, TEXT("Server: Starting construction on node %s for team %d"), *GetName(), NewConstructor->GetTeamNum());
			
			TeamNum = NewConstructor->GetTeamNum();

			Constructor = NewConstructor;
					
			// Start Invulnerability timer
			GetWorldTimerManager().SetTimer(VulnerabilityTimer, this, &AUTOnslaughtPowerNode::CheckInvulnerability, 0.5f, true);

			GotoState(NodeState::UnderConstruction);
		}
		else
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Node already under construction, same team %s"), (Constructor->GetTeamNum() == TeamNum ? TEXT("Yes") : TEXT("No")));
		}
	}	
}

void AUTOnslaughtPowerNode::OnNodeStateUnderConstruction_Implementation()
{	
	// Start construction health generator, every 1 sec call ConstructionHealth
	ConstructionTimeElapsed = 0;
	GetWorldTimerManager().SetTimer(ConstructionTimer, this, &AUTOnslaughtPowerNode::ConstructionHealth, 1.0f, true);

	AUTGameState* UTGS = GetWorld()->GetGameState<AUTOnslaughtGameState>();
	NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), UTGS->Teams[TeamNum]->TeamColor);
}

void AUTOnslaughtPowerNode::ConstructionHealth()
{
	// Check that node is still under construction, if not clear the timer
	// This can happen when the node is destroyed when constructing
	if (CurrentNodeState != NodeState::UnderConstruction)
	{
		GetWorldTimerManager().ClearTimer(ConstructionTimer);
		return;
	}

	ConstructionTimeElapsed += 1.0;

	if (Role == ROLE_Authority)
	{
		float HealthGain = (1.0 / ConstructionTime) * DamageCapacity;
		Health += HealthGain;
	}

	AUTGameState* UTGS = GetWorld()->GetGameState<AUTOnslaughtGameState>();

	// Flash team colour
	if ( (FMath::FloorToInt(ConstructionTimeElapsed) % 2) == 0)
	{
		NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), UTGS->Teams[TeamNum]->TeamColor);
	}
	else
	{
		NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), FLinearColor::White);
	}

	// Set node to active then heal reaches its damage capacity
	if (Health >= DamageCapacity)
	{
		GetWorldTimerManager().ClearTimer(ConstructionTimer);
		
		GotoState(NodeState::Active);
	}
}

void AUTOnslaughtPowerNode::OnNodeStateNeutral_Implementation()
{
	NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), FLinearColor::White);
}

void AUTOnslaughtPowerNode::OnNodeStateActive_Implementation()
{
	AUTOnslaughtGameState* UTGS = GetWorld()->GetGameState<AUTOnslaughtGameState>();

	// FIXME: sometimes GameState does not load in time so we cant set the team color, for now its set as Yello
	if (UTGS && UTGS->Teams.IsValidIndex(TeamNum))
		NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")),  UTGS->Teams[TeamNum]->TeamColor);
	else
		NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), FLinearColor::Yellow);
}

void AUTOnslaughtPowerNode::SetNodeShields(AUTCharacter* UTCharacter)
{
}

void AUTOnslaughtPowerNode::OnNodeStateDestroyed_Implementation()
{
	//NodeBeamMaterialInstance->SetVectorParameterValue(FName(TEXT("TeamColour")), FLinearColor::White);

	if (Role == ROLE_Authority)
	{
		// HACk/FIXME Set a 0.01 timer to call GotoState neutral, overwise it replication does not happen quick enough ...
		// @see https://answers.unrealengine.com/questions/165678/using-settimer-on-a-function-with-parameters.html
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDel;

		//Binding the function with specific values
		TimerDel.BindUFunction(this, FName("GotoState"), NodeState::Neutral);

		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 0.01f, false);
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Client: OnNodeStateDestroyed"));
	}
}

void AUTOnslaughtPowerNode::OnRep_Vulnerability()
{
	if (Vulnerability == EVulnerability::InvulnerableByNearbyOrb)
	{
	}
	else
	{
	}

	//UpdateShield();
}

void AUTOnslaughtPowerNode::SeveredHealth()
{

	// Check that we are still severerd from the network
	if (PoweredBy(GetTeamNum()))
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Severed node reconnected: %s"), *GetName());

		// Reconnected to network
		GetWorldTimerManager().ClearTimer(SeveredTimer);

		// No longer severed
		if (Role == ROLE_Authority)
			bIsSevered = false;
	}
	else
	{
		// Take damage amount
		if (Role == ROLE_Authority)
			Health -= SeveredDamagePerSecound;

		// Is the node still alive? 
		if (Health <= 0.f)
		{
			GetWorldTimerManager().ClearTimer(SeveredTimer);

			if (Role == ROLE_Authority)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Severed node destroyed: %s"), *GetName());
				bIsSevered = false;
				GotoState(NodeState::Destroyed);
				//UpdateLinks();
			}
		}
	}
}

void AUTOnslaughtPowerNode::FindNewHomeForFlag()
{
	if (ControllingFlag)
	{
		AUTOnslaughtFlagBase* NewFlagBase = nullptr;

		if (FlagBase != nullptr && FlagBase->MyFlag != nullptr && FlagBase->MyFlag->GetTeamNum() != ControllingFlag->GetTeamNum())
		{
			// There is a flag base at this node and the enemey team has control of it, move the enemey's flag first before moving flag here

			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Enemey flag still at attached flag base"));
			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Current Flag base is: %s"), *FlagBase->GetName());
			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Current Flag base's flag team: %d"), FlagBase->MyFlag->GetTeamNum());
			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Controlling Flag Team Number: %d"), ControllingFlag->GetTeamNum());

			TArray<AUTOnslaughtNodeObjective*> CheckedNodes; // TODO: Work out how to set TArray so that it can be used as a default parameter
			AUTOnslaughtFlagBase* EnemeyFlagNewHomeBase;
			
			AUTOnslaughtFlag* UTOnsEnemeyFlag = Cast<AUTOnslaughtFlag>(FlagBase->MyFlag); 

			// TODO: Should really perform a null check on UTOnsEnemeyFlag
			EnemeyFlagNewHomeBase = UTOnsEnemeyFlag->FindNearestFlagBase(this, CheckedNodes);
			
			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Found a new flag base of the current flag base's flag: %s"), *EnemeyFlagNewHomeBase->GetName());

			// Could not find a close flag base, use the starting home base (core)
			if (!EnemeyFlagNewHomeBase || FlagBase == EnemeyFlagNewHomeBase)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Could not find a new new homebase use starting base"));
				EnemeyFlagNewHomeBase = UTOnsEnemeyFlag->StartingHomeBase;
			}
			
			FName FlagState = FlagBase->GetFlagState();
			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Flag state %s"), *FlagState.ToString());

			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Setting %s flag to %s"), *EnemeyFlagNewHomeBase->GetName() ,*FlagBase->MyFlag->GetName());
			// Set this flag as the new flag base's flag
			EnemeyFlagNewHomeBase->MyFlag = FlagBase->MyFlag;

			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Setting current flagbase flag to null"));
			// Remove the flag from attached flag base
			FlagBase->MyFlag = nullptr;

			//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Setting %s flag home base to %s"), *EnemeyFlagNewHomeBase->MyFlag->GetName(), *EnemeyFlagNewHomeBase->GetName());
			// Set the new hombase for the enemey flag at this base
			UTOnsEnemeyFlag->HomeBase = EnemeyFlagNewHomeBase;
			UTOnsEnemeyFlag->NotifyHomeBaseChanged(EnemeyFlagNewHomeBase);
				
			// Move the flag to its new flag base if its currently home
			if (FlagState == CarriedObjectState::Home)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("PN: Enemey flag is HOME at this base, move to the new home base"));
				UTOnsEnemeyFlag->MoveToHome();
			}

			//UE_LOG(LogUTOnslaught, Warning, TEXT("Enemey flag has now moved, use this flag base and move friendly flag here"));
			NewFlagBase = FlagBase;
		}
		else if (FlagBase != nullptr)
		{
			NewFlagBase = FlagBase;
		}
		else
		{
			TArray<AUTOnslaughtNodeObjective*> CheckedNodes; // TODO: Work out how to set TArray so that it can be used as a default parameter
			NewFlagBase = ControllingFlag->FindNearestFlagBase(this, CheckedNodes);
		}

		//UE_LOG(LogUTOnslaught, Warning, TEXT("---------------------------------------------------------"));

		// Could not find a base, set as starting home base (at the core)
		if (NewFlagBase == nullptr)
		{
			NewFlagBase = ControllingFlag->StartingHomeBase;
		}

		// If the found home base is difference to the one already set
		if (NewFlagBase != ControllingFlag->HomeBase)
		{
			// Remove flag from old flag base
			AUTOnslaughtFlagBase* UTOnsCurrFlagBase = Cast<AUTOnslaughtFlagBase>(ControllingFlag->HomeBase);
			if (UTOnsCurrFlagBase)
			{
				UTOnsCurrFlagBase->MyFlag = nullptr;
			}

			// Set the flags new home base
			ControllingFlag->HomeBase = NewFlagBase;
			ControllingFlag->NotifyHomeBaseChanged(NewFlagBase);

			// Set new home base flag as the controlling flag
			AUTOnslaughtFlagBase* UTOnsNewFlagBase = Cast<AUTOnslaughtFlagBase>(NewFlagBase);

			if (UTOnsNewFlagBase)
			{
				UTOnsNewFlagBase->MyFlag = ControllingFlag;
				UTOnsNewFlagBase->ForceNetUpdate();
			}
		}
	}
}

void AUTOnslaughtPowerNode::PostRenderFor(APlayerController * PC, UCanvas * Canvas, FVector CameraPosition, FVector CameraDir)
{
	// See UTCharator.cpp PostRenderFor

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTPC->PlayerState);

	const bool bOnSameTeam = GS != nullptr && GS->OnSameTeam(PC, this);
	const bool bRecentlyRendered = (GetWorld()->TimeSeconds - GetLastRenderTime() < 0.5f);
	const bool bIsViewTarget = (PC->GetViewTarget() == this);


	// Position above the root where node info will be displayed
	FVector ScreenPosition = Canvas->Project(GetActorLocation() + FVector(0, 0, 500.f));

	// The distence away from the camera this actor is
	float Dist = (CameraPosition - GetActorLocation()).Size();


	// make sure not clipped out
	if (FVector::DotProduct(CameraDir, (GetActorLocation() - CameraPosition)) <= 0.0f)
	{
		return;
	}

	if (!GS)
		return;

	FLinearColor TeamColor = GS->Teams.IsValidIndex(TeamNum) ? GS->Teams[TeamNum]->TeamColor : FLinearColor::White;
	
	FFontRenderInfo TextRenderInfo = Canvas->CreateFontRenderInfo(true, false);

	// Background colour of the beacon
	FLinearColor BackgroundColour = TeamColor;

	// Text colour
	FLinearColor TextColour = FLinearColor::White;

	// Name of the node
	FText NodeName;
	if (PrimeCore != nullptr)
	{
		NodeName = (bDualPrimeCore || GS->OnSameTeam(UTPC, PrimeCore)) ? PrimeNodeName : EnemyPrimeNodeName;
	}
	else
	{
		NodeName = ObjectiveName;
	}

	// Node name - font
	UFont* NodeNameFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;

	// Node name - Font scale
	float NodeNameFontScale = 0.8f;

	// Health number - should show health bar (Active or under construction)
	bool bShowHealth;
	if (CurrentNodeState == NodeState::Active || CurrentNodeState == NodeState::UnderConstruction)
		bShowHealth = true;
	else
		bShowHealth = false;

	// Health number - font
	UFont* HealthFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;

	// Health number - font scale
	float HealthFontScale = 1.0f;


	// fade out beacons near crosshair and on distance - UT4-Gametype-Betrayal
	float AlphaDist = FMath::Clamp<float>((Dist - 1000.0f) / 2000.0f, 0.33f, 1.0f);
	FVector CenterScreen(Canvas->SizeX*0.5, Canvas->SizeY*0.5, 0.0);
	float AlphaFov = FMath::Clamp<float>((ScreenPosition - CenterScreen).Size2D() / (0.25*FMath::Min<int32>(Canvas->SizeX, Canvas->SizeY)), 0.33f, 1.0f);
	float AlphaValue = AlphaDist * AlphaFov;
	TextColour.A *= AlphaValue;
	BackgroundColour.A *= AlphaValue;


	// Calculate the size of the NodeName text
	float NameXL, NameYL;
	Canvas->TextSize(NodeNameFont, NodeName.ToString(), NameXL, NameYL);
	NameXL *= NodeNameFontScale;
	NameYL *= NodeNameFontScale;

	// Calculate the size of the Health text
	float HealthXL, HealthYL;
	int32 HealthPercent = FMath::FloorToInt((Health / DamageCapacity) * 100);

	Canvas->TextSize(HealthFont, FString::Printf(TEXT("%d"), HealthPercent), HealthXL, HealthYL);
	HealthXL *= HealthFontScale;
	HealthYL *= HealthFontScale;

	float XL = FMath::Max(NameXL, HealthXL); // The max width which is being used
	float YL = NameYL + HealthYL; // Hight of area being used

	// Calculate the size of the background
	// Padding around the sides
	float BackPaddingX = 0.1f;
	float BackPaddingY = 0.1f;
	// Calc the hight and width taking into account padding
	float BackW = XL + (4.0f * BackPaddingX);
	float BackH = YL + (2.0f * BackPaddingY);

	float BackPosX = ScreenPosition.X - (0.5f * BackW);
	float BackPosY = ScreenPosition.Y - BackH;


	// Draw the background
	//Canvas->SetLinearDrawColor(BackgroundColour);
	//Canvas->DrawTile(Canvas->DefaultTexture, BackPosX, BackPosY - 0.4f * BackH,  BackW, 1.8f * BackH, 0, 0, 1, 1);

	// Draw the node name
	Canvas->SetLinearDrawColor(TextColour);
	Canvas->DrawText(NodeNameFont, NodeName.ToString(), ScreenPosition.X - 0.5f * NameXL, ScreenPosition.Y - BackPaddingY - NameYL, NodeNameFontScale, NodeNameFontScale, TextRenderInfo);

	if (bShowHealth)
		Canvas->DrawText(HealthFont, FString::Printf(TEXT("%d"), HealthPercent), ScreenPosition.X - 0.5f * NameXL, BackPosY + BackPaddingY, HealthFontScale, HealthFontScale, TextRenderInfo);
/*

	if (FVector::DotProduct(CameraDir, (GetActorLocation() - CameraPosition)) <= 0.0f)
	{
		return;
	}

	float XL, YL;
	float Scale = Canvas->ClipX / 1920.f;

	UFont* TinyFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->MediumFont;
	Canvas->TextSize(TinyFont, *GetName(), XL, YL, Scale, Scale);

	// Float the text 500 unites from the root compenent
	FVector WorldPosition = GetActorLocation() + FVector(0.f, 0.f, 500.f);

	FVector ViewDir = PC->GetControlRotation().Vector();
	float Dist = (CameraPosition - GetActorLocation()).Size();

	//FVector ScreenPosition = Canvas->Project(GetActorLocation() + FVector(0, 0, 500.f));
	bool bDrawEdgeArrow = false;
	FVector ScreenPosition = GetAdjustedScreenPosition(Canvas, WorldPosition, CameraPosition, ViewDir, Dist, 20.f, bDrawEdgeArrow);
	
	float XPos = ScreenPosition.X - (XL * 0.5f);

	if (XPos < Canvas->ClipX || XPos + XL < 0.0f)
	{
		FCanvasTextItem TextItem(
			FVector2D(
				FMath::TruncToFloat(Canvas->OrgX + XPos),
				FMath::TruncToFloat(Canvas->OrgY + ScreenPosition.Y - YL)
			),
			FText::FromString(GetName()),
			TinyFont,
			FLinearColor::White
		);

		TextItem.Scale = FVector2D(Scale, Scale);
		TextItem.BlendMode = SE_BLEND_Translucent;
		TextItem.FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
		Canvas->DrawItem(TextItem);
	}
*/
}

FVector AUTOnslaughtPowerNode::GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow)
{
	FVector Cross = (ViewDir ^ FVector(0.f, 0.f, 1.f)).GetSafeNormal();
	FVector DrawScreenPosition;
	float ExtraPadding = 0.065f * Canvas->ClipX;
	DrawScreenPosition = Canvas->Project(WorldPosition);
	FVector FlagDir = WorldPosition - ViewPoint;
	if ((ViewDir | FlagDir) < 0.f)
	{
		bool bWasLeft = bBeaconWasLeft;
		bDrawEdgeArrow = true;
		DrawScreenPosition.X = bWasLeft ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
		DrawScreenPosition.Y = 0.5f*Canvas->ClipY;
		DrawScreenPosition.Z = 0.0f;
		return DrawScreenPosition;
	}
	else if ((DrawScreenPosition.X < 0.f) || (DrawScreenPosition.X > Canvas->ClipX))
	{
		bool bLeftOfScreen = (DrawScreenPosition.X < 0.f);
		float OffScreenDistance = bLeftOfScreen ? -1.f*DrawScreenPosition.X : DrawScreenPosition.X - Canvas->ClipX;
		bDrawEdgeArrow = true;
		DrawScreenPosition.X = bLeftOfScreen ? Edge + ExtraPadding : Canvas->ClipX - Edge - ExtraPadding;
		//Y approaches 0.5*Canvas->ClipY as further off screen
		float MaxOffscreenDistance = Canvas->ClipX;
		DrawScreenPosition.Y = 0.4f*Canvas->ClipY + FMath::Clamp((MaxOffscreenDistance - OffScreenDistance) / MaxOffscreenDistance, 0.f, 1.f) * (DrawScreenPosition.Y - 0.6f*Canvas->ClipY);
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, 0.25f*Canvas->ClipY, 0.75f*Canvas->ClipY);
		bBeaconWasLeft = bLeftOfScreen;
	}
	else
	{
		bBeaconWasLeft = false;
		DrawScreenPosition.X = FMath::Clamp(DrawScreenPosition.X, Edge, Canvas->ClipX - Edge);
		DrawScreenPosition.Y = FMath::Clamp(DrawScreenPosition.Y, Edge, Canvas->ClipY - Edge);
		DrawScreenPosition.Z = 0.0f;
	}
	return DrawScreenPosition;
}