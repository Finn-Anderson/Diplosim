#include "AtmosphereComponent.h"
#include "Engine/DirectionalLight.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Resources/Vegetation.h"
#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Building.h"
#include "Clouds.h"
#include "NaturalDisasterComponent.h"

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
	Sun->SetDynamicShadowDistanceMovableLight(40000.0f);
	Sun->SetCascadeDistributionExponent(3.0f);
	Sun->SetCascadeTransitionFraction(0.3f);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetShadowSlopeBias(1.0f);
	Sun->SetShadowBias(1.0f);
	Sun->SetCastShadows(true);
	Sun->SetUseTemperature(true);
	Sun->SetTemperature(5000.0f);
	Sun->SetEnableLightShaftBloom(true);
	Sun->SetBloomMaxBrightness(0.5f);
	Sun->ForwardShadingPriority = 0;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetRelativeRotation(FRotator(15.0f, 195.0f, 0.0f));
	Moon->SetDynamicShadowDistanceMovableLight(40000.0f);
	Moon->SetCascadeDistributionExponent(3.0f);
	Moon->SetCascadeTransitionFraction(0.3f);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetShadowSlopeBias(1.0f);
	Moon->SetShadowBias(1.0f);
	Moon->SetCastShadows(false);
	Moon->SetIntensity(0.5f);
	Moon->SetUseTemperature(true);
	Moon->SetTemperature(5000.0f);
	Moon->SetEnableLightShaftBloom(false);
	Moon->SetBloomMaxBrightness(0.5f);
	Moon->ForwardShadingPriority = 1;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetMultiScatteringFactor(4.0f);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetFogDensity(0.02f);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogDistance(10000.0f);
	Fog->SetVolumetricFogExtinctionScale(4.0f);
	Fog->SetVolumetricFogDistance(20000.0f);

	Clouds = CreateDefaultSubobject<UCloudComponent>(TEXT("Clouds"));

	WindComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WindComponent"));
	WindComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2500.0f));
	WindComponent->bAutoActivate = true;

	NaturalDisasterComponent = CreateDefaultSubobject<UNaturalDisasterComponent>(TEXT("NaturalDisasterComponent"));

	bRedSun = false;
}

void UAtmosphereComponent::BeginPlay()
{
	Super::BeginPlay();

	Grid = Cast<AGrid>(GetOwner());

	Clouds->Grid = Grid;
	Clouds->NaturalDisasterComponent = NaturalDisasterComponent;

	NaturalDisasterComponent->Grid = Grid;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Atmosphere = this;

	if (!settings->GetRenderWind())
		WindComponent->Deactivate();

	Fog->SetVolumetricFog(settings->GetVolumetricFog());

	Sun->SetIntensity(settings->GetSunBrightness());
	Moon->SetIntensity(settings->GetMoonBrightness());

	float lightShaftIntensity = 0.0f;

	if (settings->GetLightShafts())
		lightShaftIntensity = 0.5f;

	Sun->SetBloomMaxBrightness(lightShaftIntensity);
	Moon->SetBloomMaxBrightness(lightShaftIntensity);
}

void UAtmosphereComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.009f || DeltaTime > 1.0f)
		return;

	Clouds->TickCloud(DeltaTime);

	if (Grid->Camera->Start)
		return;

	Sun->AddLocalRotation(FRotator(-Speed, Speed, 0.0f));
	Moon->AddLocalRotation(FRotator(Speed, Speed, 0.0f));

	int32 hour = FMath::Abs(FMath::FloorToInt(24.0f / 360.0f * (Moon->GetRelativeRotation().Yaw + 180.0f))) - 18;

	if (hour < 0)
		hour += 24;

	if (Calendar.Hour != hour && (hour == 6 || hour == 18)) {
		for (FFactionStruct& faction : Grid->Camera->ConquestManager->Factions)
			for (ABuilding* building : faction.Buildings)
				building->SetLights(hour);

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
		SetDisplayText(hour);

		Grid->Camera->CitizenManager->CheckWorkStatus(hour);
		Grid->Camera->CitizenManager->CheckCitizenStatus(hour);
		Grid->Camera->CitizenManager->IssuePensions(hour);
		Grid->Camera->CitizenManager->ItterateThroughSentences();

		if (hour == 6)
			Grid->Camera->CitizenManager->CheckUpkeepCosts();

		Grid->Camera->ResourceManager->SetTrendOnHour(hour);

		Grid->Camera->ConquestManager->ComputeAI();

		NaturalDisasterComponent->IncrementDisasterChance();
	}
}

void UAtmosphereComponent::ChangeWindDirection()
{
	float yaw = Grid->Stream.FRandRange(0.0f, 360.0f);

	WindRotation = FRotator(0.0f, yaw, 0.0f);

	WindComponent->SetRelativeRotation(WindRotation + FRotator(0.0f, 180.0f, 0.0f));

	int32 time = Grid->Stream.RandRange(180.0f, 600.0f);

	Grid->Camera->CitizenManager->CreateTimer("Wind", Grid, time, "ChangeWindDirection", {}, false);
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
		Grid->Camera->DisplayEvent("Season", Calendar.Period);

		Grid->SetSeasonAffect(Calendar.Period, 0.02f);
	}

	Grid->Camera->CitizenManager->ExecuteEvent(Calendar.Period, Calendar.Days[Calendar.Index], Calendar.Hour);

	Grid->Camera->UpdateDayText();
}

void UAtmosphereComponent::SetOnFire(AActor* Actor, int32 Index)
{
	UNiagaraComponent* fire = nullptr;
	UStaticMesh* mesh = nullptr;

	UHealthComponent* healthComp = Actor->GetComponentByClass<UHealthComponent>();

	if (Actor->IsA<AVegetation>()) {
		AVegetation* vegetation = Cast<AVegetation>(Actor);

		FTransform t;
		vegetation->ResourceHISM->GetInstanceTransform(Index, t);

		fire = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FireSystem, t.GetLocation(), t.GetRotation().Rotator(), FVector(1.0f), true, false);

		mesh = vegetation->ResourceHISM->GetStaticMesh();

		vegetation->OnFire(Index);
	}
	else if (healthComp) {
		if (Actor->IsA<AAI>()) {
			healthComp->TakeHealth(1000, GetOwner());
		}
		else {
			fire = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSystem, Actor->GetRootComponent(), "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);

			mesh = Actor->GetComponentByClass<UStaticMeshComponent>()->GetStaticMesh();

			healthComp->OnFire();
		}
	}

	if (IsValid(fire)) {
		UNiagaraFunctionLibrary::OverrideSystemUserVariableStaticMesh(fire, "Static Mesh", mesh);
		fire->SetVariableStaticMesh("Fire Mesh", mesh);

		fire->Activate();
	}
}