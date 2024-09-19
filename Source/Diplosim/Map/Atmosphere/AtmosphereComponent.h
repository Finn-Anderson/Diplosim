#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtmosphereComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UAtmosphereComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAtmosphereComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class USkyLightComponent* SkyLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class UDirectionalLightComponent* Sun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class UDirectionalLightComponent* Moon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class USkyAtmosphereComponent* SkyAtmosphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		class UExponentialHeightFogComponent* Fog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		bool bNight;

	UPROPERTY()
		FRotator WindRotation;

	void ChangeWindDirection();

	void SetSunStatus(FString Value);

	void AddDay();
};
