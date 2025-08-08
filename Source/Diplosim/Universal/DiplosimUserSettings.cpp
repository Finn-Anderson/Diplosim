#include "DiplosimUserSettings.h"

#include "Camera/CameraComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/AudioComponent.h"
#include "Misc/FileHelper.h"
#include "Engine/UserInterfaceSettings.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"

#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Building.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bEnemies = true;

	bAnim = true;

	bSmoothCamera = true;

	bRenderTorches = true;

	bRenderClouds = true;
	bRenderWind = true;
	bRain = true;

	SunBrightness = 10.0f;
	MoonBrightness = 0.5f;

	AAName = "None";

	GIName = "None";

	Resolution = "0x0";

	ShadowLevel = 1;

	bMotionBlur = false;
	bDepthOfField = false;
	bVignette = true;
	bSSAO = true;
	bVolumetricFog = true;
	bLightShafts = true;
	Bloom = 0.6f;
	WPODistance = 5000.0f;

	bIsMaximised = false;

	ScreenPercentage = 100;

	WindowPosition = FVector2D(0.0f);

	MasterVolume = 1.0f;
	SFXVolume = 1.0f;
	AmbientVolume = 1.0f;

	UIScale = 1.0f;

	Atmosphere = nullptr;
	Clouds = nullptr;

	GameMode = nullptr;

	Camera = nullptr;

	Section = "/Script/Diplosim.DiplosimUserSettings";
	Filename = FPaths::ProjectDir() + "/Content/Settings/CustomUserSettings.ini";

	_keyValueSink.BindUObject(this, &UDiplosimUserSettings::HandleSink);
}

void UDiplosimUserSettings::OnWindowResize(FViewport* Viewport, uint32 Value)
{
	FVector2D viewportSize;
	GEngine->GameViewport->GetViewportSize(viewportSize);
	
	FString res = FString::FromInt(viewportSize.X) + "x" + FString::FromInt(viewportSize.Y);

	if (GetResolution() == res)
		return;

	if (GEngine->GameViewport->GetWindow()->GetNativeWindow()->IsMaximized())
		SetMaximised(true);
	else
		SetMaximised(false);

	SetResolution(res, true);

	GConfig->SetString(*Section, TEXT("Resolution"), *GetResolution(), Filename);
	GConfig->SetBool(*Section, TEXT("bIsMaximised"), GetMaximised(), Filename);

	GConfig->Flush(false, Filename);
}

void UDiplosimUserSettings::OnGameWindowMoved(const TSharedRef<SWindow>& WindowBeingMoved)
{
	if (GetWindowPos() == WindowBeingMoved.Get().GetPositionInScreen())
		return;
	
	WindowPosition = WindowBeingMoved.Get().GetPositionInScreen();

	GConfig->SetVector2D(*Section, TEXT("WindowPosition"), GetWindowPos(), Filename);

	GConfig->Flush(false, Filename);
}

