#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

UCLASS()
class DIPLOSIM_API ABuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	ABuilding();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* BuildingMesh;

	bool Blueprint;

	UPROPERTY()
		class ACamera* Camera;

	void Run();

	void DestroyBuilding();

	virtual void OnBuilt();

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual void FindCitizens();

	virtual void AddCitizen(class ACitizen* Citizen);

	virtual void RemoveCitizen(class ACitizen* Citizen);

public:
	// Building cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Money;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		int32 R1Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		int32 R2Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		int32 MoneyCost;

	bool BuildCost();

	UFUNCTION(BlueprintCallable)
		FText GetR1();

	UFUNCTION(BlueprintCallable)
		FText GetR2();

	UFUNCTION(BlueprintCallable)
		FText GetMoney();

public:
	// Upkeep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Upkeep;

	virtual void UpkeepCost();

public:
	// Building prevent overlaps
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	// Citizens
	UPROPERTY()
		TArray<class ACitizen*> Occupied;

	UPROPERTY()
		TArray<class ACitizen*> AtWork;

	FTimerHandle CostTimer;

	int32 Capacity;

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();
};
