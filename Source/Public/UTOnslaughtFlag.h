#pragma once

#include "UTCTFFlag.h"
#include "UTOnslaughtFlagBase.h"
#include "UTOnslaughtNodeObjective.h"
#include "UTOnslaughtFlag.generated.h"

// Forward declare
class AUTOnslaughtFlagBase;

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtFlag : public AUTCTFFlag
{
	GENERATED_UCLASS_BODY()

public:

	/** 
	 * Keeps track of starting home base for round resets,
	 * as HomeBase changes depending on the closest friendly base when the flag gets returned
	 */
	UPROPERTY(BlueprintReadOnly)
	AUTOnslaughtFlagBase* StartingHomeBase;

	/*
		Call Stack
			AUTCarriedObject::OnOverlapBegin
				AUTCarriedObject::TryPickup_Implementation(AUTCharacter* Character)
					CanBePickedUpBy
					Character->CanPickupObject
						SetHolder
	*/

	bool CanBePickedUpBy(AUTCharacter* Character) override;

	virtual void MoveToHome() override;

	// HACK: As homebase is set to replicate once on startup
	// Tell clients that the homebase has been updated
	UFUNCTION(NetMulticast, Reliable)
	void NotifyHomeBaseChanged(AUTOnslaughtFlagBase* NewHomeBAse);

	// Find the nearest flag base to the given objective on the Onslaught node network
	AUTOnslaughtFlagBase* FindNearestFlagBase(AUTOnslaughtNodeObjective* CurrentNode, TArray<AUTOnslaughtNodeObjective*>& CheckedNodes);
};
