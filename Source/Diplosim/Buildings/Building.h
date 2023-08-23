#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

USTRUCT(BlueprintType)
struct FCostStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Stored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Cost;

	FCostStruct()
	{
		Type = nullptr;
		Cost = 0;
		Stored = 0;
	}
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nav")
		class UBoxComponent* BoxCollision;

	bool Blueprint;

	bool bMoved;

	TArray<UObject*> Blocking;

	UPROPERTY()
		class ACamera* Camera;

	void DestroyBuilding();

	void OnBuilt();

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual void FindCitizens();

	virtual void AddCitizen(class ACitizen* Citizen);

	virtual void RemoveCitizen(class ACitizen* Citizen);

public:
	// Construct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		TArray<FCostStruct> CostList;

	TArray<class AActor*> TreeList;

	void Build();

	bool CheckBuildCost();

	UFUNCTION(BlueprintCallable)
		TArray<FCostStruct> GetCosts();

public:
	// Upkeep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		TSubclassOf<class AResource> Money;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 Capacity;

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();
};
