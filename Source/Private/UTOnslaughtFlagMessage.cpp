#include "UTOnslaught.h"
#include "UTOnslaughtFlagMessage.h"

UUTOnslaughtFlagMessage::UUTOnslaughtFlagMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Overrideing CTF messages, replacing 'flag' with 'orb'
	ReturnMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "ReturnMessage", "{Player1Name} returned the {OptionalTeam} Orb!");
	ReturnedMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "ReturnedMessage", "The {OptionalTeam} Orb was returned!");
	DroppedMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "DroppedMessage", "{Player1Name} dropped the {OptionalTeam} Orb!");
	HasMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "HasMessage", "{Player1Name} took the {OptionalTeam} Orb!");
	YouHaveMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "YouHaveMessage", "YOU HAVE THE ORB!");
	KilledMessagePostfix = NSLOCTEXT("UTOnslaughtFlagMessage", "KilledMessage", " killed the {OptionalTeam} Orb carrier!");
	NoReturnMessage = NSLOCTEXT("UTOnslaughtFlagMessage", "NoPickupFlag", "You can't pick up this orb.");
}