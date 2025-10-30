#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Clouds.generated.h"

USTRUCT(BlueprintType)
struct FCloudStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class UHierarchicalInstancedStaticMeshComponent* HISMCloud;

	UPROPERTY()
		class UNiagaraComponent* Precipitation;

	UPROPERTY()
		double Distance;

	UPROPERTY()
		bool bHide;

	UPROPERTY()
		float lightningTimer;

	UPROPERTY()
		float lightningFrequency;

	FCloudStruct()
	{
		HISMCloud = nullptr;
		Precipitation = nullptr;
		Distance = 0.0f;
		bHide = false;
		lightningTimer = 0.0f;
		lightningFrequency = 0.0f;
	}

	bool operator==(const FCloudStruct& other) const
	{
		return (other.HISMCloud == HISMCloud);
	}
};

USTRUCT()
struct FWetnessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		float Value;

	UPROPERTY()
		AActor* Actor;

	UPROPERTY()
		class UHierarchicalInstancedStaticMeshComponent* HISM;

	UPROPERTY()
		int32 Instance;

	UPROPERTY()
		float Increment;

	FWetnessStruct()
	{
		Value = 0.0f;
		Actor = nullptr;
		HISM = nullptr;
		Instance = -1;
		Increment = 0.0f;
	}

	void Create(float V, AActor* A, class UHierarchicalInstancedStaticMeshComponent* H, int32 Inst, float Inc)
	{
		Value = V;
		Actor = A;
		HISM = H;
		Instance = Inst;
		Increment = Inc;
	}

	bool operator==(const FWetnessStruct& other) const
	{
		return (other.Actor == Actor) || (other.HISM == HISM && other.Instance == Instance);
	}
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOSIM_API UCloudComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UCloudComponent();

protected:
	virtual void BeginPlay() override;

public:
	void TickCloud(float DeltaTime);

	void Clear();

	UFUNCTION()
		void ActivateCloud();

	FCloudStruct CreateCloud(FTransform Transform, int32 Chance, bool bLoad = false);

	TArray<FVector> SetPrecipitationLocations(FTransform Transform, float Bounds);

	UFUNCTION()
		void UpdateSpawnedClouds();

	UFUNCTION(BlueprintCallable)
		void RainCollisionHandler(FVector CollisionLocation, float Value = -1.0f, float Increment = 0.0f);

	UFUNCTION()
		void SetRainMaterialEffect(float Value, class AActor* Actor, class UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance);

	void SetGradualWetness(bool bLoad = false);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		UStaticMesh* CloudMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraSystem* CloudSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
		class UNiagaraSystem* LightningSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	UPROPERTY()
		class UDiplosimUserSettings* Settings;

	UPROPERTY()
		TArray<FCloudStruct> Clouds;

	UPROPERTY()
		bool bSnow;

	UPROPERTY()
		TArray<FWetnessStruct> WetnessStruct;

	UPROPERTY()
		class AGrid* Grid;

	UPROPERTY()
		class UNaturalDisasterComponent* NaturalDisasterComponent;

	UPROPERTY()
		TArray<FWetnessStruct> ProcessRainEffect;

	FCriticalSection RainLock;
};
