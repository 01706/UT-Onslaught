
#include "UTOnslaught.h"
#include "UTOnslaughtPowerCore.h"
#include "UTOnslaughtFlag.h"
#include "UTOnslaughtFlagBase.h"

AUTOnslaughtFlagBase::AUTOnslaughtFlagBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Replace the default flag base static mesh to the orb holder
	static ConstructorHelpers::FObjectFinder<UStaticMesh> OrbBaseMesh(TEXT("StaticMesh'/UTOnslaught/Flag/StaticMesh/S_FlagBase.S_FlagBase'"));

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetStaticMesh(OrbBaseMesh.Object);

	// Hide collision capsule in editor as its not used (flags cannot be captured here)
	Capsule->bVisible = true;

	TeamNum = 255;
}

void AUTOnslaughtFlagBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTOnslaughtFlagBase, ControllingNode);
}

void AUTOnslaughtFlagBase::OnOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	// No need to do anything when overlapped, teams cannot score by bringing a flag here
}

void AUTOnslaughtFlagBase::SetControllingNode_Implementation(AActor * NewObjective)
{
	AUTOnslaughtNodeObjective* Node = Cast<AUTOnslaughtNodeObjective>(NewObjective);
	if (Node != nullptr)
	{
		Node->Enhancements.Add(this);
		Node->FlagBase = this;
		TeamNum = Node->GetTeamNum();
		
		AUTGameState* UTGS = Cast<AUTGameState>(GetWorld()->GetGameState());
		if (UTGS && MyFlag != nullptr && UTGS->Teams.IsValidIndex(Node->GetTeamNum()))
		{
			MyFlag->SetTeam(UTGS->Teams[Node->GetTeamNum()]);
		}
		ControllingNode = NewObjective;
	}
}

AActor* AUTOnslaughtFlagBase::GetControllingNode_Implementation()
{
	if (ControllingNode)
	{
		return ControllingNode;
	}

	return nullptr;
}

void AUTOnslaughtFlagBase::ActivateEnhancement_Implementation()
{
	//UpdateTeamEffects();
}

void AUTOnslaughtFlagBase::DeactivateEnhancement_Implementation()
{
	//UpdateTeamEffects();
}

void AUTOnslaughtFlagBase::CreateCarriedObject()
{
	if (ControllingNode)
	{
		AUTOnslaughtPowerCore* PowerCore = Cast<AUTOnslaughtPowerCore>(ControllingNode);

		// Only spawn a flag at a base controlled by a power core
		if (PowerCore && PowerCore->FlagBase == this)
		{
			if (MyFlag == nullptr)
			{
				Super::CreateCarriedObject();

				AUTOnslaughtFlag* OnsFlag = Cast<AUTOnslaughtFlag>(MyFlag);
				if (OnsFlag && MyFlag)
				{
					MyFlag->HomeBase = this;
					OnsFlag->StartingHomeBase = this;
				}
			}
		}
		else if (MyFlag != nullptr)
		{
			// We shouldn't have a flag in this case, so destroy it
			MyFlag->Destroyed();
		}
	}
}