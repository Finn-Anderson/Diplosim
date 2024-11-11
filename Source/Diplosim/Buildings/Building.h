#pragma once

#include "CoreMinimal.h"
#include "Universal/Resource.h"
#include "Universal/DiplosimUniversalTypes.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Use;

	FItemStruct()
	{
		Resource = nullptr;
		Amount = 0;
		Stored = 0;
		Use = 0;
	}

	bool operator==(const FItemStruct& other) const
	{
		return (other.Resource == Resource);
	}
};

USTRUCT()
struct FBasketStruct
{
	GENERATED_USTRUCT_BODY()

	FGuid ID;

	FItemStruct Item;

	FBasketStruct()
	{
		
	}

	bool operator==(const FBasketStruct& other) const
	{
		return (other.ID == ID);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* ParticleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* DestructionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* AmbientAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		bool bConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float Emissiveness;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		TArray<FCollisionStruct> Collisions;
	
	UFUNCTION(BlueprintCallable)
		void DestroyBuilding();

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual bool AddCitizen(class ACitizen* Citizen);

	virtual bool RemoveCitizen(class ACitizen* Citizen);

	TArray<class ACitizen*> GetCitizensAtBuilding();

	virtual bool CheckInstant();

	// Construct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		TArray<FItemStruct> CostList;

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

	UPROPERTY()
		TArray<FTreeStruct> TreeList;

	UPROPERTY()
		UStaticMesh* ActualMesh;

	void Build();

	virtual void OnBuilt();

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
		TArray<class ACitizen*> Inside;

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
		TArray<FItemStruct> Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	UPROPERTY()
		TArray<FBasketStruct> Basket;

	bool CheckStored(class ACitizen* Citizen, TArray<FItemStruct> Items);

	void CarryResources(class ACitizen* Citizen, class ABuilding* DeliverTo, TArray<FItemStruct> Items);

	void StoreResource(class ACitizen* Citizen);

	void AddToBasket(TSubclassOf<class AResource> Resource, int32 Amount);

	void RemoveFromBasket(FGuid ID);

	// Politics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FPartyStruct> Swing;

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EReligion Belief;
};
