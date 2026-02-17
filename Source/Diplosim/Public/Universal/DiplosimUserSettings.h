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
		bool bSmoothCamera;

	UPROPERTY(Config)
		bool bRenderTorches;

	UPROPERTY(Config)
		bool bRenderClouds;

	UPROPERTY(Config)
		bool bRain;

	UPROPERTY(Config)
		bool bRenderWind;

	UPROPERTY(Config)
		float SunBrightness;

	UPROPERTY(Config)
		float MoonBrightness;

	UPROPERTY(Config)
		FString AAName;

	UPROPERTY(Config)
		FString GIName;

	UPROPERTY(Config)
		int32 ShadowLevel;

	UPROPERTY(Config)
		bool bMotionBlur;

	UPROPERTY(Config)
		bool bDepthOfField;

	UPROPERTY(Config)
		bool bVignette;

	UPROPERTY(Config)
		bool bSSAO;

	UPROPERTY(Config)
		bool bVolumetricFog;

	UPROPERTY(Config)
		bool bLightShafts;

	UPROPERTY(Config)
		float Bloom;

	UPROPERTY(Config)
		float WPODistance;

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

	UPROPERTY(Config)
		float UIScale;

	UPROPERTY(Config)
		bool bShowLog;

	UPROPERTY(Config)
		int32 AutosaveTimer;

	UPROPERTY(Config)
		int32 CitizenNum;

	UPROPERTY(Config)
		float PopupUISpeed;

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

	UFUNCTION(BlueprintCallable, Category = "Camera")
		void SetSmoothCamera(bool Value);

	UFUNCTION(BlueprintPure, Category = "Camera")
		bool GetSmoothCamera() const;

	UFUNCTION(BlueprintCallable, Category = "Torches")
		void SetRenderTorches(bool Value);

	UFUNCTION(BlueprintPure, Category = "Torches")
		bool GetRenderTorches() const;

	UFUNCTION(BlueprintCallable, Category = "Clouds")
		void SetRenderClouds(bool Value);

	UFUNCTION(BlueprintPure, Category = "Clouds")
		bool GetRenderClouds() const;

	UFUNCTION(BlueprintCallable, Category = "Clouds")
		void SetRain(bool Value);

	UFUNCTION(BlueprintPure, Category = "Clouds")
		bool GetRain() const;

	UFUNCTION(BlueprintCallable, Category = "Wind")
		void SetRenderWind(bool Value);

	UFUNCTION(BlueprintPure, Category = "Wind")
		bool GetRenderWind() const;

	UFUNCTION(BlueprintCallable, Category = "Brightness")
		void SetSunBrightness(float Value);

	UFUNCTION(BlueprintPure, Category = "Brightness")
		float GetSunBrightness() const;

	UFUNCTION(BlueprintCallable, Category = "Brightness")
		void SetMoonBrightness(float Value);

	UFUNCTION(BlueprintPure, Category = "Brightness")
		float GetMoonBrightness() const;

	UFUNCTION(BlueprintCallable, Category = "AA")
		void SetAA(FString Value);

	UFUNCTION(BlueprintPure, Category = "AA")
		FString GetAA() const;

	UFUNCTION(BlueprintCallable, Category = "GI")
		void SetGI(FString Value);

	UFUNCTION(BlueprintPure, Category = "GI")
		FString GetGI() const;

	UFUNCTION(BlueprintCallable, Category = "Shadows")
		void SetShadowLevel(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Shadows")
		int32 GetShadowLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Motion Blur")
		void SetMotionBlur(bool Value);

	UFUNCTION(BlueprintPure, Category = "Motion Blur")
		bool GetMotionBlur() const;

	UFUNCTION(BlueprintCallable, Category = "Post Processing")
		void SetDepthOfField(bool Value);

	UFUNCTION(BlueprintPure, Category = "Post Processing")
		bool GetDepthOfField() const;

	UFUNCTION(BlueprintCallable, Category = "Post Processing")
		void SetVignette(bool Value);

	UFUNCTION(BlueprintPure, Category = "Post Processing")
		bool GetVignette() const;

	UFUNCTION(BlueprintCallable, Category = "Post Processing")
		void SetSSAO(bool Value);

	UFUNCTION(BlueprintPure, Category = "Post Processing")
		bool GetSSAO() const;

	UFUNCTION(BlueprintCallable, Category = "Fog")
		void SetVolumetricFog(bool Value);

	UFUNCTION(BlueprintPure, Category = "Fog")
		bool GetVolumetricFog() const;

	UFUNCTION(BlueprintCallable, Category = "Light Shafts")
		void SetLightShafts(bool Value);

	UFUNCTION(BlueprintPure, Category = "Light Shafts")
		bool GetLightShafts() const;

	UFUNCTION(BlueprintCallable, Category = "Post Processing")
		void SetBloom(float Value);

	UFUNCTION(BlueprintPure, Category = "Post Processing")
		float GetBloom() const;

	UFUNCTION(BlueprintCallable, Category = "WPO")
		void SetWPODistance(float Value);

	UFUNCTION(BlueprintPure, Category = "WPO")
		float GetWPODistance() const;

	UFUNCTION(BlueprintCallable, Category = "Screen Percentage")
		void SetScreenPercentage(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Screen Percentage")
		int32 GetScreenPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Resolution")
		void SetResolution(FString Value, bool bResizing = false);

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

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
		void SetUIScale(float Value);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
		float GetUIScale() const;

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
		void SetShowLog(bool Value);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
		bool GetShowLog() const;

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
		void SetAutosaveTimer(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
		int32 GetAutosaveTimer() const;

	UFUNCTION(BlueprintCallable, Category = "AI")
		void SetCitizenNum(int32 Value);

	UFUNCTION(BlueprintPure, Category = "AI")
		int32 GetCitizenNum() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
		void SetPopupUISpeed(float Value);

	UFUNCTION(BlueprintPure, Category = "UI")
		float GetPopupUISpeed() const;

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
