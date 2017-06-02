#include "UTOnslaught.h"
#include "UTOnslaughtGameMode.h"
#include "UTOnslaughtNodeObjective.h"
#include "UTOnslaughtPowerCore.h"
#include "UTOnslaughtGameMessage.h"
#include "UTOnslaughtFlag.h"
#include "UTOnslaughtFlagBase.h"

AUTOnslaughtNodeObjective::AUTOnslaughtNodeObjective(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set Defaults
	MinPlayerCount = 0;
	bDualPrimeCore = false;
	bIsPrimeNode = false;
	bCalledPrimeNode = false;
	bIsSevered = false;
	bIsTeleportDestination = false;

	ConstructionTime = 30.0f;
	
	// Set the FinalCoreDistance array up with 2 slots
	FinalCoreDistance.SetNum(2);
}

void AUTOnslaughtNodeObjective::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
    DOREPLIFETIME(AUTOnslaughtNodeObjective, LinkedNodes);
	DOREPLIFETIME(AUTOnslaughtNodeObjective, CurrentNodeState);
	DOREPLIFETIME(AUTOnslaughtNodeObjective, PrimeCore);
	DOREPLIFETIME(AUTOnslaughtNodeObjective, bDualPrimeCore);
	DOREPLIFETIME(AUTOnslaughtNodeObjective, bIsSevered);
	DOREPLIFETIME(AUTOnslaughtNodeObjective, bIsTeleportDestination);
}

void AUTOnslaughtNodeObjective::UpdateCloseActors()
{

	// Vehicle factories
	/*
	for (int32 i = 0; i < VehicleFactories.Num(); i++)
	{
	VehicleFactories[i]->Deactivate();
	}
	*/

	// Weapon Lockers
	/*
	for (int32 i = 0; i < DeployableLockers.Num(); i++)
	{
		DeployableLockers[i]->Deactivate();
	}
	*/

	// Teleporters
	/*
	for (int32 i = 0; i < NodeTeleporters.Num(); i++)
	{
		NodeTeleporters[i]->SetTeamNum(255);
	}
	*/

	// Special Objectives
	/*
	for (int32 i = 0; i < ActivatedObjectives.Num(); i++)
	{
		ActivatedObjectives[i]->CheckActivate();
	}
	*/

	FindNewHomeForFlag();
}

void AUTOnslaughtNodeObjective::FindNewHomeForFlag()
{
}

void AUTOnslaughtNodeObjective::SetCoreDistance(uint8 Team, uint8 Hops)
{
	uint8 i;

	if (Hops < FinalCoreDistance[Team])
	{
		FinalCoreDistance[Team] = Hops;
		Hops += 1;
		for (i = 0; i < LinkedNodes.Num(); i++)
		{
			LinkedNodes[i]->SetCoreDistance(Team, Hops);
		}
		NumLinks = i;
	}
}

void AUTOnslaughtNodeObjective::InitLinks()
{
	uint8 i;

	for (i = 0; i < LinkedNodes.Num(); i++)
	{
		// If linked to a power core, boost the defense priority			
		AUTOnslaughtPowerCore* PC = Cast<AUTOnslaughtPowerCore>(LinkedNodes[i]);
		if (PC != nullptr)
		{
			DefensePriority = FMath::Max(DefensePriority, 5);
			SetPrimeCore(PC);
		}
		
		LinkedNodes[i]->CheckLinks(this);
	}

	if (bStandalone)
	{
		DefensePriority = DefensePriority - 1;
	}
}

void AUTOnslaughtNodeObjective::SetPrimeCore(AUTOnslaughtNodeObjective* Core)
{
	bIsPrimeNode = true;
	if (bCalledPrimeNode)
	{
		return;
	}

	if ((PrimeCore != NULL) && (PrimeCore != Core))
	{
		bDualPrimeCore = true;
	}

	PrimeCore = Core;
}

void AUTOnslaughtNodeObjective::CheckLinks(AUTOnslaughtNodeObjective* Node)
{
	// If linked to a power core, boost the defense priority			
	AUTOnslaughtPowerCore* PC = Cast<AUTOnslaughtPowerCore>(Node);
	if (PC != nullptr)
	{
		DefensePriority = FMath::Max(DefensePriority, 5);
		SetPrimeCore(PC);
	}

	// See if the Node is already in the list
	if (LinkedNodes.Find(Node) != INDEX_NONE)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("NO: Node %s is already linked to %s"), *Node->GetName(), *this->GetName());
		return;
	}

	// Node not in the list, add it
	UE_LOG(LogUTOnslaught, Warning, TEXT("NO: Adding %s to %s linked nodes"), *Node->GetName(), *this->GetName());
	LinkedNodes.Add(Node);
	NumLinks = NumLinks + 1;

}

void AUTOnslaughtNodeObjective::Reset()
{
	Health = DamageCapacity;

	//if (bScriptInitialized)
	//{
	//	SetInitialState()
	//}

	//UpdateCloseActors();
	//SendChangeEvent(None);
}

