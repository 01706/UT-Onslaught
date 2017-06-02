
#include "UTOnslaught.h"
#include "UTOnslaughtGameMessage.h"

UUTOnslaughtGameMessage::UUTOnslaughtGameMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MessageArea = FName(TEXT("Announcements"));
	MessageSlot = FName(TEXT("GameMessages"));

	RedTeamDominates =			NSLOCTEXT("OnslaughtMessage", "RedTeamDominates", "Red Team wins the round!");
	BlueTeamDominates =			NSLOCTEXT("OnslaughtMessage", "BlueTeamDominates", "Blue Team wins the round!");
	RedTeamNodeConstructed =	NSLOCTEXT("OnslaughtMessage", "RedTeamNodeConstructed", "Red Node Constructed!");
	BlueTeamNodeConstructed =	NSLOCTEXT("OnslaughtMessage", "BlueTeamNodeConstructed", "Blue Node Constructed!");
	InvincibleCore =			NSLOCTEXT("OnslaughtMessage", "InvincibleCore", "You Cannot Damage Unlinked Nodes!");
	UnattainableNode =			NSLOCTEXT("OnslaughtMessage", "UnattainableNode", "You Cannot Capture Unlinked Nodes!");
	RedPowerNodeAttacked =		NSLOCTEXT("OnslaughtMessage", "RedPowerNodeAttacked", "Red Node under Attact!");
	BluePowerNodeAttacked =		NSLOCTEXT("OnslaughtMessage", "BluePowerNodeAttacked", "Blue Node under Attact!");
	RedPrimeNodeAttacked =		NSLOCTEXT("OnslaughtMessage", "RedPrimeNodeAttacked", "Prime Node under Attact!");
	BluePrimeNodeAttacked =		NSLOCTEXT("OnslaughtMessage", "BluePrimeNodeAttacked", "Prime Node under Attact!");
	Unpowered =					NSLOCTEXT("OnslaughtMessage", "Unpowered", "Turret is Unpowered!");
	RedPowerNodeDestroyed =		NSLOCTEXT("OnslaughtMessage", "RedPowerNodeDestroyed", "Red Node Destroyed");
	BluePowerNodeDestroyed =	NSLOCTEXT("OnslaughtMessage", "BluePowerNodeDestroyed", "Blue Node Distroyed");
	RedPowerNodeUnderConstruction = NSLOCTEXT("OnslaughtMessage", "RedPowerNodeUnderConstruction", "Red Node Constructing!");
	BluePowerNodeUnderConstruction = NSLOCTEXT("OnslaughtMessage", "BluePowerNodeUnderConstruction", "Blue Node Constructing");
	RedPowerNodeSevered =		NSLOCTEXT("OnslaughtMessage", "RedPowerNodeSevered", "Red Node Isolated!");
	BluePowerNodeSevered =		NSLOCTEXT("OnslaughtMessage", "BluePowerNodeSevered", "Blue Node Isolated!");
	PowerCoresAreDraining =		NSLOCTEXT("OnslaughtMessage", "PowerCoresAreDraining", "Cores are draning!");
	UnhealablePowerCore =		NSLOCTEXT("OnslaughtMessage", "UnhealablePowerCore", "You can't heal your core!");
	PowerNodeShieldedByOrb =	NSLOCTEXT("OnslaughtMessage", "PowerNodeShieldedByOrb", "Node is Shieled by nearby Orb!");
	PowerNodeTemporarilyShielded = NSLOCTEXT("OnslaughtMessage", "PowerNodeTemporarilyShielded", "Node has Temorary Shielding!");
	NoTeleportWithOrb =			NSLOCTEXT("OnslaughtMessage", "NoTeleportWithOrb", "Orb will be dropped if you teleport!");
	RoundDraw =						NSLOCTEXT("OnslaughtMessage", "RoundDraw", "DRAW - Both Cores Drained!");
	Nodebuster =				NSLOCTEXT("OnslaughtMessage", "Nodebuster", "Node Buster!");
	RegulationWin =				NSLOCTEXT("OnslaughtMessage", "RegulationWin", "2 Points for Regulation Win");
	OverteamWin =				NSLOCTEXT("OnslaughtMessage", "OverteamWin", "1 Point for Overtime Win");

	bIsStatusAnnouncement = true;
	bIsPartiallyUnique = true;
	bPlayDuringInstantReplay = false;
	bPlayDuringIntermission = false;
}

FText UUTOnslaughtGameMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	switch (Switch)
	{
		case 0: return RedTeamDominates; break;
		case 1: return BlueTeamDominates; break;
		case 2: return RedTeamNodeConstructed; break;
		case 3: return BlueTeamNodeConstructed; break;
		case 4: return RoundDraw; break;
		case 5: return InvincibleCore; break;
		case 6: return UnattainableNode; break;
		case 7: return Nodebuster; break;
		case 8: break;
		case 9: return RedPrimeNodeAttacked; break;
		case 10: return BluePrimeNodeAttacked; break;
		case 11: return RegulationWin; break;
		case 12: return OverteamWin; break;
		case 13: return Unpowered; break;

		case 16: return RedPowerNodeDestroyed; break;
		case 17: return BluePowerNodeDestroyed; break;
		
		case 23: return RedPowerNodeUnderConstruction; break;
		case 24: return BluePowerNodeUnderConstruction; break;
		
		case 27: return RedPowerNodeSevered; break;
		case 28: return BluePowerNodeSevered; break;
		case 29: return PowerCoresAreDraining; break;
		case 30: return UnhealablePowerCore; break;
		
		case 42: return PowerNodeShieldedByOrb; break;
		case 43: return PowerNodeTemporarilyShielded; break;
		case 44: return NoTeleportWithOrb; break;

		case 255: return FText::FromString("Test");
	}
	return FText::GetEmpty();
}