void UDiplosimUserSettings::HandleSink(const TCHAR* Key, const TCHAR* Value)
{
	FString value = FString(Value);

	if (FString("bEnemies").Equals(Key))
		SetSpawnEnemies(value.ToBool());
	else if (FString("bAnim").Equals(Key))
		SetViewAnimations(value.ToBool());
	else if (FString("bSmoothCamera").Equals(Key))
		SetSmoothCamera(value.ToBool());
	else if (FString("bRenderTorches").Equals(Key))
		SetRenderTorches(value.ToBool());
	else if (FString("bRenderClouds").Equals(Key))
		SetRenderClouds(value.ToBool());
	else if (FString("bRenderWind").Equals(Key))
		SetRenderWind(value.ToBool());
	else if (FString("bRain").Equals(Key))
		SetRain(value.ToBool());
	else if (FString("SunBrightness").Equals(Key))
		SetSunBrightness(FCString::Atof(Value));
	else if (FString("MoonBrightness").Equals(Key))
		SetMoonBrightness(FCString::Atof(Value));
	else if (FString("AAName").Equals(Key))
		SetAA(value);
	else if (FString("GIName").Equals(Key))
		SetGI(value);
	else if (FString("Resolution").Equals(Key))
		SetResolution(value);
	else if (FString("ShadowLevel").Equals(Key))
		SetShadowLevel(FCString::Atoi(Value));
	else if (FString("bMotionBlur").Equals(Key))
		SetMotionBlur(value.ToBool());
	else if (FString("bDepthOfField").Equals(Key))
		SetDepthOfField(value.ToBool());
	else if (FString("bVignette").Equals(Key))
		SetVignette(value.ToBool());
	else if (FString("bSSAO").Equals(Key))
		SetSSAO(value.ToBool());
	else if (FString("bVolumetricFog").Equals(Key))
		SetVolumetricFog(value.ToBool());
	else if (FString("bLightShafts").Equals(Key))
		SetLightShafts(value.ToBool());
	else if (FString("Bloom").Equals(Key))
		SetBloom(FCString::Atof(Value));
	else if (FString("WPODistance").Equals(Key))
		SetWPODistance(FCString::Atof(Value));
	else if (FString("ScreenPercentage").Equals(Key))
		SetScreenPercentage(FCString::Atoi(Value));
	else if (FString("WindowPosition").Equals(Key))
		SetWindowPos(GetWindowPosAsVector(Value));
	else if (FString("bIsMaximised").Equals(Key))
		SetMaximised(value.ToBool());
	else if (FString("MasterVolume").Equals(Key))
		SetMasterVolume(FCString::Atof(Value));
	else if (FString("SFXVolume").Equals(Key))
		SetSFXVolume(FCString::Atof(Value));
	else if (FString("AmbientVolume").Equals(Key))
		SetAmbientVolume(FCString::Atof(Value));
	else if (FString("UIScale").Equals(Key))
		SetUIScale(FCString::Atof(Value));
	else if (FString("bShowLog").Equals(Key))
		SetShowLog(value.ToBool());
}

void UDiplosimUserSettings::LoadIniSettings()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.FileExists(*Filename))
		FFileHelper::SaveStringToFile(TEXT(""), *Filename);

	GConfig->Flush(true, Filename);

	GConfig->LoadFile(Filename);

	GConfig->ForEachEntry(_keyValueSink, *Section, Filename);

	GEngine->Exec(GetWorld(), TEXT("r.FullScreenMode 2"));

	if (Resolution == "0x0") {
		TArray<FIntPoint> resolutions;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(resolutions);

		SetResolution(FString::FromInt(resolutions.Last().X) + "x" + FString::FromInt(resolutions.Last().Y));
	}

	GEngine->GameViewport->Viewport->ViewportResizedEvent.AddUObject(this, &UDiplosimUserSettings::OnWindowResize);
	GEngine->GameViewport->GetWindow()->SetOnWindowMoved(FOnWindowMoved::CreateUObject(this, &UDiplosimUserSettings::OnGameWindowMoved));
}

