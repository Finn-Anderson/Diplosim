#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "ExternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AExternalProduction : public AWork
{
	GENERATED_BODY()

public:
	AExternalProduction();

public:
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void FindCitizens() override;

	virtual void RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;

	class AResource* Resource;

	int32 Instance;

	TArray<int32> InstanceList;
};
