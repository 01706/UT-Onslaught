#pragma once

#include "UTOnslaught.h"
#include "UTHealableInterface.h"
#include "UTOnslaughtObjective.h"
#include "UTOnslaughtFlagBase.h"
#include "UTOnslaughtEnhancementInterface.h"
#include "UTOnslaughtNodeObjective.generated.h"

// States which a node can be in
namespace NodeState
{
	const FName Disabled = FName(TEXT("Disabled"));
	const FName Neutral = FName(TEXT("Neutral"));
	const FName UnderConstruction = FName(TEXT("UnderConstruction"));
	const FName Active = FName(TEXT("Active"));
	const FName Destroyed = FName (TEXT("Destroyed"));
}

// Forward declaration
class IUTOnslaughtEnhancementInterface;

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtNodeObjective : public AUTOnslaughtObjective, public IUTHealableInterface
{
	GENERATED_UCLASS_BODY()

public:

	// if > 0, minimum number of players before this node is enabled (checked only when match begins, not during play)
	uint8 MinPlayerCount;

	// Maximum number of links per node
	static const uint8 MaxNumLinks = 8;

	// Number of links this node has
	UPROPERTY(VisibleAnywhere, category = "GameMode")
	uint8 NumLinks = 0;

	// Nodes which are linked to this node
	// replicated to all clients
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, category = "GameMode")
	TArray<AUTOnslaughtNodeObjective*> LinkedNodes;

	// When true players can teleport to this node when owned by the same team
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleAnywhere, category = "GameMode")
	bool bIsTeleportDestination;

	// if set this node can exist and be captured without being linked
	UPROPERTY(VisibleAnywhere, category = "GameMode")
	bool bStandalone;

	// Current node state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_NodeStateChanged, category = "Objective")
	FName CurrentNodeState;

	// Called when CurrentNodeState is updated
	UFUNCTION()
	virtual void OnRep_NodeStateChanged();

	// Time it takes in secounds to complete construction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Objective")
	float ConstructionTime;
	
	// Time in secounds since construction started
	float ConstructionTimeElapsed;
	
	FTimerHandle ConstructionTimer;

	float HealingTime;

	UFUNCTION()
	void UpdateLinks();

	UFUNCTION()
	void SetSevered();

	// Amount of damage recived from palyer when shield is active, used to give player a message
	uint8 ShieldDamageCounter;

	// Shield hit sound effect
	//ShieldHitSound

	// Called by SeveredTimer timer
	virtual void SeveredHealth();

	// True when active node is severed from its power network
	UPROPERTY(Replicated, VisibleAnywhere)
	bool bIsSevered;

	// Time in secounds since node was severed from the network
	float ServeredTimeElapsed;

	FTimerHandle SeveredTimer;

	// Amount of damage to apply to node after it has been severed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Objective")
	float SeveredDamagePerSecound;

	// Node Number
	uint8 NodeNum;

	// The team that owns this PowerCore, starts the game with this node in their control
	UPROPERTY(VisibleAnywhere, category = "GameMode")
	AUTOnslaughtObjective* StartingOwnerCore;

	// set if prime node (adjacent to power core) and bNeverCalledPrimeNode is false 
	UPROPERTY(VisibleAnywhere, Replicated, category = "GameMode")
	AUTOnslaughtNodeObjective* PrimeCore;

	// set if adjacent to both cores
	UPROPERTY(VisibleAnywhere, Replicated, category = "GameMode")
	bool bDualPrimeCore;

	// if set, is prime node
	UPROPERTY(VisibleAnywhere, category = "GameMode")
	bool bIsPrimeNode;

	// If set true, override this node's names with "Prime Node"
	UPROPERTY(VisibleAnywhere, category = "GameMode")
	bool bCalledPrimeNode;

	//UPROPERTY(VisibleAnywhere, category = "GameMode")
	//TArray<UTOnslaughtTeleporter*> NodeTeleporters

	// Node enhancements (tarydium processors, flag/orb, etc.)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, category = "Objective")
	TArray<AActor*> Enhancements;

	// Notify actors associated with this node taht is has been destroyed/disabled
	UFUNCTION()
	void UpdateCloseActors();

	// If there is a FlagBase accociated with this node the flag can be returned here
	UPROPERTY(EditAnyWhere, BlueprintReadOnly, category = "Flag")
	AUTOnslaughtFlagBase* FlagBase;

	// If a flag's homebase is linked to this node, find a new homebase for it
	UFUNCTION()
	virtual void FindNewHomeForFlag();

	// Determine how many hops each node is from powercore in hops
	void SetCoreDistance(uint8 TeamNum, uint8 Hops);

	void InitLinks();

	void SetPrimeCore(AUTOnslaughtNodeObjective* Core);

	void CheckLinks(AUTOnslaughtNodeObjective* Node);

	// Resets nodes health
	void Reset();

	// Checks to see if this power node is connected to the power node network
	bool PoweredBy(uint8 Team);

	virtual void InitCloseActors() override;

	// Checks to see if gvien team an spawn here
	bool IsValidSpawnFor(uint32 Team);

	virtual float GetSpawnRating(uint32 EnemyTeam) override;

	// --- Function overrides from parents

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	virtual float HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);

	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	virtual void BeginPlay() override;
};

