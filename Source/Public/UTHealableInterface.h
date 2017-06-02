// Implement this interface for Actors which can be healed by UTWeaponStateFiringLinkBeamHeal (Charators, Objectives, etc)
#pragma once

#include "UTHealableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UUTHealableInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class UTONSLAUGHT_API IUTHealableInterface
{
	GENERATED_IINTERFACE_BODY()

	// Checks to see if this actor can be healed by the instigator
	virtual bool IsHealable(ACharacter* EventInstigator, AActor* DamageCauser)
	{
		return false;
	}
	
	// Apply damage as health to actor
	virtual float HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);
};