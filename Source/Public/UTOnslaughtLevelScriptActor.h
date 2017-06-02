#pragma once
#include "UTOnslaught.h"

#include "Json.h"
#include "Engine/LevelScriptActor.h"
#include "UTOnslaughtLevelScriptActor.generated.h"


USTRUCT()
struct FFullNodes
{
	GENERATED_USTRUCT_BODY()
public:
	// Where the link to coming from
	UPROPERTY(EditDefaultsOnly)
	AUTOnslaughtNodeObjective* FromNode;

	// Where the link is going
	UPROPERTY(EditDefaultsOnly)
	AUTOnslaughtNodeObjective* ToNode;
};


USTRUCT()
struct FNodeStartingOwner
{
	GENERATED_USTRUCT_BODY()
public:

	// If the team owns this power core
	UPROPERTY(EditDefaultsOnly)
	AUTOnslaughtPowerCore* StartingOwnerCore;

	// They start with this node
	UPROPERTY(EditDefaultsOnly)
	AUTOnslaughtNodeObjective* Node;
};

USTRUCT()
struct FLinkSetup
{
	GENERATED_USTRUCT_BODY()
public:

	// Setup Name
	UPROPERTY(EditDefaultsOnly)
	FString SetupName;

	// Full link setup
	UPROPERTY(EditDefaultsOnly)
	TArray<FFullNodes> NodeLinks;

	// Standalown nodes in this setup (Nodes which don't need to linked)
	UPROPERTY(EditDefaultsOnly)
	TArray<AUTOnslaughtNodeObjective*> StandaloneNodes;

	// Nodes that start with an owner in this setup
	UPROPERTY(EditDefaultsOnly)
	TArray<FNodeStartingOwner> NodeStartingOwners;

	// Nodes that have a minimum player count in this setup
	// UPROPERTY(EditDefaultsOnly)
	// TArray<> NodeMinPlayers;

	// Actors that should be hidden/deactivated when this link setup is active
	// UPROPERTY(EditDefaultsOnly)
	// TArray<Actor> DeactivatedActors

	// Actors that should be visible/activated when this link setup is active
	// UPROPERTY(EditDefaultsOnly)
	// TArray<Actor> ActivatedActors

	// Hides/Deactivates all actors with the specified Group TODO: groups are now tags?
	// UPROPERTY(EditDefaultsOnly)
	// TArray<FName> DeactivatedGroups;

	// Visible/Activates all actors with the specified Group TODO: groups are now tags?
	// UPROPERTY(EditDefaultsOnly)
	// TArray<FName> ActivatedGroups;

	
	// Set defaults
	FLinkSetup()
	{
		SetupName = TEXT("Default");
	}
	
	// Create a Json respective of this struct when asked to convert to sting
	FString ToString() const
	{

		TSharedRef<FJsonObject> LinkJson = MakeShareable(new FJsonObject());
		
		// Setup Name
		LinkJson->SetStringField(TEXT("SetupName"), SetupName);

		// Node Links
		TArray< TSharedPtr<FJsonValue> >  NodesAsJson;
		for (int32 i = 0; i < NodeLinks.Num(); i++)
		{
			TSharedRef<FJsonObject> NodeJson = MakeShareable(new FJsonObject());
			NodeJson->SetStringField(TEXT("FromNode"), NodeLinks[i].FromNode->GetName());
			NodeJson->SetStringField(TEXT("ToNode"), NodeLinks[i].ToNode->GetName());

			NodesAsJson.Add( MakeShareable( new FJsonValueObject(NodeJson) ) );
		}
		LinkJson->SetArrayField(TEXT("NodeLinks"), NodesAsJson);

		// StandaloneNodes
		TArray< TSharedPtr<FJsonValue> >  StandalownNodesAsJson;
		for (int32 i = 0; i < StandaloneNodes.Num(); i++)
		{
			StandalownNodesAsJson.Add(MakeShareable(new FJsonValueString(StandaloneNodes[i]->GetName())));
		}
		LinkJson->SetArrayField(TEXT("StandaloneNodes"), StandalownNodesAsJson);

		// NodeStartingOwners
		TArray< TSharedPtr<FJsonValue> > NodeStartingOwnersAsJson;
		for (int32 i = 0; i < NodeStartingOwners.Num(); i++)
		{
			TSharedRef<FJsonObject> NodeStartingJson = MakeShareable(new FJsonObject());
			NodeStartingJson->SetStringField(TEXT("StartingOwnerCore"), NodeStartingOwners[i].StartingOwnerCore->GetName());
			NodeStartingJson->SetStringField(TEXT("Node"), NodeStartingOwners[i].Node->GetName());

			NodeStartingOwnersAsJson.Add(MakeShareable(new FJsonValueObject(NodeStartingJson)));
				
		}
		LinkJson->SetArrayField(TEXT("NodeStartingOwners"), NodeStartingOwnersAsJson);


		// Convert to string
		FString LayoutAsString;
		TSharedRef< TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR> > > Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR> >::Create(&LayoutAsString);
		
		check(FJsonSerializer::Serialize(LinkJson, Writer));

		return LayoutAsString;
	}
};


UCLASS()
class UTONSLAUGHT_API AUTOnslaughtLevelScriptActor : public ALevelScriptActor
{
	GENERATED_UCLASS_BODY()
	
	// Map node setups
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = OnslaughtLevel)
	TArray<FLinkSetup> LinkSetups;
};