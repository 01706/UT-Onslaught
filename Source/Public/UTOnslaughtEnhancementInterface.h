#pragma once

#include "UTOnslaught.h"
#include "UTOnslaughtEnhancementInterface.generated.h"

UINTERFACE(Blueprintable)
class UUTOnslaughtEnhancementInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class UTONSLAUGHT_API IUTOnslaughtEnhancementInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	// Sets the node which controls this enhancement 
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement Interface")
	void SetControllingNode(AActor* NewObjective);

	// Gets the node which controls this enhancement
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement Interface")
	AActor* GetControllingNode();

	// Called when the controlling node becomes active
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement Interface")
	void ActivateEnhancement();

	// Called when the controlling node deactivates
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "Onslaught Enhancement Interface")
	void DeactivateEnhancement();


	void UpdateTeamEffects();
};
