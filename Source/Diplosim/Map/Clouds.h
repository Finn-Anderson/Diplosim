#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Clouds.generated.h"

USTRUCT()
struct FCloudStruct
{
	GENERATED_USTRUCT_BODY()

	class UNiagaraComponent* CloudComponent;

	int32 Speed;

	FCloudStruct() {
		CloudComponent = nullptr;

		Speed = 0;
	}

	bool operator==(const FCloudStruct& other) const
	{
		return (other.CloudComponent == CloudComponent) && (other.Speed == Speed);
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

	TArray<FCloudStruct> Clouds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraSystem* CloudSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
		class UNiagaraComponent* CloudComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	float Speed;

	float X;
	float Y;

	void GetCloudBounds(class AGrid* Grid);

	void ResetCloud();
};