void UDiplosimUserSettings::SaveIniSettings()
{
	GConfig->SetBool(*Section, TEXT("bEnemies"), GetSpawnEnemies(), Filename);
	GConfig->SetBool(*Section, TEXT("bAnim"), GetViewAnimations(), Filename);
	GConfig->SetBool(*Section, TEXT("bSmoothCamera"), GetSmoothCamera(), Filename);
	GConfig->SetBool(*Section, TEXT("bRenderTorches"), GetRenderTorches(), Filename);
	GConfig->SetBool(*Section, TEXT("bRenderClouds"), GetRenderClouds(), Filename);
	GConfig->SetBool(*Section, TEXT("bRenderWind"), GetRenderWind(), Filename);
	GConfig->SetBool(*Section, TEXT("bRain"), GetRain(), Filename);
	GConfig->SetFloat(*Section, TEXT("SunBrightness"), GetSunBrightness(), Filename);
	GConfig->SetFloat(*Section, TEXT("MoonBrightness"), GetMoonBrightness(), Filename);
	GConfig->SetString(*Section, TEXT("AAName"), *GetAA(), Filename);
	GConfig->SetString(*Section, TEXT("GIName"), *GetGI(), Filename);
	GConfig->SetString(*Section, TEXT("Resolution"), *GetResolution(), Filename);
	GConfig->SetInt(*Section, TEXT("ShadowLevel"), GetShadowLevel(), Filename);
	GConfig->SetBool(*Section, TEXT("bMotionBlur"), GetMotionBlur(), Filename);
	GConfig->SetBool(*Section, TEXT("bDepthOfField"), GetDepthOfField(), Filename);
	GConfig->SetBool(*Section, TEXT("bVignette"), GetVignette(), Filename);
	GConfig->SetBool(*Section, TEXT("bSSAO"), GetSSAO(), Filename);
	GConfig->SetBool(*Section, TEXT("bVolumetricFog"), GetVolumetricFog(), Filename);
	GConfig->SetBool(*Section, TEXT("bLightShafts"), GetLightShafts(), Filename);
	GConfig->SetFloat(*Section, TEXT("Bloom"), GetBloom(), Filename);
	GConfig->SetFloat(*Section, TEXT("WPODistance"), GetWPODistance(), Filename);
	GConfig->SetInt(*Section, TEXT("ScreenPercentage"), GetScreenPercentage(), Filename);
	GConfig->SetVector2D(*Section, TEXT("WindowPosition"), GetWindowPos(), Filename);
	GConfig->SetBool(*Section, TEXT("bIsMaximised"), GetMaximised(), Filename);
	GConfig->SetFloat(*Section, TEXT("MasterVolume"), GetMasterVolume(), Filename);
	GConfig->SetFloat(*Section, TEXT("SFXVolume"), GetSFXVolume(), Filename);
	GConfig->SetFloat(*Section, TEXT("AmbientVolume"), GetAmbientVolume(), Filename);
	GConfig->SetFloat(*Section, TEXT("UIScale"), GetUIScale(), Filename);
	GConfig->SetBool(*Section, TEXT("bShowLog"), GetShowLog(), Filename);

	GConfig->Flush(false, Filename);
}

void UDiplosimUserSettings::SetSpawnEnemies(bool Value)
{
	bEnemies = Value;

	if (!IsValid(GameMode) || !IsValid(Camera) || !IsValid(Camera->CitizenManager))
		return;

	Camera->CitizenManager->ResetTimer("WaveTimer", GameMode);
}

bool UDiplosimUserSettings::GetSpawnEnemies() const
{
	return bEnemies;
}

void UDiplosimUserSettings::SetViewAnimations(bool Value)
{
	bAnim = Value;

	if (!IsValid(Camera) || !IsValid(Camera->CitizenManager))
		return;

	for (ACitizen* citizen : Camera->CitizenManager->Citizens) {
		UAnimSequence* anim = nullptr;
		bool bLooping = false;

		if (bAnim && !citizen->MovementComponent->Points.IsEmpty()) {
			anim = citizen->MovementComponent->MoveAnim;
			bLooping = true;
		}

		citizen->MovementComponent->SetAnimation(anim, bLooping, true);
	}
}

bool UDiplosimUserSettings::GetViewAnimations() const
{
	return bAnim;
}

void UDiplosimUserSettings::SetSmoothCamera(bool Value)
{
	bSmoothCamera = Value;
}

bool UDiplosimUserSettings::GetSmoothCamera() const
{
	return bSmoothCamera;
}

void UDiplosimUserSettings::SetRenderTorches(bool Value)
{
	bRenderTorches = Value;

	if (Camera == nullptr || Atmosphere == nullptr)
		return;

	int32 hour = 12;

	if (bRenderTorches)
		hour = Atmosphere->Calendar.Hour;

	Camera->Grid->AIVisualiser->ActivateTorches(hour);
}

