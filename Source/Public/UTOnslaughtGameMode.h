#pragma once

#include "UTOnslaught.h"
#include "UTOnslaughtGameState.h"
#include "UTOnslaughtObjective.h"
#include "UTOnslaughtNodeObjective.h"
#include "UTOnslaughtPowerCore.h"        
#include "UTOnslaughtPowerNode.h"   
#include "UTOnslaughtLevelScriptActor.h"

#include "UTOnslaughtGameMode.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtGameMode : public AUTTeamGameMode
{
	GENERATED_UCLASS_BODY()

public:

	// Holdes all the Power Nodes (UTOnslaughtObjective) within the map
	TArray<AUTOnslaughtNodeObjective*> PowerNodes;

	// Holdes all the power cores within the map
	TArray<AUTOnslaughtPowerCore*> PowerCore;

	// Holdes the two power orbs within the map
	//TArray<UTOnslaughtOrbs> Orbs[2];

	// TODO Move to player state
	// The node which the user selected as a spawn location 
	AUTOnslaughtNodeObjective* SelectedStartNode;
	
	// Blueprint level script actor
	AUTOnslaughtLevelScriptActor* OnslaughtLvlBP;

	// Name of the active link setup
	FString ActiveSetupName;

	// Apply the given link setup
	void ApplyLinkSetup(FString LinkSetupName);
	
	// Removes current link setup
	void RemoveLinkSetup();

	// Finds and initializes any power cores within the level
	void SetPowerCores();

	// Determine which powernodes are severed
	void UpdateLinks();

	void CheckSevering(AUTOnslaughtNodeObjective* CheckNode, uint8 Team);

	//Associate certain actor classes with near onslaught objectives
	void FindCloseActors();

	// Finds the closest objective to given actor
	AUTOnslaughtObjective * ClosestObjectiveTo(AActor * A);

	// Finds the closest node objective to given actor
	AUTOnslaughtNodeObjective* ClosestNodeTo(AActor * A);

	// First thing which is called when starting the game mode
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	// @see UTOnslaughtPlayerController
	// Called from the player controller when player presses key binding for RequestRally (default 'E')
	virtual bool HandleDistroyFlag(AUTPlayerController* C);

	// Comes from AUTGameMode
	virtual void CheckGameTime() override;

protected:

	// File name of the custom setup file, saved under ./Saved/
	FString CustomLinkSetupIni;

	// Find a player start for given player, called from FindPlayerStart
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// returns whether a node with a flag at it should be prioritized for spawning members of Team
	bool ShouldPrioritizeNodeWithFlag(uint32 Team, uint32 EnemyTeam, bool bEnemyCanAttackCore, AController* Player);

	// Returns a player start at a given node for a given player
	AActor* BestPlayerStartAtNode(AUTOnslaughtObjective* Node, AController* Player);

	// Saves all the link setups to a config file
	void SaveLinkSetupToConfig();

	// Attemps to load the given link setup from a config file, if found OutSetup is populated with the setup
	bool LoadLinkSetupFromConfig(const FString SetupName, FLinkSetup& OutSetup);

	// Attempts to create a link set up based on a Json string
	bool NewLinkSetupFromString(const FString SetupString, FLinkSetup& OutSetup);

private:

	/**
	* Make a Json string friendly for writing out to UE .ini config files.
	* The opposite of GetLayoutStringFromIni.
	*
	* @see: Engine/Source/Runtime/Slate/Private/Framework/Docking/LayoutService.cpp
	*/
	FString PrepareLinkSetupStringForIni(const FString & LinkSetupString);

	/**
	* Convert from UE .ini Json string to a vanilla Json string.
	* The opposite of PrepareLayoutStringForIni.
	*
	* @see: Engine/Source/Runtime/Slate/Private/Framework/Docking/LayoutService.cpp
	*/
	FString GetLinkSetupStringFormIni(const FString & LinkSetupString);
};