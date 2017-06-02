#pragma once

#include "UTOnslaughtGameMessage.generated.h"

UCLASS()
class UTONSLAUGHT_API UUTOnslaughtGameMessage : public UUTLocalMessage
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText EnemyOrbNearby;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText NodeNotLinked;

	// Copy UTOnslaughtMessage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedTeamDominates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BlueTeamDominates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedTeamNodeConstructed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BlueTeamNodeConstructed;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText InvincibleCore;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText UnattainableNode;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedPowerNodeAttacked;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BluePowerNodeAttacked;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedPrimeNodeAttacked;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BluePrimeNodeAttacked;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText Unpowered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedPowerNodeDestroyed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BluePowerNodeDestroyed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedPowerNodeUnderConstruction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BluePowerNodeUnderConstruction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RedPowerNodeSevered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText BluePowerNodeSevered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText PowerCoresAreDraining;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText UnhealablePowerCore;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText PowerNodeShieldedByOrb;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText PowerNodeTemporarilyShielded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText NoTeleportWithOrb;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RoundDraw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText Nodebuster;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText RegulationWin;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText OverteamWin;

	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const override;
};