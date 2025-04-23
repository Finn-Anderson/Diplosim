#include "AtmosphereComponent.h"
#include "Engine/DirectionalLight.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Universal/DiplosimUserSettings.h"

UAtmosphereComponent::UAtmosphereComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(1.0f/24.0f);

	Speed = 0.025f;

	Skybox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skybox"));
	Skybox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skybox->SetRelativeScale3D(FVector(1000.0f, 1000.0f, 1000.0f));
	Skybox->SetCastShadow(false);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetRealTimeCaptureEnabled(true);
	SkyLight->SetCastShadows(false);
	SkyLight->SetIntensity(2.0f);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetRelativeRotation(FRotator(-15.0f, 15.0f, 0.0f));
	Sun->SetDynamicShadowDistanceMovableLight(20000.0f);
	Sun->SetCascadeDistributionExponent(1.0f);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetShadowSlopeBias(1.0f);
	Sun->SetShadowBias(1.0f);
	Sun->SetCastShadows(true);
	Sun->SetUseTemperature(true);
	Sun->SetTemperature(5772.0f);
	Sun->SetEnableLightShaftBloom(true);
	Sun->SetBloomMaxBrightness(0.5f);
	Sun->ForwardShadingPriority = 0;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetRelativeRotation(FRotator(15.0f, 195.0f, 0.0f));
	Moon->SetDynamicShadowDistanceMovableLight(20000.0f);
	Moon->SetCascadeDistributionExponent(1.0f);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetShadowSlopeBias(1.0f);
	Moon->SetShadowBias(1.0f);
	Moon->SetCastShadows(false);
	Moon->SetIntensity(0.5f);
	Moon->SetUseTemperature(true);
	Moon->SetTemperature(4110.0f);
	Moon->SetEnableLightShaftBloom(false);
	Moon->SetBloomMaxBrightness(0.5f);
	Moon->ForwardShadingPriority = 1;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetMultiScatteringFactor(4.0f);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetFogDensity(0.05f);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogDistance(10000.0f);

	WindComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WindComponent"));
	WindComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2500.0f));
	WindComponent->bAutoActivate = true;
}

void UAtmosphereComponent::BeginPlay()
{
	Super::BeginPlay();

	ChangeWindDirection();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Atmosphere = this;

	if (!settings->GetRenderFog())
		Fog->SetHiddenInGame(true);

	if (!settings->GetRenderWind())
		WindComponent->Deactivate();

	Sun->SetIntensity(settings->GetSunBrightness());
	Moon->SetIntensity(settings->GetMoonBrightness());
}

void UAtmosphereComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Cast<AGrid>(GetOwner())->Camera->Start)
		return;

	Sun->AddLocalRotation(FRotator(-Speed, Speed, 0.0f));
	Moon->AddLocalRotation(FRotator(Speed, Speed, 0.0f));

	int32 hour = FMath::Abs(FMath::FloorToInt(24.0f / 360.0f * (Moon->GetRelativeRotation().Yaw + 180.0f))) - 18;

	if (hour < 0)
		hour += 24;

	if (Calendar.Hour != hour && (hour == 6 || hour == 18)) {
		UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

		if (settings->GetRenderTorches())
			for (AActor* Actor : Cast<AGrid>(GetOwner())->Camera->CitizenManager->Citizens)
				Cast<ACitizen>(Actor)->SetTorch(hour);

		if (hour == 18) {
			Sun->SetCastShadows(false);
			Sun->SetEnableLightShaftBloom(false);

			Moon->SetCastShadows(true);
			Moon->SetEnableLightShaftBloom(true);
		}
		else {
			Sun->SetCastShadows(true);
			Sun->SetEnableLightShaftBloom(true);

			Moon->SetCastShadows(false);
			Moon->SetEnableLightShaftBloom(false);
		}
	}

	if (hour != Calendar.Hour) {
		Cast<AGrid>(GetOwner())->Camera->CitizenManager->CheckWorkStatus(hour);
		Cast<AGrid>(GetOwner())->Camera->CitizenManager->CheckSleepStatus(hour);
		Cast<AGrid>(GetOwner())->Camera->CitizenManager->IssuePensions(hour);

		Cast<AGrid>(GetOwner())->Camera->ResourceManager->SetTrendOnHour(hour);

		SetDisplayText(hour);
	}
}

void UAtmosphereComponent::ChangeWindDirection()
{
	float yaw = FMath::FRandRange(0.0f, 360.0f);

	WindRotation = FRotator(0.0f, yaw, 0.0f);

	WindComponent->SetRelativeRotation(WindRotation + FRotator(0.0f, 180.0f, 0.0f));

	int32 time = FMath::RandRange(180.0f, 600.0f);

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ChangeWindDirection";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), time, info);
}

void UAtmosphereComponent::SetWindDimensions(int32 Bound)
{
	WindComponent->SetVariableVec3("Dimensions", FVector(Bound * 100.0f * 2.0f, Bound * 100.0f * 2.0f, 3000.0f));
	WindComponent->SetVariableFloat("SpawnRate", Bound / 100.0f);
}

void UAtmosphereComponent::SetDisplayText(int32 Hour)
{
	Calendar.Hour = Hour;

	FString period = Calendar.Period;

	if (Hour == 0)
		Calendar.NextDay();

	if (period != Calendar.Period) {
		Cast<AGrid>(GetOwner())->Camera->DisplayEvent("Season", Calendar.Period);

		Cast<AGrid>(GetOwner())->SetSeasonAffect(Calendar.Period, 0.02f);
	}

	Cast<AGrid>(GetOwner())->Camera->CitizenManager->ExecuteEvent(Calendar.Period, Calendar.Days[Calendar.Index], Calendar.Hour);

	Cast<AGrid>(GetOwner())->Camera->UpdateDayText();
}