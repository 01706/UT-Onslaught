#pragma once

#include "UTOnslaught.h"
#include "UTOnslaughtNodeObjective.h"

#include "UTOnslaughtPowerNode.generated.h"

UENUM(BlueprintType)
enum class EVulnerability : uint8
{
	Vulnerable, // node is vulnerable
	InvulnerableByNearbyOrb, // node is invulnerable because of nearby orb
	InvulnerableToOrbCapture, // node is invulnerable (only to orb captures) because it was recently captured by the orb
};

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtPowerNode : public AUTOnslaughtNodeObjective
{
	GENERATED_UCLASS_BODY()

public: 

	// Actor compent -  Collision capsule used to trigger capture
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UCapsuleComponent* Collision;

	// Actor compent -  Static mesh repersenting the node base
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* NodeBase;

	// Actor compent - Static mesh repersenting the top of the node
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* NodeTop;

	// NOTE: Change to a partical system?
	// Actor compent - Static mesh repersenting the beam
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* NodeBeam;

	// Matrial to use on the node beam, MUST have a variable called TeamColour
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) // Stop garbage collection, was causing crashes on begin play
	UMaterialInterface* NodeBeamMaterial;

	// Node beam material instance
	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* NodeBeamMaterialInstance;

	// Changes the power nodes state
	UFUNCTION()
	virtual void GotoState(FName NewState);
	
	// Server
	UFUNCTION()
	void SetNodeStateUnderConstruction(AUTCharacter* Constructor);
	
	// Update node effects when when going into a UnderConstruction state
	UFUNCTION(BlueprintNativeEvent, Category = "NodeSate")
	void OnNodeStateUnderConstruction();

	// Server
	UFUNCTION()
	void ConstructionHealth();

	// Server
	//UFUNCTION()
	//void SetNodeStateNeutral();

	// Update node effects when when going into a Netural state
	UFUNCTION(BlueprintNativeEvent, Category = "NodeSate")
	void OnNodeStateNeutral();

	// Server
	UFUNCTION()
	void SetNodeShields(AUTCharacter* UTCharacter);
	
	// Server
	//UFUNCTION()
	//void SetNodeStateActive();

	// Update node effects when when going into a Active state
	UFUNCTION(BlueprintNativeEvent, Category = "NodeSate")
	void OnNodeStateActive();
	
	// Update node effects when when going into a Distoryied state
	UFUNCTION(BlueprintNativeEvent, Category = "NodeSate")
	void OnNodeStateDestroyed();

	// Node vulnerability
	UPROPERTY(ReplicatedUsing=OnRep_Vulnerability)
	EVulnerability Vulnerability;

	// Called when the nodes vunerability has changed
	UFUNCTION()
	void OnRep_Vulnerability();
	
	UPROPERTY()
	bool bBeaconWasLeft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText PrimeNodeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText EnemyPrimeNodeName;

	virtual void SeveredHealth() override;

	// Last captured time (in secounds)
	float LastCaptureTime;

	// How long the node in invulnerable to attack after being captured by a orb
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
	float OrbCaptureInvulnerabilityDuration;

	// When the friendly orb carrier is this close and visible, this node cannot be captured by the enemy
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
	float InvulnerableRadius;

	// When friendly orb carrier is within 'InvulnerableRadius' of a prime node the holder gets +1 added to their score every x secounds
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
	float OrbLockScoringInterval;

	// Amount to heal per secound when orb is in line of sight of firendly node
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Orb")
	float OrbHealingPerSecond;

	FTimerHandle OrbHealingTimer;

	// Client, last warning times
	float LastInvulnerabilityScoreTime;
	float LastUnlinkedWarningTime;

	// Person show started construction on this node
	AUTCharacter* Constructor;

	bool bCapturedByOrb;

	/* True when SetInitalState has been called */
	bool bInitialState;

protected:

	// Sets the initial state of the node on BeginPlay
	void SetInitialState();

	UFUNCTION()
	virtual void OnRep_NodeStateChanged() override;

	// TODO: Change function name to NotifyNodeChanged
	// Having issues with CurrentNodeState and TemNum replicating to the client, perform a RPC to the client to froce a update, replication will then hopfully catchup
	UFUNCTION(NetMulticast, Reliable)
	void ClientNodeChanged(FName ServerNodeState, uint8 ServerTeamNum);

	// Triggered if any part of this actor gets hit
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/*
	 * Heals this node by the given damage amount
	 * @return float Amount the actor was healed by
	 */
	virtual float HealDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/* 
	 * Gets the information required to make a called to HealDamage, then calls HealDamage
	 */
	void DoOrbHealing();

	/**
	 * Chceks it see if the event instigator and damage causer can heal this actor
	 * @param ACharacter The Charactor trying to heal
	 * @param AActor The actor being used by the character to heal
	 * @return bool True if instigator and damage causer can heal this actor 
	 */
	virtual bool IsHealable(ACharacter* EventInstigator, AActor* DamageCauser);

	virtual void BeginPlay() override;

	virtual void FindNewHomeForFlag() override;

	virtual void PostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir) override;

	FVector GetAdjustedScreenPosition(UCanvas* Canvas, const FVector& WorldPosition, const FVector& ViewPoint, const FVector& ViewDir, float Dist, float Edge, bool& bDrawEdgeArrow);

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	/**
	 * Perform a flag capture if given character is holding a flag
	 * @param AUTCharacter The Charactor trying to perform a flag capture
	 * @return bool true when captured by flag
	 */
	UFUNCTION()
	bool PerformFlagCapture(AUTCharacter* UTC);

	/**
     * When node is active this will contain a reference to the friendly orb, used to check if its near by
	 */
	UPROPERTY(Replicated)
	AUTOnslaughtFlag* ControllingFlag;

	/**
	 * Checks to see if given flag/orb is near and in sight this node
	 * @param AUTOnslaughtFlag Flag to check
	 * @return bool True when ControllingFlag is near by
	 */
	UFUNCTION(BlueprintCallable, Category = UTOnslaught)
	bool VerifyOrbLock(AUTOnslaughtFlag* CheckedFlag);

	/**
	 * Timer used to call CheckInvulnerability
	 */
	FTimerHandle VulnerabilityTimer;

	/**
	 * Finds and checks the presence of the firendly orb, when present set Invulnerability
	 * Called via VulnerabilityTimer
	 */
	UFUNCTION()
	void CheckInvulnerability();
};