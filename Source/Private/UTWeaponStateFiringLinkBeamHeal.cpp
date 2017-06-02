
#include "UTOnslaught.h"

#include "UTWeap_LinkGun.h"
#include "UTHealableInterface.h"
#include "UTWeaponStateFiringLinkBeamHeal.h"
#include "Animation/AnimInstance.h"


UUTWeaponStateFiringLinkBeamHeal::UUTWeaponStateFiringLinkBeamHeal(const FObjectInitializer& OI)
	: Super(OI)
{
}

// Alt fire
void UUTWeaponStateFiringLinkBeamHeal::FireShot()
{
	// [possibly] consume ammo but don't fire from here
	AUTWeap_LinkGun* LinkGun = Cast<AUTWeap_LinkGun>(GetOuterAUTWeapon());
	
	if (LinkGun != NULL)
	{
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Fire shot TTTTT"));
	}
	Super::FireShot();
}

void UUTWeaponStateFiringLinkBeamHeal::Tick(float DeltaTime)
{
	AUTWeap_LinkGun* LinkGun = Cast<AUTWeap_LinkGun>(GetOuterAUTWeapon());
	if (LinkGun && (LinkGun->Role == ROLE_Authority))
	{
		LinkGun->bLinkCausingDamage = false;
	}
	if (bPendingEndFire) // player is no longer pressing alt fire
	{
		if (LinkGun && LinkGun->IsLinkPulsing()) //
		{
			// Happens when the pull has been done and the player is flying towards us
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Pulsing"));
			LinkGun->GetUTOwner()->SetFlashLocation(LinkGun->PulseLoc, LinkGun->GetCurrentFireMode());

			return;
		}
		else if (LinkGun && LinkGun->bReadyToPull && LinkGun->CurrentLinkedTarget) // Can we pull the target towards us?
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("Starting PULL!"));

			// Dont link pull a player which is healable
			AUTCharacter* PlayerTarget = Cast<AUTCharacter>(LinkGun->CurrentLinkedTarget);
			IUTHealableInterface* HealableActor = Cast<IUTHealableInterface>(LinkGun->CurrentLinkedTarget);
			// TODO: Change IsHealable to IsHealableBy(ACharacter, DamageCouser)
			if (PlayerTarget && HealableActor && HealableActor->IsHealable(LinkGun->GetUTOwner(), LinkGun))
			{
				LinkGun->bReadyToPull = false;
			}
			else
			{
				LinkGun->StartLinkPull();
			}

			return;
		}

		if (bPendingStartFire) // Player has started pressing alt fire again
		{
			//UE_LOG(LogUTOnslaught, Warning, TEXT("alt fire again"));

			bPendingEndFire = false;
		}
		else
		{
			EndFiringSequence(1);
			return;
		}
	}
	bPendingStartFire = false;
	HandleDelayedShot();

	if (LinkGun && !LinkGun->FireShotOverride() && LinkGun->InstantHitInfo.IsValidIndex(LinkGun->GetCurrentFireMode()))
	{
		const FInstantHitDamageInfo& DamageInfo = LinkGun->InstantHitInfo[LinkGun->GetCurrentFireMode()]; //Get and store reference to DamageInfo, Damage = 34, Momentum = -100000, TraceRange = 1800
		FHitResult Hit;
		FName RealShotsStatsName = LinkGun->ShotsStatsName;
		LinkGun->ShotsStatsName = NAME_None;
		FName RealHitsStatsName = LinkGun->HitsStatsName;
		LinkGun->HitsStatsName = NAME_None;
		//UE_LOG(LogUTOnslaught, Warning, TEXT("Normal shot"));
		LinkGun->FireInstantHit(false, &Hit);  // Dont deal damage, find the target we hit, return the Hit result to 'Hit'
		LinkGun->ShotsStatsName = RealShotsStatsName;
		LinkGun->HitsStatsName = RealHitsStatsName;

		AccumulatedFiringTime += DeltaTime;
		float RefireTime = LinkGun->GetRefireTime(LinkGun->GetCurrentFireMode());
		AUTPlayerState* PS = (LinkGun->Role == ROLE_Authority) && LinkGun->GetUTOwner() && LinkGun->GetUTOwner()->Controller ? Cast<AUTPlayerState>(LinkGun->GetUTOwner()->Controller->PlayerState) : NULL;
		LinkGun->bLinkBeamImpacting = (Hit.Time < 1.f);
		AActor* OldLinkedTarget = LinkGun->CurrentLinkedTarget;
		LinkGun->CurrentLinkedTarget = nullptr;
		if (Hit.Actor.IsValid() && Hit.Actor.Get()->bCanBeDamaged)
		{
			if (LinkGun->IsValidLinkTarget(Hit.Actor.Get()))
			{
				LinkGun->CurrentLinkedTarget = Hit.Actor.Get();
			}
			if (LinkGun->Role == ROLE_Authority)
			{
				LinkGun->bLinkCausingDamage = true;
			}

			float LinkedDamage = float(DamageInfo.Damage);
			Accumulator += LinkedDamage / RefireTime * DeltaTime;
			if (PS && (LinkGun->ShotsStatsName != NAME_None) && (AccumulatedFiringTime > RefireTime))
			{
				AccumulatedFiringTime -= RefireTime;
				PS->ModifyStatsValue(LinkGun->ShotsStatsName, 1);
			}

			if (Accumulator >= MinDamage)
			{
				int32 AppliedDamage = FMath::TruncToInt(Accumulator);
				Accumulator -= AppliedDamage;
				FVector FireDir = (Hit.Location - Hit.TraceStart).GetSafeNormal();
				AController* LinkDamageInstigator = LinkGun->GetUTOwner() ? LinkGun->GetUTOwner()->Controller : nullptr;
				//UE_LOG(LogUTOnslaught, Warning, TEXT("Alt fire - first or reload shot damage: %s"), *Hit.Actor->GetName());

				// Check to see if we can heal this actor
				IUTHealableInterface* HealableActor = Cast<IUTHealableInterface>(Hit.Actor.Get());				

				if (HealableActor && HealableActor->IsHealable(LinkDamageInstigator->GetCharacter(), LinkGun))
				{
					HealableActor->HealDamage(AppliedDamage, FUTPointDamageEvent(AppliedDamage, Hit, FireDir, DamageInfo.DamageType, FireDir * (LinkGun->GetImpartedMomentumMag(Hit.Actor.Get()) * float(AppliedDamage) / float(DamageInfo.Damage))), LinkDamageInstigator, LinkGun);
				}
				else
				{
					Hit.Actor->TakeDamage(AppliedDamage, FUTPointDamageEvent(AppliedDamage, Hit, FireDir, DamageInfo.DamageType, FireDir * (LinkGun->GetImpartedMomentumMag(Hit.Actor.Get()) * float(AppliedDamage) / float(DamageInfo.Damage))), LinkDamageInstigator, LinkGun);
				}

				if (PS && (LinkGun->HitsStatsName != NAME_None))
				{
					PS->ModifyStatsValue(LinkGun->HitsStatsName, AppliedDamage / FMath::Max(LinkedDamage, 1.f));
				}
			}
		}
		else
		{
			if (PS && (LinkGun->ShotsStatsName != NAME_None) && (AccumulatedFiringTime > RefireTime))
			{
				AccumulatedFiringTime -= RefireTime;
				PS->ModifyStatsValue(LinkGun->ShotsStatsName, 1);
			}
		}
		if (OldLinkedTarget != LinkGun->CurrentLinkedTarget)
		{
			LinkGun->LinkStartTime = GetWorld()->GetTimeSeconds();
			LinkGun->bReadyToPull = false;
		}
		else if (LinkGun->CurrentLinkedTarget && !LinkGun->IsLinkPulsing())
		{
			LinkGun->bReadyToPull = (GetWorld()->GetTimeSeconds() - LinkGun->LinkStartTime > LinkGun->PullWarmupTime);
		}
		// beams show a clientside beam target
		if (LinkGun->Role < ROLE_Authority && LinkGun->GetUTOwner() != NULL) // might have lost owner due to TakeDamage() call above!
		{
			LinkGun->GetUTOwner()->SetFlashLocation(Hit.Location, LinkGun->GetCurrentFireMode());
		}
	}
}