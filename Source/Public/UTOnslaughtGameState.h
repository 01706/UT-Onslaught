#pragma once

#include "UTOnslaught.h"
#include "UTGameState.h"

#include "UTOnslaughtGameState.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtGameState : public AUTGameState
{
	GENERATED_UCLASS_BODY()

	virtual bool HasMatchEnded() const override;
};