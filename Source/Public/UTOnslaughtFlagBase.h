#pragma once

#include "UTCTFFlagBase.h"
#include "UTOnslaughtEnhancementInterface.h"
#include "UTOnslaughtFlagBase.generated.h"


UCLASS()
class UTONSLAUGHT_API AUTOnslaughtFlagBase : public AUTCTFFlagBase, public IUTOnslaughtEnhancementInterface
{
	GENERATED_UCLASS_BODY()

public:
	
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Interface implantations 

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement FlagBase")
	void SetControllingNode(AActor* NewObjective);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement FlagBase")
	AActor* GetControllingNode();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement FlagBase")
	void ActivateEnhancement();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement FlagBase")
	void DeactivateEnhancement();


protected:

	virtual void CreateCarriedObject() override;

	// TODO: change AActor to UTOnsluaghtNodeObjective
	// The node which controls this flagbase
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Flag")
	AActor* ControllingNode;
};
