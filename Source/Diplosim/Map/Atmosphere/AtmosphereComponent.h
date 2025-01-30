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
		Period = "Spring";
		Days = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
		Index = 0;
		Hour = 6;
	}

	void NextDay()
	{
		if (Index == (Days.Num() - 1)) {
			Index = 0;
			Period = "Spring";
		}
		else {
			Index++;

			if (Days[Index] != 1)
				return;

			if (Period == "Spring")
				Period = "Summer";
			else if (Period == "Summer")
				Period = "Autumn";
			else if (Period == "Autumn")
				Period = "Winter";
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
		class UStaticMeshComponent* Skybox;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* WindComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
		FCalendarStruct Calendar;

	UPROPERTY()
		FRotator WindRotation;

	void ChangeWindDirection();

	void SetWindDimensions(int32 Size);

	void SetDisplayText(int32 Hour);
};
