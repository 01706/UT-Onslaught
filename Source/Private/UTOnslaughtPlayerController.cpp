
#include "UTOnslaught.h"
#include "UTOnslaughtGameMode.h"
#include "UTOnslaughtPlayerController.h"

AUTOnslaughtPlayerController::AUTOnslaughtPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AUTOnslaughtPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Override RequestRally key binding, default key 'E'
	InputComponent->BindAction("RequestRally", IE_Pressed, this, &AUTOnslaughtPlayerController::DistroyFlag);
}

void AUTOnslaughtPlayerController::DistroyFlag()
{
	if (UTPlayerState)
	{
		ServerDistroyFlag();
	}
}

bool AUTOnslaughtPlayerController::ServerDistroyFlag_Validate()
{
	return true;
}

void AUTOnslaughtPlayerController::ServerDistroyFlag_Implementation()
{
	AUTCharacter* UTC = GetUTCharacter();

	if (UTC && !UTC->IsRagdoll())
	{
		AUTOnslaughtGameMode* Game = GetWorld()->GetAuthGameMode<AUTOnslaughtGameMode>();
		if (Game)
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Sending request to game mode!"));
			Game->HandleDistroyFlag(this);
		}
	}
}