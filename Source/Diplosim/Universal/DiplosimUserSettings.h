#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "DiplosimUserSettings.generated.h"

UCLASS()
class DIPLOSIM_API UDiplosimUserSettings : public UGameUserSettings
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(Config)
		bool bEnemies;

	UPROPERTY(Config)
		bool bRenderClouds;

	UPROPERTY(Config)
		bool bRenderFog;

	UPROPERTY(Config)
		bool bUseAA;

	UPROPERTY(Config)
		bool bMotionBlur;

	UPROPERTY(Config)
		FString SunPosition;

public:
	UFUNCTION(BlueprintCallable)
		static UDiplosimUserSettings* GetDiplosimUserSettings();

	UFUNCTION(BlueprintCallable, Category = "Enemies")
		void SetSpawnEnemies(bool Value);

	UFUNCTION(BlueprintPure, Category = "Enemies")
		bool GetSpawnEnemies() const;

	UFUNCTION(BlueprintCallable, Category = "Clouds")
		void SetRenderClouds(bool Value);

	UFUNCTION(BlueprintPure, Category = "Clouds")
		bool GetRenderClouds() const;

	UFUNCTION(BlueprintCallable, Category = "Fog")
		void SetRenderFog(bool Value);

	UFUNCTION(BlueprintPure, Category = "Fog")
		bool GetRenderFog() const;

	UFUNCTION(BlueprintCallable, Category = "AA")
		void SetAA(bool Value);

	UFUNCTION(BlueprintPure, Category = "AA")
		bool GetAA() const;

	UFUNCTION(BlueprintCallable, Category = "Motion Blur")
		void SetMotionBlur(bool Value);

	UFUNCTION(BlueprintPure, Category = "Motion Blur")
		bool GetMotionBlur() const;

	UFUNCTION(BlueprintCallable, Category = "Sun Position")
		void SetSunPosition(FString Value);

	UFUNCTION(BlueprintPure, Category = "Sun Position")
		FString GetSunPosition() const;

	UPROPERTY()
		class AClouds* Clouds;

	UPROPERTY()
		class AExponentialHeightFog* Fog;

	UPROPERTY()
		class UAtmosphereComponent* Atmosphere;

	UPROPERTY()
		class ADiplosimGameModeBase* GameMode;
};
