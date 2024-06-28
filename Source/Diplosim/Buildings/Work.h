#pragma once

#include "CoreMinimal.h"
#include "Building.h"
#include "Work.generated.h"

UCLASS()
class DIPLOSIM_API AWork : public ABuilding
{
	GENERATED_BODY()
	
public:
	AWork();

protected:
	virtual void BeginPlay() override;

public:
	// Range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class USphereComponent* SphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		bool bRange;

	UFUNCTION()
		virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TArray<class AHouse*> Houses;

	// Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Wage;

	virtual void UpkeepCost() override;

	// Citizens
	FTimerHandle FindTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMesh* WorkHat;

	virtual void FindCitizens() override;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	// Resources
	FTimerHandle ProdTimer;

	virtual void Production(class ACitizen* Citizen);
};
