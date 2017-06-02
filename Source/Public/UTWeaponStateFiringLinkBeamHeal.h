#pragma once

#include "UTWeaponStateFiringLinkBeam.h"

#include "UnrealTournament.h"
#include "UTWeaponStateFiringLinkBeamHeal.generated.h"

UCLASS()
class UTONSLAUGHT_API UUTWeaponStateFiringLinkBeamHeal : public UUTWeaponStateFiringLinkBeam
{
	GENERATED_UCLASS_BODY()

	virtual void FireShot() override;
	virtual void Tick(float DeltaTime) override;	
};
