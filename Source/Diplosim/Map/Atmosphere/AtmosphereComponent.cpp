#include "AtmosphereComponent.h"
#include "Engine/DirectionalLight.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
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

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetDynamicShadowDistanceMovableLight(20000.0f);
	Sun->SetCascadeDistributionExponent(1.0f);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetCastShadows(true);
	Sun->ForwardShadingPriority = 0;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Moon->SetDynamicShadowDistanceMovableLight(20000.0f);
	Moon->SetCascadeDistributionExponent(1.0f);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetCastShadows(false);
	Moon->SetIntensity(0.5f);
	Moon->ForwardShadingPriority = 1;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetFogDensity(0.05f);
	Fog->SetSecondFogDensity(0.05f);
	Fog->SetSecondFogHeightOffset(5000.0f);
}

void UAtmosphereComponent::BeginPlay()
{
	Super::BeginPlay();

	ChangeWindDirection();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Atmosphere = this;

	if (!settings->GetRenderFog())
		Fog->SetHiddenInGame(true);
}

void UAtmosphereComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Sun->AddLocalRotation(FRotator(-Speed, Speed, 0.0f));
	Moon->AddLocalRotation(FRotator(Speed, Speed, 0.0f));

	int32 hour = FMath::Abs(FMath::FloorToInt(24.0f / 360.0f * (Moon->GetRelativeRotation().Yaw + 180.0f))) - 18;

	if (hour < 0)
		hour += 24;

	if (Calendar.Hour != hour && (hour == 6 || hour == 18)) {
		TArray<AActor*> citizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

		for (AActor* Actor : citizens)
			Cast<ACitizen>(Actor)->SetTorch(hour);

		if (hour == 18) {
			Sun->SetCastShadows(false);
			Moon->SetCastShadows(true);
		}
		else {
			Sun->SetCastShadows(true);
			Moon->SetCastShadows(false);
		}
	}

	if (hour != Calendar.Hour) {
		Cast<AGrid>(GetOwner())->Camera->CitizenManager->CheckWorkStatus(hour);

		SetDisplayText(hour);
	}
}

void UAtmosphereComponent::ChangeWindDirection()
{
	float yaw = FMath::FRandRange(0.0f, 360.0f);

	WindRotation = FRotator(0.0f, yaw, 0.0f);

	int32 time = FMath::RandRange(180.0f, 600.0f);

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ChangeWindDirection";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), time, info);
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