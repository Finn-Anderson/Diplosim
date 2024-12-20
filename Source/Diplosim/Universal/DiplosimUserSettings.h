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
		FString AAName;

	UPROPERTY(Config)
		FString GIName;

	UPROPERTY(Config)
		bool bRayTracing;

	UPROPERTY(Config)
		bool bMotionBlur;

	UPROPERTY(Config)
		int32 ScreenPercentage;

	UPROPERTY(Config)
		float MasterVolume;

	UPROPERTY(Config)
		float SFXVolume;

	UPROPERTY(Config)
		float AmbientVolume;

public:
	UFUNCTION(BlueprintCallable)
		static UDiplosimUserSettings* GetDiplosimUserSettings();
		
	void SetVisualSettings();

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
		void SetAA(FString Value);

	UFUNCTION(BlueprintPure, Category = "AA")
		FString GetAA() const;

	UFUNCTION(BlueprintCallable, Category = "GI")
		void SetGI(FString Value);

	UFUNCTION(BlueprintPure, Category = "GI")
		FString GetGI() const;

	UFUNCTION(BlueprintCallable, Category = "Fog")
		void SetRayTracing(bool Value);

	UFUNCTION(BlueprintPure, Category = "Fog")
		bool GetRayTracing() const;

	UFUNCTION(BlueprintCallable, Category = "Motion Blur")
		void SetMotionBlur(bool Value);

	UFUNCTION(BlueprintPure, Category = "Motion Blur")
		bool GetMotionBlur() const;

	UFUNCTION(BlueprintCallable, Category = "Screen Percentage")
		void SetScreenPercentage(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Screen Percentage")
		int32 GetScreenPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
		void SetMasterVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "Audio")
		float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
		void SetSFXVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "Audio")
		float GetSFXVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
		void SetAmbientVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "Audio")
		float GetAmbientVolume() const;

	void UpdateAmbientVolume();

	UPROPERTY()
		class UCloudComponent* Clouds;

	UPROPERTY()
		class UAtmosphereComponent* Atmosphere;

	UPROPERTY()
		class ADiplosimGameModeBase* GameMode;
};
