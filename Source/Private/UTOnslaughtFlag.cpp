
#include "UTOnslaught.h"
#include "UTOnslaughtFlagBase.h"
#include "UTOnslaughtFlag.h"
#include "UTOnslaughtFlagMessage.h"


AUTOnslaughtFlag::AUTOnslaughtFlag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MessageClass = UUTOnslaughtFlagMessage::StaticClass();
}

bool AUTOnslaughtFlag::CanBePickedUpBy(AUTCharacter* Character)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS != NULL && (!GS->IsMatchInProgress() || GS->IsMatchIntermission()))
	{
		return false;
	}
	else if (Character->IsRagdoll())
	{
		// check again later in case they get up
		GetWorldTimerManager().SetTimer(CheckTouchingHandle, this, &AUTCarriedObject::CheckTouching, 0.1f, false);
		return false;
	}
	else if (GetTeamNum() == Character->GetTeamNum())
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag Same team pickup"));
		return true;
	}
	else
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Enemey trying to pickup flag"));

		AUTPlayerController* PC = Character ? Cast < AUTPlayerController>(Character->GetController()) : NULL;
		if (PC)
		{
			PC->ClientReceiveLocalizedMessage(MessageClass, 13);
		}

		return false;
	}
}

void AUTOnslaughtFlag::MoveToHome()
{
	//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: Moving to home %s"), *HomeBase->GetName());
	Super::MoveToHome();
}

void AUTOnslaughtFlag::NotifyHomeBaseChanged_Implementation(AUTOnslaughtFlagBase* NewHomeBase)
{
	if (HomeBase != NewHomeBase)
	{
		HomeBase = NewHomeBase;
	}
}

AUTOnslaughtFlagBase* AUTOnslaughtFlag::FindNearestFlagBase(AUTOnslaughtNodeObjective* CurrentNode, TArray<AUTOnslaughtNodeObjective*>& CheckedNodes)
{
	AUTOnslaughtFlagBase* Result;

	//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: FindNearestFlagBase Called, CheckedNode %d"), CheckedNodes.Num());

	if (CheckedNodes.Num() > 100)
	{
		return nullptr;
	}
	else
	{
		CheckedNodes.Add(CurrentNode);

		// Check adjacent nodes for a flag base; if one is found, return it
		for (int32 i = 0; i < CurrentNode->LinkedNodes.Num(); i++)
		{
			if (CurrentNode->LinkedNodes[i] != nullptr &&
				CurrentNode->LinkedNodes[i]->FlagBase != nullptr &&
				CurrentNode->LinkedNodes[i]->GetTeamNum() == GetTeamNum() &&
				CurrentNode->LinkedNodes[i]->CurrentNodeState == NodeState::Active)
			{
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: Found a FlagBase on a linked node"));
				return CurrentNode->LinkedNodes[i]->FlagBase;
			}
		}

		//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: Could not find a linked node with a FlagBase!"));

		// Didn't find one, so ask the nodes if any adjacent to them have a flag base
		for (int32 i = 0; i < CurrentNode->LinkedNodes.Num(); i++)
		{
			if (CurrentNode->LinkedNodes[i] != nullptr &&
				CurrentNode->LinkedNodes[i]->GetTeamNum() == GetTeamNum() &&
				CheckedNodes.Find(CurrentNode->LinkedNodes[i]) == INDEX_NONE
			)
			{
				Result = FindNearestFlagBase(CurrentNode->LinkedNodes[i], CheckedNodes);
				if (Result != nullptr)
				{
					//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: Found a FlagBase on distance linked node"));
					return Result;
				}
			}
		}
	}

	// No flag bases found
	//UE_LOG(LogUTOnslaught, Warning, TEXT("Flag: No FlagBase's found"));
	return nullptr;
}
