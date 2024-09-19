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

	bNight = false;

	Speed = 0.01f;

	Day = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetRealTimeCaptureEnabled(true);
	SkyLight->SetCastShadows(false);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetCastShadows(true);
	Sun->ForwardShadingPriority = 0;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetCastShadows(false);
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
	
	FTimerStruct timer;
	timer.CreateTimer(GetOwner(), 1500.0f, FTimerDelegate::CreateUObject(this, &UAtmosphereComponent::AddDay), true);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	camera->CitizenManager->Timers.Add(timer);

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Atmosphere = this;

	if (!settings->GetRenderFog())
		Fog->SetHiddenInGame(true);

	SetSunStatus(settings->GetSunPosition());
}

void UAtmosphereComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Sun->AddLocalRotation(FRotator(0.0f, -Speed, 0.0f));
	Moon->SetRelativeRotation(Sun->GetRelativeRotation() + FRotator(180.0f, 0.0f, 0.0f));

	float pitch = FMath::RoundHalfFromZero((Sun->GetRelativeRotation().Pitch * 100.0f)) / 100.0f;

	if (pitch == 0.0f) {
		TArray<AActor*> citizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

		bNight = !bNight;

		for (AActor* Actor : citizens)
			Cast<ACitizen>(Actor)->SetTorch();

		if (bNight) {
			Sun->SetCastShadows(false);
			Moon->SetCastShadows(true);
		}
		else {
			Sun->SetCastShadows(true);
			Moon->SetCastShadows(false);
		}
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

void UAtmosphereComponent::SetSunStatus(FString Value)
{
	if (Value == "Cycle") {
		SetComponentTickEnabled(true);
	}
	else {
		SetComponentTickEnabled(false);

		FRotator rotation = FRotator(130.0f, 0.0f, 264.0f);

		bNight = false;

		Sun->SetCastShadows(true);
		Moon->SetCastShadows(false);

		if (Value == "Morning") {
			rotation.Pitch = -15.0f;
		}
		else if (Value == "Noon") {
			rotation.Pitch = -89.0f;
		}
		else if (Value == "Evening") {
			rotation.Pitch = -165.0f;
		}
		else {
			rotation.Pitch = -269.0f;

			bNight = true;

			Sun->SetCastShadows(false);
			Moon->SetCastShadows(true);
		}

		TArray<AActor*> citizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

		for (AActor* Actor : citizens)
			Cast<ACitizen>(Actor)->SetTorch();

		Sun->SetRelativeRotation(rotation);
		Moon->SetRelativeRotation(rotation + FRotator(180.0f, 0.0f, 0.0f));
	}
}

void UAtmosphereComponent::AddDay()
{
	Day++;

	Cast<AGrid>(GetOwner())->Camera->UpdateDayText();
}