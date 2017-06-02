#include "UTOnslaught.h"
#include "UTHealableInterface.h"

UUTHealableInterface::UUTHealableInterface(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float IUTHealableInterface::HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	return 0.0f;
}
