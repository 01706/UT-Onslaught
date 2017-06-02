# Onslaught /Warfare for UT4

This is a ported version of the Unreal Tournmanet 3 game mode **Onslaught/Warfare** for the latest version of **_[Unreal Tournament](//https://www.unrealtournament.com)_**.

For the time being this is going to be a straight port of the UT3 version, I have seen the lengthy discussions on Onslaught/Warfare on the forums and how to improve it, but my current ability is limited (see Disclaimer below)

## Build

Make sure you have the latest Unreal Tournament source release, can be got from **_[here](//https://github.com/EpicGames/UnrealTournament/tree/release)_**
Download/Clone this repositry to your Plugins folder e.g.
``` sh
D:\UTDev\UnrealTournament\UnrealTournament\Plugins
```
Run GenerateProjectFiles.bat
Open Visual Studio and build UnrealTournament

## Usage

Open the editor
Within the Content Explorer make sure show plugin content is on
Open 'UTOnsluaght Content/Maps/TestMap/WAR-TestMap'
Set the number of players to 2
Click Play

## Current Features

- Basic node beam effects showing team colour
- Basic win condition checking
- Node stats, Disabled, Neutral, Under Construction, Active, Destroyed
- Healability interface, can be used on any actor which you wish to be healable with the linkguns alt fire
- Linkgun alt fire can heal teammates. (Requires healability interface on character and linkgun firing state change)
- Linkgun alt fire can heal friendly nodes when in the states, Under Construction (Speeds up condition), Active.
- Nodes can be attacked when linked with enemy node(s)
- Nodes can become severed from the node network e.g. side nodes, they will slowly decrease in health and be destroyed if they are not reconnected, reconnected nodes will stay at the health level they were at when severed.
- Dual prime nodes
- Node naming
- Basic node HUD, shows percentage of health and node name (Prime Node or given name)
- Custom link setups using re-patented level blueprint with ?LinkSetup= URL option
- Export link setup (Not currently exposed to editor)
- Load link setup from file
- Player starts, on start or respawn player goes to a friendly node closest to the enemy core
- Destroy dropped enemy orb by pressing 'E' (Rally now key binding)
- Nodes can be captured with a Orb when in states, Neutral, Under Construction, Active
- Orb can link/heal/shield friendly node when in line of sight of node (known issue, healing seems to be every other second even when timer is set every second)
``` sh
	@ Node
	W Wall
	* Orb carrier

	Orb cannot link/heal/shield node, enemy team can attack
	|----------|
	|          |  
	|    @     |
	|   WWWW   |
	|    *     |
	|__________|

	Orb can link/heal/shield node, enemy team can’t attack
	|----------|
	|        * |  
	|    @     |
	|   WWWW   |
	|          |
	|__________|
```	
- Enemy node cannot be attacked when enemy orb carrier is in sight of node (linked/healing/shielding). Basic message sent to user when trying to attack.
- Node captures with orb shields that node for a set given time, enemy cant attack or capture with orb in till shield has faded (currently no effects just a HUD message)
- When friendly Orb is near a friendly PrimeNode which can be attracted by the enemy, holder of said orb will get +1 score every 'OrbLockScoringInterval' seconds for shielding
- HUD Messages (basic: orb, cannot attack node, node shielded)
- Orb base's next to power nodes
- Orb's moves to a power node when captured, if that node has a orb base. Enemy orbs go back to a Enemy node with orb base.
- Node Enhancements: Allows the power node to notify nearby actors when it becomes active or is destroyed (deactivated) can be used for bridges, doors, etc (Can be implanted with blueprint or C++)

#### Disclaimer

This is an extremely early version, there is a lot of placeholder content and there are chunks of code which shouldn't be where they are.

I don't class myself as a programmer, this is the first time creating anything other the 'Hello World'; it's also the first time using Unreal Engine, both learning C++ and the engine at the same time is a massive learning curve. 

IF there is (I say if but there is bound to be) anything which absolutely shocking or you wish contribute just create a pull request or raise a issue, open to all suggestions.
