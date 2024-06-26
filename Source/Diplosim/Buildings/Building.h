#pragma once

#include "CoreMinimal.h"
#include "AI/Citizen.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

USTRUCT(BlueprintType)
struct FItemStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Amount;

	int32 Stored;

	FItemStruct()
	{
		Resource = nullptr;
		Amount = 0;
		Stored = 0;
	}
};

USTRUCT(BlueprintType)
struct FQueueStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		class UWidget* OrderWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		bool bCancelled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Wait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		bool bRepeat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FItemStruct> Items;

	FQueueStruct()
	{
		OrderWidget = nullptr;
		bCancelled = false;
		Wait = 0;
		bRepeat = false;
	}
};

USTRUCT()
struct FCollisionStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	class UHierarchicalInstancedStaticMeshComponent* HISM;

	int32 Instance;

	FCollisionStruct()
	{
		Actor = nullptr;
		HISM = nullptr;
		Instance = -1;
	}

	bool operator==(const FCollisionStruct& other) const
	{
		return (other.Actor == Actor) && (other.HISM == HISM) && (other.Instance == Instance);
	}
};

USTRUCT()
struct FTreeStruct
{
	GENERATED_USTRUCT_BODY()

	class AVegetation* Resource;

	int32 Instance;

	FTreeStruct()
	{
		Resource = nullptr;
		Instance = -1;
	}

	bool operator==(const FTreeStruct& other) const
	{
		return (other.Resource == Resource) && (other.Instance == Instance);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UMaterial* DamagedMaterialOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* ParticleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		bool bConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float Emissiveness;

	UPROPERTY()
		class ACamera* Camera;

	TArray<FCollisionStruct> Collisions;
	
	UFUNCTION(BlueprintCallable)
		void DestroyBuilding();

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual void FindCitizens();

	virtual bool AddCitizen(class ACitizen* Citizen);

	virtual bool RemoveCitizen(class ACitizen* Citizen);

	TArray<class ACitizen*> GetCitizensAtBuilding();

	virtual bool CheckInstant();

	// Construct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		FQueueStruct CostList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		class UStaticMesh* ConstructionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bInstantConstruction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bCanMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bCanDestroy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen")
		bool bHideCitizen;

	TArray<FTreeStruct> TreeList;

	UStaticMesh* ActualMesh;

	void Build();

	void OnBuilt();

	// Upkeep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		TSubclassOf<class AResource> Money;

	virtual void UpkeepCost();

	// Building prevent overlaps
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Citizens
	UPROPERTY(BlueprintReadOnly, Category = "Capacity")
		TArray<class ACitizen*> Occupied;

	FTimerHandle CostTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 Capacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 MaxCapacity;

	UFUNCTION(BlueprintCallable)
		void AddCapacity();

	UFUNCTION(BlueprintCallable)
		void RemoveCapacity();

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();

	// Resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	UPROPERTY(BlueprintReadOnly, Category = "Resource")
		TArray<FQueueStruct> Orders;

	bool CheckStored(ACitizen* Citizen, TArray<FItemStruct> Items);

	void CarryResources(ACitizen* Citizen, ABuilding* DeliverTo, TArray<FItemStruct> Items);

	void StoreResource(class ACitizen* Citizen);

	void ReturnResource(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void SetNewOrder(FQueueStruct Order);

	UFUNCTION(BlueprintCallable)
		void SetOrderWidget(int32 index, class UWidget* Widget);

	UFUNCTION(BlueprintCallable)
	void SetOrderCancelled(int32 index, bool bCancel);

	// Politics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FPartyStruct> Swing;

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		FReligionStruct Belief;
};
