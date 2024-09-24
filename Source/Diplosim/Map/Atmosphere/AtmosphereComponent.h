#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtmosphereComponent.generated.h"

USTRUCT(BlueprintType)
struct FCalendarStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
		FString Period;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
		TArray<int32> Days;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
		int32 Index;

	UPROPERTY(BlueprintReadOnly, Category = "Calendar")
		int32 Hour;

	FCalendarStruct()
	{
		Period = "Warmth";
		Days = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6 };
		Index = 0;
		Hour = 6;
	}

	void NextDay()
	{
		if (Index == (Days.Num() - 1)) {
			Index = 0;
			Period = "Warmth";
		}
		else {
			Index++;

			if (Days[Index] == 1)
				Period = "Cold";
		}
	}
};

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
		FCalendarStruct Calendar;

	UPROPERTY()
		FRotator WindRotation;

	void ChangeWindDirection();

	void SetDisplayText(int32 Hour);
};