bool AUTOnslaughtNodeObjective::PoweredBy(uint8 Team)
{
	if (bStandalone)
	{
		// We are a standalown node and always connected to the node network
		return true;
	}
	else
	{
		// Loop all the nodes which we are linked to
		for (uint8 i = 0; i < LinkedNodes.Num(); i++)
		{
			// Same team, active and not severed from the network
			if (LinkedNodes[i]->TeamNum == Team && LinkedNodes[i]->CurrentNodeState == NodeState::Active && !LinkedNodes[i]->bIsSevered)
			{
				// This node is linked to the power node network
				return true;
			}		
		}
		// Could not find any linked nodes, this node is not linked to the node network
		return false;
	}
}

float AUTOnslaughtNodeObjective::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	UE_LOG(LogUTOnslaught, Warning, TEXT("NO: Damage %f"), Damage);

	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();

	// No need to do anything if their was no instigator, the damage taken was 0 or the match has eneded
	if ((EventInstigator == NULL)  || (Damage <= 0.0f) || GS->HasMatchEnded())
		return 0.0f;

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(EventInstigator);
	
	// Check if player is attacking own node, check if node is powered by players team
	if (UTPC && UTPC->GetTeamNum() != TeamNum && PoweredBy(UTPC->GetTeamNum()) )
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("NO: Attcking openents linked node"));

		SetUnderAttack(true);
		if (GetNetMode() != NM_DedicatedServer)
		{
			BecomeUnderAttack();
		}

		LastAttacker = UTPC;

		//Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

		// Take the damage amount from nodes health
		Health -= Damage;
		if (Health <= 0.0f)
		{
			//Reset the node
			// TODO move its down function
			bIsNeutral = true;
			TeamNum = 255;

			// Update any effects
			//if (Role != NM_DedicatedServer)
				//UpdateEffects();

			UE_LOG(LogUTOnslaught, Warning, TEXT("Distroyed by %s"), *LastAttacker->GetName());
			return 100.f;
		}
		else
		{
			// Attack notification
			UE_LOG(LogUTOnslaught, Warning, TEXT("Under attack remaining health %f"), Health);
			return 50.f;
		}
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("NO: Attcking node which is not linked or owned by same team"));
	}
	
	//return ;
	return 1.0f;
}

float AUTOnslaughtNodeObjective::HealDamage(float Damage, const FDamageEvent & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	if (Role == ROLE_Authority)
	{	
		if (Health + Damage >= DamageCapacity)
		{
			Health = DamageCapacity;
		}
		else
		{
			Health += Damage;
		}

		return Damage;
	}

	return 0.f;
}

void AUTOnslaughtNodeObjective::BeginPlay()
{
	Super::BeginPlay();
}

void AUTOnslaughtNodeObjective::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void AUTOnslaughtNodeObjective::OnRep_NodeStateChanged()
{
}

void AUTOnslaughtNodeObjective::SetSevered()
{
	UE_LOG(LogUTOnslaught, Warning, TEXT("Node %s severed from node network"), *GetName());

	bIsSevered = true;

	ServeredTimeElapsed = 0;
	GetWorldTimerManager().SetTimer(SeveredTimer, this, &AUTOnslaughtNodeObjective::SeveredHealth, 1.0f, true);

	AUTTeamGameMode* GM = GetWorld()->GetAuthGameMode<AUTTeamGameMode>();
	if (GM)
	{
		if (GetTeamNum() == 0)
		{
			GM->BroadcastLocalized(this, UUTOnslaughtGameMessage::StaticClass(), 27, NULL, NULL, NULL);
		}
		else if (GetTeamNum() == 1)
		{
			GM->BroadcastLocalized(this, UUTOnslaughtGameMessage::StaticClass(), 28, NULL, NULL, NULL);
		}
	}
}

void AUTOnslaughtNodeObjective::SeveredHealth()
{
}

void AUTOnslaughtNodeObjective::UpdateLinks()
{
	AUTOnslaughtGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTOnslaughtGameMode>();
	if (GameMode)
	{
		GameMode->UpdateLinks();
	}
}

void AUTOnslaughtNodeObjective::InitCloseActors()
{
	Super::InitCloseActors();
	// Remove reference to any teleporters around us
	//NodeTeleporters.Empty();
}

bool AUTOnslaughtNodeObjective::IsValidSpawnFor(uint32 Team)
{
	// This node is on the same team and there are player starts
	if (TeamNum == Team && (PlayerStarts.Num() > 0))
	{
		// NOTE: may need to add more condidtion here like if the node is under attact or not
		if (CurrentNodeState == NodeState::Active)
		{
			return true;
		}
	}

	return false;
}

float AUTOnslaughtNodeObjective::GetSpawnRating(uint32 EnemyTeam)
{
	// if (bStandalone && GetBestAvailableVehicleRating <= 0.0f)
	if (bStandalone)
	{
		return 255.f;
	}
	else
	{
		return Super::GetSpawnRating(EnemyTeam);
	}
}