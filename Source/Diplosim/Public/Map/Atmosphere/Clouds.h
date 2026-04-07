#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Clouds.generated.h"

USTRUCT(BlueprintType)
struct FCloudStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class UInstancedStaticMeshComponent* HISMCloud;

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
		UPrimitiveComponent* Component;

	UPROPERTY()
		int32 Instance;

	UPROPERTY()
		float Increment;

	FWetnessStruct()
	{
		Value = 0.0f;
		Component = nullptr;
		Instance = INDEX_NONE;
		Increment = 0.0f;
	}

	void Create(float Val, class UPrimitiveComponent* Comp, int32 Inst, float Inc)
	{
		Value = Val;
		Component = Comp;
		Instance = Inst;
		Increment = Inc;
	}

	bool operator==(const FWetnessStruct& other) const
	{
		return (other.Component == Component && other.Instance == Instance);
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

	UFUNCTION()
		void UpdateSpawnedClouds();

	UFUNCTION(BlueprintCallable)
		void RainCollisionHandler(FVector CollisionLocation, float Value = -1.0f, float Increment = 0.0f);

	UFUNCTION()
		void SetRainMaterialEffect(float Value, UPrimitiveComponent* Component, int32 Instance);

	FCloudStruct CreateCloud(FTransform Transform, int32 Chance, bool bLoad = false, TArray<FTransform> LoadTransforms = {});

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		UStaticMesh* CloudMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraSystem* CloudSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lightning")
		class UNiagaraSystem* LightningSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	UPROPERTY()
		bool bSnow;

	UPROPERTY()
		class AGrid* Grid;

	UPROPERTY()
		TArray<FCloudStruct> Clouds;

	UPROPERTY()
		TArray<FWetnessStruct> WetnessStruct;

	UPROPERTY()
		TArray<FWetnessStruct> ProcessRainEffect;

	UPROPERTY()
		class UNaturalDisasterComponent* NaturalDisasterComponent;

	UPROPERTY()
		class UDiplosimUserSettings* Settings;

private:
	FCriticalSection RainLock;

	TArray<FVector> SetPrecipitationLocations(FTransform Transform, float Bounds);

	void SetGradualWetness(bool bLoad = false);
};
