#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "AIVisualiser.generated.h"

USTRUCT()
struct FHatsToUpdateStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class UAIInstancedStaticMeshComponent* ISM;

	UPROPERTY()
		TMap<int32, FTransform> InstanceTransformsToUpdate;

	UPROPERTY()
		TArray<int32> InstanceDataToUpdate;

	FHatsToUpdateStruct()
	{
		ISM = nullptr;
	}

	bool operator==(const FHatsToUpdateStruct& other) const
	{
		return (other.ISM == ISM);
	}
};

USTRUCT()
struct FPendingChangeStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class AAI* AI;

	UPROPERTY()
		class UAIInstancedStaticMeshComponent* ISM;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		TArray<int32> Instances;

	FPendingChangeStruct()
	{
		AI = nullptr;
		ISM = nullptr;
		Transform = FTransform();
	}

	bool operator==(const FPendingChangeStruct& other) const
	{
		return (other.ISM == ISM && other.AI == AI);
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

	UPROPERTY()
		bool bUnbuiltBuildings;

	FOverlapsStruct()
	{
		bCitizens = false;
		bRebels = false;
		bEnemies = false;
		bClones = false;
		bBuildings = false;
		bResources = false;
		bGettingCitizensEnemies = false;
		bUnbuiltBuildings = false;
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
		bBuildings = true;
		bCitizens = true;
		bEnemies = true;
		bClones = true;
	}

	void GetEnemyEnemies()
	{
		bBuildings = true;
		bCitizens = true;
		bRebels = true;
		bClones = true;
	}

	void GetEveryPawn()
	{
		bCitizens = true;
		bRebels = true;
		bEnemies = true;
		bClones = true;
	}

	void GetEverythingWithHealth()
	{
		GetEveryPawn();
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
		class UAIInstancedStaticMeshComponent* ISMHat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hats")
		TArray<ACitizen*> Citizens;

	FHatsStruct()
	{
		ISMHat = nullptr;
	}

	bool operator==(const FHatsStruct& other) const
	{
		return (other.ISMHat == ISMHat);
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
		class UAIInstancedStaticMeshComponent* HISMCitizen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UAIInstancedStaticMeshComponent* HISMClone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UAIInstancedStaticMeshComponent* HISMRebel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UAIInstancedStaticMeshComponent* HISMEnemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UAIInstancedStaticMeshComponent* HISMSnake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* HarvestNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		TArray<FAnimStruct> Animations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
		TMap<FString, FLinearColor> HarvestVisuals;

	void ResetToDefaultValues();

	void MainLoop(class ACamera* Camera, float DeltaTime);

	void AddInstance(class AAI* AI, class UAIInstancedStaticMeshComponent* ISM, FTransform Transform);

	void RemoveInstance(class UAIInstancedStaticMeshComponent* ISM, int32 Instance);

	void SetHarvestVisuals(class ACitizen* Citizen, class AResource* Resource);

	TTuple<class UAIInstancedStaticMeshComponent*, int32> GetAIHISM(class AAI* AI);

	class AAI* GetHISMAI(class ACamera* Camera, class UAIInstancedStaticMeshComponent* ISM, int32 Instance);

	FTransform GetAnimationPoint(class AAI* AI);

	void SetAnimationPoint(class AAI* AI, FTransform Transform, TArray<int32>& Instances);

	TArray<AActor*> GetOverlaps(class ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType, FFactionStruct* Faction = nullptr, FVector Location = FVector::Zero());

	UPROPERTY()
		TArray<FPendingChangeStruct> PendingChange;

	UPROPERTY()
		TMap<class AActor*, double> DestructingActors;

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

private:
	void CalculateCitizenMovement(class ACamera* Camera);

	void CalculateAIMovement(class ACamera* Camera);

	void CalculateBuildingDeath(ACamera* Camera);

	void CalculateBuildingRotation(ACamera* Camera);

	void UpdateInstanceCustomData(class UAIInstancedStaticMeshComponent* ISM, int32 Instance, int32 Index, float Value, TArray<int32>& Instances);

	void SetAIColour(class UAIInstancedStaticMeshComponent* ISM, int32 Instance, FLinearColor Colour, TArray<int32>& Instances);

	void SetInstanceTransform(class UAIInstancedStaticMeshComponent* ISM, int32 Instance, FTransform Transform, TMap<int32, FTransform>& InstanceTransformsToUpdate);

	void UpdateCitizenVisuals(class UAIInstancedStaticMeshComponent* ISM, class ACamera* Camera, class ACitizen* Citizen, int32 Instance, TArray<int32>& Instances);

	void ActivateTorch(class ACamera* Camera, class UAIInstancedStaticMeshComponent* ISM, int32 Instance, TArray<int32>& Instances);

	FVector AddHarvestVisual(class AAI* AI, FLinearColor Colour);

	void SetEyesVisuals(class UAIInstancedStaticMeshComponent* ISM, int32 Instance, class ACitizen* Citizen, TArray<int32>& Instances);

	FTransform GetHatTransform(ACitizen* Citizen);

	void UpdateHatTransform(class ACitizen* Citizen, TArray<FHatsToUpdateStruct>& HatsToUpdate);

	FHatsStruct* GetCitizenHat(ACitizen* Citizen);

	FCriticalSection CitizenMovementLock;
	int32 MaxCounter;
	int32 Counter;

	FCriticalSection AIMovementLock;

	FCriticalSection BuildingDeathLock;

	FCriticalSection BuildingRotationLock;

	float HarvestVisualCooldownTimer;
};
