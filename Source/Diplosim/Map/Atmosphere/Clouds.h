#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
};
