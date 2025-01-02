#include "DiplosimUserSettings.h"

#include "Camera/CameraComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/AudioComponent.h"
#include "Misc/FileHelper.h"

#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "Buildings/Building.h"

UDiplosimUserSettings::UDiplosimUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bEnemies = true;

	bRenderClouds = true;
	bRenderFog = true;

	AAName = "None";

	GIName = "None";

	Resolution = "0x0";

	bRayTracing = false;

	bMotionBlur = false;
	bDepthOfField = false;
	bVignette = true;

	bIsMaximised = false;

	ScreenPercentage = 100;

	WindowPosition = FVector2D(0.0f);

	MasterVolume = 1.0f;
	SFXVolume = 1.0f;
	AmbientVolume = 1.0f;

	Atmosphere = nullptr;
	Clouds = nullptr;

	GameMode = nullptr;

	Section = "/Script/Diplosim.DiplosimUserSettings";
	Filename = FPaths::ProjectDir() + "/Content/Settings/CustomUserSettings.ini";

	_keyValueSink.BindUObject(this, &UDiplosimUserSettings::HandleSink);
}

void UDiplosimUserSettings::OnWindowResize(FViewport* Viewport, uint32 Value)
{
	FString res = FString::FromInt(Viewport->GetSizeXY().X) + "x" + FString::FromInt(Viewport->GetSizeXY().Y);

	if (GetResolution() == res)
		return;

	if (GEngine->GameViewport->GetWindow()->GetNativeWindow()->IsMaximized())
		bIsMaximised = true;
	else
		bIsMaximised = false;

	Resolution = res;

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

	if (FString("bRenderClouds").Equals(Key))
		SetRenderClouds(value.ToBool());
	else if (FString("bRenderFog").Equals(Key))
		SetRenderFog(value.ToBool());
	else if (FString("AAName").Equals(Key))
		SetAA(value);
	else if (FString("GIName").Equals(Key))
		SetGI(value);
	else if (FString("Resolution").Equals(Key))
		SetResolution(value);
	else if (FString("bRayTracing").Equals(Key))
		SetRayTracing(value.ToBool());
	else if (FString("bMotionBlur").Equals(Key))
		SetMotionBlur(value.ToBool());
	else if (FString("bDepthOfField").Equals(Key))
		SetDepthOfField(value.ToBool());
	else if (FString("bVignette").Equals(Key))
		SetVignette(value.ToBool());
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
	GConfig->SetBool(*Section, TEXT("bRenderClouds"), GetRenderClouds(), Filename);
	GConfig->SetBool(*Section, TEXT("bRenderFog"), GetRenderFog(), Filename);
	GConfig->SetString(*Section, TEXT("AAName"), *GetAA(), Filename);
	GConfig->SetString(*Section, TEXT("GIName"), *GetGI(), Filename);
	GConfig->SetString(*Section, TEXT("Resolution"), *GetResolution(), Filename);
	GConfig->SetBool(*Section, TEXT("bRayTracing"), GetRayTracing(), Filename);
	GConfig->SetBool(*Section, TEXT("bMotionBlur"), GetMotionBlur(), Filename);
	GConfig->SetBool(*Section, TEXT("bDepthOfField"), GetDepthOfField(), Filename);
	GConfig->SetBool(*Section, TEXT("bVignette"), GetVignette(), Filename);
	GConfig->SetInt(*Section, TEXT("ScreenPercentage"), GetScreenPercentage(), Filename);
	GConfig->SetVector2D(*Section, TEXT("WindowPosition"), GetWindowPos(), Filename);
	GConfig->SetBool(*Section, TEXT("bIsMaximised"), GetMaximised(), Filename);
	GConfig->SetFloat(*Section, TEXT("MasterVolume"), GetMasterVolume(), Filename);
	GConfig->SetFloat(*Section, TEXT("SFXVolume"), GetSFXVolume(), Filename);
	GConfig->SetFloat(*Section, TEXT("AmbientVolume"), GetAmbientVolume(), Filename);

	GConfig->Flush(false, Filename);
}

void UDiplosimUserSettings::SetSpawnEnemies(bool Value)
{
	bEnemies = Value;

	if (GameMode == nullptr)
		return;

	if (!GameMode->GetWorldTimerManager().IsTimerActive(GameMode->WaveTimer) && GameMode->CheckEnemiesStatus())
		GameMode->SetWaveTimer();
}

bool UDiplosimUserSettings::GetSpawnEnemies() const
{
	return bEnemies;
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

void UDiplosimUserSettings::SetRenderFog(bool Value)
{
	bRenderFog = Value;

	if (Atmosphere == nullptr)
		return;

	if (bRenderFog)
		Atmosphere->Fog->SetHiddenInGame(false);
	else
		Atmosphere->Fog->SetHiddenInGame(true);
}

bool UDiplosimUserSettings::GetRenderFog() const
{
	return bRenderFog;
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

void UDiplosimUserSettings::SetRayTracing(bool Value)
{
	bRayTracing = Value;

	if (bRayTracing)
		GEngine->Exec(GetWorld(), TEXT("r.RayTracing.Enable 1"));
	else
		GEngine->Exec(GetWorld(), TEXT("r.RayTracing.Enable 0"));
}

bool UDiplosimUserSettings::GetRayTracing() const
{
	return bRayTracing;
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

void UDiplosimUserSettings::SetResolution(FString Value)
{
	Resolution = Value;

	FSlateApplicationBase::Get().GetPlatformCursor()->Lock(nullptr);

	FString cmd = "r.SetRes " + Resolution;

	GEngine->Exec(GetWorld(), *cmd);
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