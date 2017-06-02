#pragma once

#include "UTOnslaught.h"
#include "UTOnslaughtNodeObjective.h"
#include "UTOnslaughtPowerCore.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtPowerCore : public AUTOnslaughtNodeObjective
{
	GENERATED_UCLASS_BODY()
public:

	void InitializeForThisRound(uint8 CoreIndex);

protected:
	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	virtual float HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;
};