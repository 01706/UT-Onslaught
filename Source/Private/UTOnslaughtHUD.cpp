#include "UTOnslaught.h"
#include "UTOnslaughtHUD.h"
#include "UTHUD_TeamDM.h"

#include "UTOnslaughtPowerNode.h"

AUTOnslaughtHUD::AUTOnslaughtHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// copy the required hud widgets from the team DM hud @credit RattleSN4k3 see DefaultGame.ini D:\UnrealTournamentSource\UnrealTournament\UnrealTournament\Config\DefaultGame.ini
	RequiredHudWidgetClasses = AUTHUD_TeamDM::StaticClass()->GetDefaultObject<AUTHUD_TeamDM>()->RequiredHudWidgetClasses;
	
	// Add our scoreboard
	RequiredHudWidgetClasses.Remove(TEXT("/Script/UnrealTournament.UTTeamScoreboard"));
	RequiredHudWidgetClasses.Add(TEXT("/Script/UTOnslaught.UTOnslaughtScoreboard"));
}

void AUTOnslaughtHUD::BeginPlay()
{
	Super::BeginPlay();

	// Find all PowerNodes, if they are not disabled, add found node to PostRenderedActors array
	for (TActorIterator<AUTOnslaughtPowerNode> It(GetWorld()); It; ++It)
	{
		AUTOnslaughtPowerNode* PowerNode = *It;
		if (PowerNode && PowerNode->CurrentNodeState != NodeState::Disabled)
		{
			AddPostRenderedActor(PowerNode);
		}
	}
}