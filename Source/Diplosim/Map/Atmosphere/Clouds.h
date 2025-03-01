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

	FCloudStruct()
	{
		HISMCloud = nullptr;
		Precipitation = nullptr;
		Distance = 0.0f;
		bHide = false;
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
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Clear();

	UFUNCTION()
		void ActivateCloud();

	FCloudStruct CreateCloud(FTransform Transform, int32 Chance);

	TArray<FVector> SetPrecipitationLocations(FTransform Transform, float Bounds);

	UFUNCTION()
		void UpdateSpawnedClouds();

	UFUNCTION(BlueprintCallable)
		void RainCollisionHandler(FVector CollisionLocation);

	void SetRainMaterialEffect(float Value, class AActor* Actor, class UHierarchicalInstancedStaticMeshComponent* HISM = nullptr, int32 Instance = -1);

	void SetGradualWetness();

	void SetMaterialWetness(UMaterialInterface* MaterialInterface, float Value, UStaticMeshComponent* StaticMesh, USkeletalMeshComponent* SkeletalMesh);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		UStaticMesh* CloudMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraSystem* CloudSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	UPROPERTY()
		class UDiplosimUserSettings* Settings;

	UPROPERTY()
		TArray<FCloudStruct> Clouds;

	UPROPERTY()
		FTimerHandle CloudTimer;

	UPROPERTY()
		bool bSnow;

	UPROPERTY()
		FTimerHandle WetnessTimer;

	UPROPERTY()
		TArray<FWetnessStruct> WetnessStruct;
};
