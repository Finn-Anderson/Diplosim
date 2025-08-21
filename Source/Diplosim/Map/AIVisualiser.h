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
		class UNiagaraComponent* TorchNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* DiseaseNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* HarvestNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
		TArray<FAnimStruct> Animations;

	void MainLoop(class ACamera* Camera);

	void AddInstance(class AAI* AI, class UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform);

	void RemoveInstance(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	void SetInstanceTransform(class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, FTransform Transform);

	void UpdateCitizenVisuals(class ACamera* Camera, class ACitizen* Citizen, int32 Instance);

	void ActivateTorches(int32 Hour, class UHierarchicalInstancedStaticMeshComponent* HISM = nullptr, int32 Instance = -1);

	void AddHarvestVisual(FVector Location, FLinearColor Colour);

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> GetAIHISM(class AAI* AI);

	class AAI* GetHISMAI(class ACamera* Camera, class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	FTransform GetAnimationPoint(class AAI* AI);

	void SetAnimationPoint(class AAI* AI, FTransform Transform);

	TArray<AActor*> GetOverlaps(class ACamera* Camera, AActor* Actor, float Range);

	UPROPERTY()
		TArray<FPendingChangeStruct> PendingChange;

	UPROPERTY()
		float ToggleTorches;

	FCriticalSection MovementLock;
};