bool UDiplosimUserSettings::GetRenderTorches() const
{
	return bRenderTorches;
}

void UDiplosimUserSettings::SetRenderClouds(bool Value)
{
	if (bRenderClouds == Value)
		return;

	bRenderClouds = Value;

	if (Clouds == nullptr)
		return;
	
	if (bRenderClouds) {
		Clouds->SetComponentTickEnabled(true);

		Clouds->ActivateCloud();
	}
	else {
		Clouds->Clear();

		Clouds->SetComponentTickEnabled(false);
	}
}

bool UDiplosimUserSettings::GetRenderClouds() const
{
	return bRenderClouds;
}

void UDiplosimUserSettings::SetRain(bool Value)
{
	if (bRain == Value)
		return;

	bRain = Value;

	if (Clouds == nullptr || bRain)
		return;

	for (int32 i = Clouds->Clouds.Num() - 1; i > -1; i--) {
		FCloudStruct cloudStruct = Clouds->Clouds[i];

		if (cloudStruct.Precipitation == nullptr)
			continue;

		cloudStruct.Precipitation->DestroyComponent();
		cloudStruct.HISMCloud->DestroyComponent();

		Clouds->Clouds.RemoveAt(i);
	}
}

bool UDiplosimUserSettings::GetRain() const
{
	return bRain;
}

void UDiplosimUserSettings::SetRenderWind(bool Value)
{
	bRenderWind = Value;

	if (Atmosphere == nullptr)
		return;

	if (bRenderWind)
		Atmosphere->WindComponent->Activate();
	else
		Atmosphere->WindComponent->Deactivate();
}

bool UDiplosimUserSettings::GetRenderWind() const
{
	return bRenderWind;
}

void UDiplosimUserSettings::SetSunBrightness(float Value)
{
	SunBrightness = Value;

	if (Atmosphere == nullptr)
		return;

	Atmosphere->Sun->SetIntensity(SunBrightness);
}

float UDiplosimUserSettings::GetSunBrightness() const
{
	return SunBrightness;
}

void UDiplosimUserSettings::SetMoonBrightness(float Value)
{
	MoonBrightness = Value;

	if (Atmosphere == nullptr)
		return;

	Atmosphere->Moon->SetIntensity(MoonBrightness);
}

float UDiplosimUserSettings::GetMoonBrightness() const
{
	return MoonBrightness;
}

void UDiplosimUserSettings::SetAA(FString Value)
{
	AAName = Value;

	if (AAName == "None")
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 0"));
	else if (AAName == "TAA")
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 2"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 4"));
}

FString UDiplosimUserSettings::GetAA() const
{
	return AAName;
}

void UDiplosimUserSettings::SetGI(FString Value)
{
	GIName = Value;

	if (GIName == "None") {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 0"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 0"));
	}
	else if (GIName == "Lumen") {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 1"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 1"));
	}
	else {
		GEngine->Exec(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 2"));
		GEngine->Exec(GetWorld(), TEXT("r.ReflectionMethod 2"));
	}
}

FString UDiplosimUserSettings::GetGI() const
{
	return GIName;
}

void UDiplosimUserSettings::SetShadowLevel(int32 Value)
{
	ShadowLevel = Value;

	if (ShadowLevel == 5) {
		GEngine->Exec(GetWorld(), TEXT("r.Shadow.Virtual.Enable 1"));
	}
	else {
		GEngine->Exec(GetWorld(), TEXT("r.Shadow.Virtual.Enable 0"));

		FString cmd = "r.Shadow.CSM.MaxCascades " + FString::FromInt(ShadowLevel);

		GEngine->Exec(GetWorld(), *cmd);
	}
}

int32 UDiplosimUserSettings::GetShadowLevel() const
{
	return ShadowLevel;
}

