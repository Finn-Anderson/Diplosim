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

protected:
	virtual void BeginPlay() override;

public:
	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	UFUNCTION()
		void OnResourceOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class USphereComponent* RangeComponent;

	class AResource* Resource = nullptr;

	TArray<AResource*> ResourceList;
};
