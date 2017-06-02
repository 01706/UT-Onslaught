#pragma once

#include "UTSquadAI.h"
#include "UTOnslaughtPowerCore.h"
#include "UTOnslaughtSquadAI.generated.h"

UCLASS()
class UTONSLAUGHT_API AUTOnslaughtSquadAI : public AUTSquadAI
{
	GENERATED_BODY()
	
public:

	// The main goal, to distroy the enemey core
	UPROPERTY()
	AUTOnslaughtPowerCore* EnemeyCore;


	virtual void Initialize(AUTTeamInfo* InTeam, FName InOrders) override;

protected:

private:	
	
};