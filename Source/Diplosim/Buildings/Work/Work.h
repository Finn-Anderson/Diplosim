#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
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

	UFUNCTION()
		virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Wage;

	virtual void UpkeepCost() override;

	// Citizens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMesh* WorkHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hours")
		int32 WorkStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hours")
		int32 WorkEnd;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rest")
		bool bCanRest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		bool bCanAttendEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
		bool bOpen;

	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void Enter(class ACitizen* Citizen) override;

	void Open();

	void Close();

	void CheckWorkStatus(int32 Hour);

	// Resources
	virtual void Production(class ACitizen* Citizen);

	UPROPERTY()
		TArray<class ABooster*> Boosters;

	// Forcefield
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forcefield")
		int32 ForcefieldRange;
};
