
#include "UTOnslaught.h"
#include "UTCharacterOnslaught.h"


AUTCharacterOnslaught::AUTCharacterOnslaught(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool AUTCharacterOnslaught::IsHealable(ACharacter* EventInstigator, AActor* DamageCauser)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	
	AUTCharacter* UTC = Cast<AUTCharacter>(EventInstigator);
	
	// Can only be healed when instigator and 'this' are on the same team and we needs tobe healed
	if (UTC && GS && GS->OnSameTeam(this, UTC) && Health < HealthMax)
	{
		return true;
	}
	
	return false;
}

float AUTCharacterOnslaught::HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (!ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser))
	{
		return 0.0f;
	}
	else if (Damage < 0.0f)
	{
		UE_LOG(LogUTOnslaught, Warning, TEXT("HealDamage() called with damage %i of type %s... damage amount needs to be possitive"), int32(Damage), *GetNameSafe(DamageEvent.DamageTypeClass));
		return 0.0f;
	}
	else
	{
		// TODO: Check for SuperHealthMax
		// Is the players current health plus the damage amount greater then the maximum health
		if ((Health + Damage) >= HealthMax)
		{
			// Set health to maximum health
			Health = HealthMax;
			return 0.0f;
		}
		else
		{
			// Add damage amount to players health
			Health += Damage;
			UE_LOG(LogUTOnslaught, Warning, TEXT("Player %s healed %f current health %d "), *GetName(), Damage, Health);
			return Damage;
		}
	}
	return 0.0f;
}