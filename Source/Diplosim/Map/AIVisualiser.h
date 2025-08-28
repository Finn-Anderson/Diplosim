#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Universal/DiplosimUniversalTypes.h"
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
};

UENUM()
enum class EFactionType : uint8
{
	Both,
	Different,
	Same
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAIVisualiser : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAIVisualiser();

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
		class UNiagaraComponent* TorchNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* DiseaseNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* HarvestNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		TArray<FAnimStruct> Animations;

	void MainLoop(class ACamera* Camera);

	void CalculateCitizenMovement(class ACamera* Camera);

	void CalculateAIMovement(class ACamera* Camera);

	void CalculateBuildingDeath();

	void CalculateBuildingRotation();

	void AddInstance(class AAI* AI, class UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform);

	void RemoveInstance(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	void SetInstanceTransform(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, FTransform Transform);

	void UpdateCitizenVisuals(class UHierarchicalInstancedStaticMeshComponent* HISM, class ACamera* Camera, class ACitizen* Citizen, int32 Instance);

	void ActivateTorches(int32 Hour, class UHierarchicalInstancedStaticMeshComponent* HISM = nullptr, int32 Instance = -1);

	void AddHarvestVisual(FVector Location, FLinearColor Colour);

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> GetAIHISM(class AAI* AI);

	class AAI* GetHISMAI(class ACamera* Camera, class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	FTransform GetAnimationPoint(class AAI* AI);

	void SetAnimationPoint(class AAI* AI, FTransform Transform);

	TArray<AActor*> GetOverlaps(class ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType);

	UPROPERTY()
		TArray<FPendingChangeStruct> PendingChange;

	UPROPERTY()
		float ToggleTorches;

	UPROPERTY()
		TArray<class ABuilding*> DestructingBuildings;

	UPROPERTY()
		TArray<class AResearch*> RotatingBuildings;

	FCriticalSection CitizenMovementLock;

	FCriticalSection AIMovementLock;

	FCriticalSection BuildingDeathLock;

	FCriticalSection BuildingRotationLock;
};
