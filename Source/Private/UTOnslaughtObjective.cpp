#include "UTOnslaught.h"
#include "UTOnslaughtObjective.h"

AUTOnslaughtObjective::AUTOnslaughtObjective(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set defaults
	bUnderAttack = false;
	bIsNeutral = true;
	bAssociatePlayerStarts = false;
	DefensePriority = 2;

	// Replicate this and child actors to clients
	SetReplicates(true);
	bAlwaysRelevant = true;
}

void AUTOnslaughtObjective::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTOnslaughtObjective, Health);
	DOREPLIFETIME(AUTOnslaughtObjective, bUnderAttack);
}

bool AUTOnslaughtObjective::IsCritical()
{
	// Objectives are considered critical if their health is less than 20% of max
	if (Health < DamageCapacity * 0.2)
	{
		return true;
	}
	return false;
}

bool AUTOnslaughtObjective::PoweredBy(uint8 TeamIndex)
{
	return false;
}

void AUTOnslaughtObjective::BecomeUnderAttack()
{
}

void AUTOnslaughtObjective::SetUnderAttack(bool bNewUnderAttack)
{
	bUnderAttack = bNewUnderAttack;

	if (bUnderAttack)
	{
		// Timer to clearUnderAttack
		GetWorldTimerManager().SetTimer(AttackTimer, this, &AUTOnslaughtObjective::ClearUnderAttack, 5.0f, true);
	}
	else
	{ 
		GetWorldTimerManager().ClearTimer(AttackTimer);
	}
}

void AUTOnslaughtObjective::ClearUnderAttack()
{
	SetUnderAttack(false);
}

void AUTOnslaughtObjective::InitCloseActors()
{
	// Remove any references to player starts and vehicle factories
	PlayerStarts.Empty();
	//VehicleFactories.Empty();
}

float AUTOnslaughtObjective::GetSpawnRating(uint32 EnemyTeam)
{	
	// return FinalCoreDistance[EnemyTeam] - FMath::Min(GetBestAvailableVehicleRating(), 0.99f)
	return FinalCoreDistance[EnemyTeam];
}