#pragma once

#include "UTCharacter.h"
#include "UTHealableInterface.h"
#include "UTCharacterOnslaught.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTCharacterOnslaught : public AUTCharacter, public IUTHealableInterface
{
	GENERATED_UCLASS_BODY()

	// Interface overrides

	virtual float HealDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	
	virtual bool IsHealable(ACharacter* Instigator, AActor* DamageCauser);
};
