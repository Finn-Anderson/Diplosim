#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "ExternalProduction.generated.h"

USTRUCT()
struct FValidResourceStruct
{
	GENERATED_USTRUCT_BODY()

	class AResource* Resource;

	TArray<int32> Instances;

	FValidResourceStruct()
	{
		Resource = nullptr;
		Instances = {};
	}
};

UCLASS()
class DIPLOSIM_API AExternalProduction : public AWork
{
	GENERATED_BODY()

public:
	AExternalProduction();

public:
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void FindCitizens() override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	TArray<FValidResourceStruct> ValidResourceList;
};
