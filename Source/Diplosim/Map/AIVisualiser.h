#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "AIVisualiser.generated.h"

USTRUCT()
struct FPendingChangeStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class AAI* AI;

	UPROPERTY()
		class UHierarchicalInstancedStaticMeshComponent* HISM;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		int32 Instance;

	FPendingChangeStruct()
	{
		AI = nullptr;
		HISM = nullptr;
		Transform = FTransform();
		Instance = -1;
	}
};

USTRUCT()
struct FOverlapsStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		bool bCitizens;

	UPROPERTY()
		bool bRebels;

	UPROPERTY()
		bool bEnemies;

	UPROPERTY()
		bool bClones;

	UPROPERTY()
		bool bBuildings;

	UPROPERTY()
		bool bResources;

	UPROPERTY()
		bool bGettingCitizensEnemies;

	FOverlapsStruct()
	{
		bCitizens = false;
		bRebels = false;
		bEnemies = false;
		bClones = false;
		bBuildings = false;
		bResources = false;
		bGettingCitizensEnemies = false;
	}

	void GetCitizenInteractions(bool bIncludeBuildingsAndResources, bool bIncludeRebels)
	{
		bCitizens = true;
		bBuildings = bIncludeBuildingsAndResources;
		bResources = bIncludeBuildingsAndResources;
		bRebels = bIncludeRebels;
	}

	void GetCitizenEnemies()
	{
		GetEverythingWithHealth();
		bGettingCitizensEnemies = true;
	}

	bool IsGettingCitizenEnemies()
	{
		return bGettingCitizensEnemies;
	}

	void GetRebelsEnemies()
	{
		bCitizens = true;
		bEnemies = true;
		bClones = true;
	}

	void GetEnemyEnemies()
	{
		bCitizens = true;
		bRebels = true;
		bClones = true;
	}

	void GetEverythingWithHealth()
	{
		bCitizens = true;
		bRebels = true;
		bEnemies = true;
		bClones = true;
		bBuildings = true;
	}

	void GetEverything()
	{
		GetEverythingWithHealth();
		bResources = true;
	}
};

USTRUCT(BlueprintType)
struct FHatsStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hats")
		class UHierarchicalInstancedStaticMeshComponent* HISMHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hats")
		TArray<ACitizen*> Citizens;

	FHatsStruct()
	{
		HISMHat = nullptr;
	}

	bool operator==(const FHatsStruct& other) const
	{
		return (other.HISMHat == HISMHat);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAIVisualiser : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAIVisualiser();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class USceneComponent* AIContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMCitizen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMClone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMRebel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMEnemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* HarvestNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		TArray<FAnimStruct> Animations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
		TMap<FString, FLinearColor> HarvestVisuals;

	void ResetToDefaultValues();

	void MainLoop(class ACamera* Camera);

	void AddInstance(class AAI* AI, class UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform);

	void RemoveInstance(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	void UpdateInstanceCustomData(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, int32 Index, float Value);

	void SetHarvestVisuals(class ACitizen* Citizen, class AResource* Resource);

	FVector AddHarvestVisual(class AAI* AI, FLinearColor Colour);

	void SetEyesVisuals(class ACitizen* Citizen, int32 HappinessValue);

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> GetAIHISM(class AAI* AI);

	class AAI* GetHISMAI(class ACamera* Camera, class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	FTransform GetAnimationPoint(class AAI* AI);

	void SetAnimationPoint(class AAI* AI, FTransform Transform);

	TArray<AActor*> GetOverlaps(class ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType, FFactionStruct* Faction = nullptr, FVector Location = FVector::Zero());

	UPROPERTY()
		TArray<FPendingChangeStruct> PendingChange;

	UPROPERTY()
		TArray<class ABuilding*> DestructingBuildings;

	UPROPERTY()
		TArray<class AResearch*> RotatingBuildings;

	// Hats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class USceneComponent* HatsContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hats")
		TArray<FHatsStruct> HISMHats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hats")
		TMap<UStaticMesh*, int32> HatsMeshesList;

	void AddCitizenToHISMHat(ACitizen* Citizen, UStaticMesh* HatMesh);

	void RemoveCitizenFromHISMHat(ACitizen* Citizen);

	bool DoesCitizenHaveHat(ACitizen* Citizen);

	void ToggleOfficerLights(ACitizen* Citizen, float Value);

	UPROPERTY()
		int32 hatsNum;

private:
	void CalculateCitizenMovement(class ACamera* Camera);

	void CalculateAIMovement(class ACamera* Camera);

	void CalculateBuildingDeath(ACamera* Camera);

	void CalculateBuildingRotation(ACamera* Camera);

	void SetInstanceTransform(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, FTransform Transform);

	void UpdateCitizenVisuals(class UHierarchicalInstancedStaticMeshComponent* HISM, class ACamera* Camera, class ACitizen* Citizen, int32 Instance);

	void ActivateTorch(int32 Hour, class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	void UpdateArmyVisuals(class ACamera* Camera, class ACitizen* Citizen);

	FTransform GetHatTransform(ACitizen* Citizen);

	void UpdateHatsTransforms(ACamera* Camera);

	FCriticalSection CitizenMovementLock;
	FCriticalSection CitizenMovementLock1;
	FCriticalSection CitizenMovementLock2;

	FCriticalSection AIMovementLock;

	FCriticalSection BuildingDeathLock;

	FCriticalSection BuildingRotationLock;

	FCriticalSection HatLock;
};
