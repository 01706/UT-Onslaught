#pragma once

#include "UTPlayerController.h"
#include "UTOnslaughtPlayerController.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtPlayerController : public AUTPlayerController
{
	GENERATED_UCLASS_BODY()
	
	virtual void SetupInputComponent() override;

	// Tells the server player wants to distroy a flag
	UFUNCTION(exec)
	virtual void DistroyFlag();

	// Execute client request to distroy flag
	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerDistroyFlag();
};
