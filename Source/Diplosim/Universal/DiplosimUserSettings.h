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
		bool bDepthOfField;

	UPROPERTY(Config)
		bool bVignette;

	UPROPERTY(Config)
		int32 ScreenPercentage;

	UPROPERTY(Config)
		FString Resolution;

	UPROPERTY(Config)
		FVector2D WindowPosition;

	UPROPERTY(Config)
		bool bIsMaximised;

	UPROPERTY(Config)
		float MasterVolume;

	UPROPERTY(Config)
		float SFXVolume;

	UPROPERTY(Config)
		float AmbientVolume;

public:
	UFUNCTION(BlueprintCallable)
		static UDiplosimUserSettings* GetDiplosimUserSettings();

	void OnWindowResize(FViewport* Viewport, uint32 Value);

	void OnGameWindowMoved(const TSharedRef<SWindow>& WindowBeingMoved);

	UFUNCTION(BlueprintCallable)
		void SaveIniSettings();
		
	void LoadIniSettings();

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

	UFUNCTION(BlueprintCallable, Category = "Depth Of Field")
		void SetDepthOfField(bool Value);

	UFUNCTION(BlueprintPure, Category = "Vignette")
		bool GetDepthOfField() const;

	UFUNCTION(BlueprintCallable, Category = "Vignette")
		void SetVignette(bool Value);

	UFUNCTION(BlueprintPure, Category = "Vignette")
		bool GetVignette() const;

	UFUNCTION(BlueprintCallable, Category = "Screen Percentage")
		void SetScreenPercentage(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Screen Percentage")
		int32 GetScreenPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Resolution")
		void SetResolution(FString Value);

	UFUNCTION(BlueprintPure, Category = "Resolution")
		FString GetResolution() const;

	UFUNCTION(BlueprintCallable, Category = "Resolution")
		void SetWindowPos(FVector2D Position);

	UFUNCTION(BlueprintPure, Category = "Resolution")
		FVector2D GetWindowPos() const;

	UFUNCTION(BlueprintCallable, Category = "Resolution")
		void SetMaximised(bool Value);

	UFUNCTION(BlueprintPure, Category = "Resolution")
		bool GetMaximised() const;

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

	FVector2D GetWindowPosAsVector(FString Value);

	UPROPERTY()
		class UCloudComponent* Clouds;

	UPROPERTY()
		class UAtmosphereComponent* Atmosphere;

	UPROPERTY()
		class ADiplosimGameModeBase* GameMode;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		FString Section;

	UPROPERTY()
		FString Filename;

private:
	FKeyValueSink _keyValueSink;

	void HandleSink(const TCHAR* Key, const TCHAR* Value);
};
