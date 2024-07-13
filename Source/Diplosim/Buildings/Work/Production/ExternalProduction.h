#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
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

	bool operator==(const FValidResourceStruct& other) const
	{
		return (other.Resource == Resource);
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

	virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	UPROPERTY()
		TArray<FValidResourceStruct> Resources;
};
