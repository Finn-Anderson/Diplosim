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
		int32 Cost;

	int32 Stored;

	FCostStruct()
	{
		Type = nullptr;
		Cost = 0;
		Stored = 0;
	}
};

USTRUCT()
struct FBuildStruct
{
	GENERATED_USTRUCT_BODY()

	UObject* Object;

	int32 Instance;

	FVector Location;

	FBuildStruct()
	{
		Object = nullptr;
		Instance = -1;
		Location = FVector::Zero();
	}

	bool operator==(const FBuildStruct& other) const
	{
		return (other.Object == Object) && (other.Instance == Instance) && (other.Location == Location);
	}
};

UENUM()
enum class EBuildStatus : uint8 
{
	Blueprint,
	Construction,
	Complete
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interface")
		class UInteractableComponent* InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* ParticleComponent;

	EBuildStatus BuildStatus;

	TArray<FBuildStruct> Collisions;

	UPROPERTY()
		class ACamera* Camera;

	FVector CheckCollisions(class UObject* Object, int32 Index);

	void DestroyBuilding();

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual void FindCitizens();

	virtual void AddCitizen(class ACitizen* Citizen);

	virtual void RemoveCitizen(class ACitizen* Citizen);

	virtual bool CheckInstant();

	// Construct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		TArray<FCostStruct> CostList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		class UStaticMesh* ConstructionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bInstantConstruction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen")
		bool bHideCitizen;

	bool bMoved;

	TArray<class AActor*> TreeList;

	int32 BuildPercentage;

	FTimerHandle ConstructTimer;

	UStaticMesh* ActualMesh;

	void Build();

	bool CheckBuildCost();

	UFUNCTION(BlueprintCallable)
		TArray<FCostStruct> GetCosts();

	void OnBuilt();

	void CheckGatherSites(class ACitizen* Citizen, struct FCostStruct Stock);

	void AddBuildPercentage(class ACitizen* Citizen);

	// Upkeep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		TSubclassOf<class AResource> Money;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Upkeep;

	virtual void UpkeepCost(int32 Cost = 0);

	// Building prevent overlaps
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Citizens
	UPROPERTY()
		TArray<class ACitizen*> Occupied;

	FTimerHandle CostTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 Capacity;

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();

	// Resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;
};
