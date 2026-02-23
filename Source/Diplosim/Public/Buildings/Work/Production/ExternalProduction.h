#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public AWork
{
	GENERATED_BODY()

public:
	AExternalProduction();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual bool IsAtWork(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost")
		int32 Range;

	TMap<AResource*, TArray<int32>> GetValidResources();
};
