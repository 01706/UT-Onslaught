#include "UTOnslaught.h"
#include "UTOnslaughtPowerCore.h"

AUTOnslaughtPowerCore::AUTOnslaughtPowerCore(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefensePriority = 10;
	DamageCapacity = 100.0f;
	bIsNeutral = false;
	bAssociatePlayerStarts = true;
}

void AUTOnslaughtPowerCore::InitializeForThisRound(uint8 CoreIndex)
{
	SetCoreDistance(CoreIndex, 0);
	
	if (FlagBase != nullptr && FlagBase->MyFlag != nullptr)
	{

	}
}

void AUTOnslaughtPowerCore::BeginPlay()
{
	CurrentNodeState = NodeState::Active;
}

float AUTOnslaughtPowerCore::TakeDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();

	if ((Role < ROLE_Authority) || (EventInstigator == NULL) || (Damage <= 0.f) || (GS && (GS->HasMatchEnded())))
	{
		return 0.0f;
	}
	else
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("Power Core Taking Damage"));

		AUTPlayerController* UTPC = Cast<AUTPlayerController>(EventInstigator);
		uint8 EnemyTeam = 1 - TeamNum;
		// Check if player is attacking own node, check if node is powered by players team
		if (UTPC && UTPC->GetTeamNum() != TeamNum && PoweredBy(EnemyTeam))
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Attcking openents core"));

			// Take damage amount from health
			Health -= Damage;

			// Health less then 0, distroy the node
			if (Health <= 0.0f)
			{
				CurrentNodeState = NodeState::Destroyed;
				UE_LOG(LogUTOnslaught, Warning, TEXT("Core destroyed"));
			}

			return Damage;
		}
		else
		{
			UE_LOG(LogUTOnslaught, Warning, TEXT("Core not linked to enemys core"));
		}
	}

	return 0.0f;
}

float AUTOnslaughtPowerCore::HealDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	// Power cores can not be healed
	return 0.0f;
}