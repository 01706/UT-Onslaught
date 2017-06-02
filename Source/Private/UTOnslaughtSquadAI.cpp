
#include "UTOnslaught.h"
#include "UTOnslaughtGameMode.h"
#include "UTOnslaughtSquadAI.h"

void AUTOnslaughtSquadAI::Initialize(AUTTeamInfo* InTeam, FName InOrders)
{
	Super::Initialize(InTeam, InOrders);

	AUTOnslaughtGameMode* GM = GetWorld()->GetAuthGameMode<AUTOnslaughtGameMode>();

	if (GM != NULL && GM->PowerCore.Num() >= 2 && GM->PowerCore[0] != NULL && GM->PowerCore[1] != NULL)
	{
		int32 EnemeyTeam = 1 - InTeam->GetTeamNum();
		EnemeyCore = GM->PowerCore[EnemeyTeam];
	}

	SetObjective(GM->PowerCore[InTeam->GetTeamNum()]);
}