void UDiplosimUserSettings::SetMotionBlur(bool Value)
{
	bMotionBlur = Value;

	if (bMotionBlur)
		GEngine->Exec(GetWorld(), TEXT("r.MotionBlur.Amount 0.39"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.MotionBlur.Amount 0"));
}

bool UDiplosimUserSettings::GetMotionBlur() const
{
	return bMotionBlur;
}

void UDiplosimUserSettings::SetDepthOfField(bool Value)
{
	bDepthOfField = Value;

	if (bDepthOfField)
		GEngine->Exec(GetWorld(), TEXT("r.DepthOfFieldQuality 1"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.DepthOfFieldQuality 0"));
}

bool UDiplosimUserSettings::GetDepthOfField() const
{
	return bDepthOfField;
}

void UDiplosimUserSettings::SetVignette(bool Value)
{
	bVignette = Value;

	if (Camera == nullptr)
		return;

	if (bVignette)
		Camera->CameraComponent->PostProcessSettings.VignetteIntensity = 0.25f;
	else
		Camera->CameraComponent->PostProcessSettings.VignetteIntensity = 0.0f;
}

bool UDiplosimUserSettings::GetVignette() const
{
	return bVignette;
}

void UDiplosimUserSettings::SetSSAO(bool Value)
{
	bSSAO = Value;

	if (Camera == nullptr)
		return;

	if (bSSAO)
		Camera->CameraComponent->PostProcessSettings.AmbientOcclusionIntensity = 0.5f;
	else
		Camera->CameraComponent->PostProcessSettings.AmbientOcclusionIntensity = 0.0f;
}

bool UDiplosimUserSettings::GetSSAO() const
{
	return bSSAO;
}

void UDiplosimUserSettings::SetVolumetricFog(bool Value)
{
	bVolumetricFog = Value;

	if (Atmosphere == nullptr)
		return;

	Atmosphere->Fog->SetVolumetricFog(bVolumetricFog);
}

bool UDiplosimUserSettings::GetVolumetricFog() const
{
	return bVolumetricFog;
}

void UDiplosimUserSettings::SetLightShafts(bool Value)
{
	bLightShafts = Value;

	if (Atmosphere == nullptr)
		return;

	float intensity = 0.0f;
	
	if (bLightShafts)
		intensity = 0.5f;

	Atmosphere->Sun->SetBloomMaxBrightness(intensity);
	Atmosphere->Moon->SetBloomMaxBrightness(intensity);
}

bool UDiplosimUserSettings::GetLightShafts() const
{
	return bLightShafts;
}

void UDiplosimUserSettings::SetBloom(float Value)
{
	if (Bloom == Value)
		return;

	Bloom = Value;

	if (Camera == nullptr)
		return;

	Camera->CameraComponent->PostProcessSettings.BloomIntensity = Value;
}

float UDiplosimUserSettings::GetBloom() const
{
	return Bloom;
}

void UDiplosimUserSettings::SetWPODistance(float Value)
{
	if (WPODistance == Value)
		return;

	WPODistance = Value;

	if (Camera == nullptr)
		return;

	Camera->Grid->HISMRiver->SetWorldPositionOffsetDisableDistance(GetWPODistance());

	for (FResourceHISMStruct& ResourceStruct : Camera->Grid->TreeStruct)
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(GetWPODistance());

	for (FResourceHISMStruct& ResourceStruct : Camera->Grid->FlowerStruct)
		ResourceStruct.Resource->ResourceHISM->SetWorldPositionOffsetDisableDistance(GetWPODistance());
}

float UDiplosimUserSettings::GetWPODistance() const
{
	float distance = WPODistance;

	if (distance == 0.0f)
		distance = 1.0f;

	return distance;
}

void UDiplosimUserSettings::SetScreenPercentage(int32 Value)
{
	ScreenPercentage = Value;

	FString cmd = "r.ScreenPercentage " + FString::FromInt(ScreenPercentage);

	GEngine->Exec(GetWorld(), *cmd);
}

int32 UDiplosimUserSettings::GetScreenPercentage() const
{
	return ScreenPercentage;
}

void UDiplosimUserSettings::SetResolution(FString Value, bool bResizing)
{
	Resolution = Value;

	if (Camera)
		Camera->UpdateResolutionText();

	if (bResizing)
		return;

	FSlateApplicationBase::Get().GetPlatformCursor()->Lock(nullptr);

	FString cmd = "r.SetRes " + Resolution;

	GEngine->Exec(GetWorld(), *cmd);

	SetMaximised(false);
}

FString UDiplosimUserSettings::GetResolution() const
{
	return Resolution;
}

void UDiplosimUserSettings::SetWindowPos(FVector2D Position)
{
	WindowPosition = Position;
	
	GEngine->GameViewport->GetWindow()->MoveWindowTo(WindowPosition);
}

FVector2D UDiplosimUserSettings::GetWindowPos() const
{
	return WindowPosition;
}

void UDiplosimUserSettings::SetMaximised(bool Value)
{
	bIsMaximised = Value;

	if (bIsMaximised)
		GEngine->GameViewport->GetWindow()->GetNativeWindow()->Maximize();
}

bool UDiplosimUserSettings::GetMaximised() const
{
	return bIsMaximised;
}

void UDiplosimUserSettings::SetMasterVolume(float Value)
{
	if (MasterVolume == Value)
		return;

	MasterVolume = Value;

	UpdateAmbientVolume();
}

float UDiplosimUserSettings::GetMasterVolume() const
{
	return MasterVolume;
}

void UDiplosimUserSettings::SetSFXVolume(float Value)
{
	if (SFXVolume == Value)
		return;

	SFXVolume = Value;
}

float UDiplosimUserSettings::GetSFXVolume() const
{
	return SFXVolume;
}

void UDiplosimUserSettings::SetAmbientVolume(float Value)
{
	if (AmbientVolume == Value)
		return;

	AmbientVolume = Value;

	UpdateAmbientVolume();
}

float UDiplosimUserSettings::GetAmbientVolume() const
{
	return AmbientVolume;
}

void UDiplosimUserSettings::SetUIScale(float Value)
{
	if (UIScale == Value)
		return;

	UIScale = Value;

	UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass());
	UISettings->ApplicationScale = UIScale;
}

float UDiplosimUserSettings::GetUIScale() const
{
	return UIScale;
}

void UDiplosimUserSettings::SetShowLog(bool Value)
{
	bShowLog = Value;

	if (!IsValid(Camera) || !Camera->BuildUIInstance->IsInViewport())
		return;

	if (bShowLog)
		Camera->LogUIInstance->AddToViewport();
	else
		Camera->LogUIInstance->RemoveFromParent();
}

bool UDiplosimUserSettings::GetShowLog() const
{
	return bShowLog;
}

void UDiplosimUserSettings::UpdateAmbientVolume()
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!PController)
		return;

	ACamera* camera = PController->GetPawn<ACamera>();

	for (ACitizen* citizen : camera->CitizenManager->Citizens)
		citizen->AmbientAudioComponent->SetVolumeMultiplier(GetAmbientVolume() * GetMasterVolume());

	for (ABuilding* building : camera->CitizenManager->Buildings)
		building->AmbientAudioComponent->SetVolumeMultiplier(GetAmbientVolume() * GetMasterVolume());
}

FVector2D UDiplosimUserSettings::GetWindowPosAsVector(FString Value)
{
	TArray<FString> positions;
	Value.ParseIntoArray(positions, TEXT(" "));

	TArray<int32> XY;

	for (FString pos : positions) {
		TArray<FString> position;

		pos.ParseIntoArray(position, TEXT("="));

		XY.Add(FCString::Atoi(*position[1]));
	}

	return FVector2D(XY[0], XY[1]);
}

UDiplosimUserSettings* UDiplosimUserSettings::GetDiplosimUserSettings()
{
	return Cast<UDiplosimUserSettings>(UGameUserSettings::GetGameUserSettings());
}