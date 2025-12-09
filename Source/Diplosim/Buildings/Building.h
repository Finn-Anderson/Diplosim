#pragma once

#include "CoreMinimal.h"
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
struct FSocketStruct
{
	GENERATED_USTRUCT_BODY()

	FName Name;

	FVector SocketLocation;

	FRotator SocketRotation;

	class ACitizen* Citizen;

	FSocketStruct()
	{
		Name = "";
		SocketLocation = FVector(0.0f, 0.0f, 0.0f);
		Citizen = nullptr;
	}

	bool operator==(const FSocketStruct& other) const
	{
		return (other.Citizen == Citizen);
	}
};

USTRUCT(BlueprintType)
struct FSeedStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		UTexture* Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		int32 Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		int32 Capacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		bool bExplosive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		int32 Yield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		int32 TimeLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		int32 Tier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		class UNiagaraSystem* NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		class UStaticMesh* WorkHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		TArray<class UStaticMesh*> Meshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		TArray<FItemStruct> Cost;

	FSeedStruct()
	{
		Texture = nullptr;
		Health = -1;
		Capacity = -1;
		bExplosive = false;
		Name = "";
		Yield = -1;
		TimeLength = -1;
		Tier = 1;
		NiagaraSystem = nullptr;
		WorkHat = nullptr;
	}
};

USTRUCT(BlueprintType)
struct FOccupantStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Capacity")
		class ACitizen* Occupant;

	UPROPERTY(BlueprintReadOnly, Category = "Capacity")
		TArray<class ACitizen*> Visitors;

	FOccupantStruct()
	{
		Occupant = nullptr;
	}

	bool operator==(const FOccupantStruct& other) const
	{
		return (other.Occupant == Occupant);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* DecalComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		class UDecalComponent* GroundDecalComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		bool bConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		float Emissiveness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		bool bBlink;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		TArray<FLinearColor> Colours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		TArray<FSeedStruct> Seeds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		bool bAffectBuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Randomisation")
		FString BuildingName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		USoundBase* CitizenSound;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		TArray<FSocketStruct> SocketList;

	UPROPERTY()
		int32 SeedNum;

	UPROPERTY(BlueprintReadOnly, Category = "Colour")
		FLinearColor ChosenColour;

	UPROPERTY()
		int32 Tier;

	UPROPERTY(BlueprintReadOnly, Category = "Faction")
		FString FactionName;

	UFUNCTION(BlueprintCallable)
		void SetSeed(int32 Seed);

	UFUNCTION(BlueprintCallable)
		int32 GetTier();

	virtual void SetTier(int32 Value);

	UFUNCTION(BlueprintCallable)
		virtual void SetBuildingColour(float R, float G, float B);

	virtual void StoreSocketLocations();

	void SetSocketLocation(class ACitizen* Citizen);
	
	UFUNCTION(BlueprintCallable)
		virtual void DestroyBuilding(bool bCheckAbove = true, bool bMove = false);

	TArray<FItemStruct> GetGradeCost(int32 Grade);

	virtual void Enter(class ACitizen* Citizen);

	virtual void Leave(class ACitizen* Citizen);

	virtual bool AddCitizen(class ACitizen* Citizen);

	virtual bool RemoveCitizen(class ACitizen* Citizen);

	class ACitizen* GetOccupant(class ACitizen* Citizen);

	TArray<class ACitizen*> GetVisitors(class ACitizen* Occupant);

	virtual void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor);

	virtual void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor);

	TArray<class ACitizen*> GetCitizensAtBuilding();

	virtual bool CheckInstant();

	void SetLights(int32 Hour);

	void ToggleDecalComponentVisibility(bool bVisible);

	// Construct
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
		TArray<FItemStruct> CostList;

	UPROPERTY()
		TArray<FItemStruct> TargetList;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bUnique;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
		bool bCoastal;

	UPROPERTY()
		double DeathTime;

	UPROPERTY()
		UStaticMesh* ActualMesh;

	UFUNCTION(BlueprintCallable)
		TArray<FItemStruct> GetRebuildCost();

	UFUNCTION(BlueprintCallable)
		virtual void Rebuild(FString NewFactionName = "");

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0);

	void SetConstructionMesh();

	virtual void OnBuilt();

	// Upkeep
	UPROPERTY()
		bool bOperate;

	UFUNCTION(BlueprintCallable)
		void AlterOperate();

	// Citizens
	UPROPERTY(BlueprintReadOnly, Category = "Capacity")
		TArray<class ACitizen*> Inside;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		TArray<FOccupantStruct> Occupied;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TMap<FName, EAnim> AnimSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 Capacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 MaxCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 Space;

	UFUNCTION(BlueprintCallable)
		void AddCapacity();

	UFUNCTION(BlueprintCallable)
		void RemoveCapacity();

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();

	// Resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FItemStruct> Storage;

	UPROPERTY()
		TArray<FBasketStruct> Basket;

	bool CheckStored(class ACitizen* Citizen, TArray<FItemStruct> Items);

	void CarryResources(class ACitizen* Citizen, class ABuilding* DeliverTo, TArray<FItemStruct> Items);

	void StoreResource(class ACitizen* Citizen);

	void AddToBasket(TSubclassOf<class AResource> Resource, int32 Amount);

	UFUNCTION()
		void RemoveFromBasket(FGuid ID);
};
