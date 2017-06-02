#include "UTOnslaught.h"
#include "UTOnslaughtGameState.h"

AUTOnslaughtGameState::AUTOnslaughtGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool AUTOnslaughtGameState::HasMatchEnded() const
{
	return Super::HasMatchEnded();
}