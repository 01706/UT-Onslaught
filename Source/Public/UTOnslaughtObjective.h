#pragma once

#include "UTOnslaught.h"
#include "UTOnslaughtObjective.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtObjective : public AUTGameObjective
{
	GENERATED_UCLASS_BODY()

	float LastAttackMessageTime;
	float LastAttctTime;
	float LastAttackEpirationTime;
	float LastAttactAnnouncementTime;
	uint8 LastAttactSwitch;

	UPROPERTY(VisibleAnywhere, category = "Objective")
	TArray<uint8> FinalCoreDistance;

	// Person who last damaged us
	float LastDamagedBy;

	// Person who last attacted us
	AUTPlayerController* LastAttacker;

	// Name of this objective
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Objective")
	FText ObjectiveName;

	// Amount of damage which can be taken before being destroyed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Objective")
	float DamageCapacity;

	// Current health
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, category = "Objective")
	float Health;

	// If > 0, Link Gun secondary heals an amount equal to its damage times this
	float LinkHealMult;
	
	// True when anyone can capture
	UPROPERTY(VisibleAnywhere, category = "Objective")
	bool bIsNeutral;
	
	// Can this node have player starts associated with it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Objective")
	bool bAssociatePlayerStarts;

	// Player starts which are near this objective
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, category = "Objective")
	TArray<AUTPlayerStart*> PlayerStarts;

	// True when this objective is under attact
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=BecomeUnderAttack, category = "Objective")
	bool bUnderAttack;

	// Returns weather or not this objective's health is critical
	bool IsCritical();

	// Returns weather or not this objective is powered by a given team
	bool PoweredBy(uint8 TeamIndex);

	FTimerHandle AttackTimer;

	UFUNCTION()
	virtual void BecomeUnderAttack();

	void SetUnderAttack(bool bNewUnderAttack);

	void ClearUnderAttack();
	
	// Nodes with Higher DefensePriority are defended first
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, category = "Objective")
	int32 DefensePriority;

	virtual void InitCloseActors();

	virtual float GetSpawnRating(uint32 EnemyTeam);
};
