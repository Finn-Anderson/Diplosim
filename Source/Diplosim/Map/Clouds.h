#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Clouds.generated.h"

USTRUCT(BlueprintType)
struct FCloudStruct
{
	GENERATED_USTRUCT_BODY()

	class UNiagaraComponent* CloudComponent;

	bool Active;

	FCloudStruct() {
		CloudComponent = nullptr;

		Active = false;
	}

	bool operator==(const FCloudStruct& other) const
	{
		return (other.CloudComponent == CloudComponent) && (other.Active == Active);
	}
};

UCLASS()
class DIPLOSIM_API AClouds : public AActor
{
	GENERATED_BODY()
	
public:	
	AClouds();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent5;

	TArray<FCloudStruct> Clouds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	FTimerHandle CloudTimer;

	float XY;

	void GetCloudBounds(int32 GridSize);

	UFUNCTION()
		void ActivateCloud();
};
