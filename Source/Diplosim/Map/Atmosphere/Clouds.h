#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Clouds.generated.h"

USTRUCT()
struct FCloudStruct
{
	GENERATED_USTRUCT_BODY()

	class UNiagaraComponent* Cloud;

	double Distance;

	FCloudStruct()
	{
		Cloud = nullptr;
		Distance = 0.0f;
	}

	bool operator==(const FCloudStruct& other) const
	{
		return (other.Cloud == Cloud);
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
		class UNiagaraSystem* CloudSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		float Height;

	class UDiplosimUserSettings* Settings;

	TArray<FCloudStruct> Clouds;

	class AGrid* Grid;

	void Clear();

	UFUNCTION()
		void ActivateCloud();
};
