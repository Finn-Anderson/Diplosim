#include "DebugManager.h"

#include "Components/WidgetComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Player/Camera.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"

UDebugManager::UDebugManager()
{
	bInstantBuildCheat = false;
	bInstantEnemies = false;
	bRain = false;
	bFight = false;
}

void UDebugManager::SpawnEnemies()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	bInstantEnemies = true;

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->StartRaid();
}

void UDebugManager::AddEnemies(FString Category, int32 Amount)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	TSubclassOf<AResource> resource;

	for (FResourceStruct resourceStruct : camera->ResourceManager->ResourceList) {
		if (resourceStruct.Category != Category)
			continue;

		resource = resourceStruct.Type;

		break;
	}

	gamemode->TallyEnemyData(resource, Amount * 200);
}

void UDebugManager::ChangeSeasonAffect(FString Season)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	camera->ShowEvent("Season", Season);
	camera->Grid->AtmosphereComponent->SetSeasonAffect(Season, 0.02f);
}

void UDebugManager::SetRain(bool bChance)
{
	bRain = bChance;
}

void UDebugManager::SetWindSpeed(int32 Speed)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	camera->Grid->AtmosphereComponent->WindSpeed = Speed;
}

void UDebugManager::AddResearch(float Amount)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	camera->ResearchManager->Research(Amount, camera->ColonyName);
}

void UDebugManager::TurnOnInstantBuild(bool Value)
{
	bInstantBuildCheat = Value;
}

void UDebugManager::FillStorage()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();
	AActor* actor = camera->AttachedTo.Actor;

	if (!IsValid(actor) || !actor->IsA<ABuilding>())
		return;

	ABuilding* building = Cast<ABuilding>(actor);
	TArray<TSubclassOf<AResource>> resources = camera->ResourceManager->GetResources(building);

	for (TSubclassOf<AResource> resource : resources)
		camera->ResourceManager->AddLocalResource(resource, building, 1000000000);
}

void UDebugManager::SpawnCitizen(int32 Amount, int32 Age)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	FFactionStruct* faction = camera->ConquestManager->GetFaction(camera->ColonyName);

	for (int32 i = 0; i < Amount; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(Cast<ABroch>(faction->EggTimer)->CitizenClass, FVector::Zero(), FRotator::ZeroRotator);

		if (!IsValid(citizen))
			continue;

		FTransform transform;
		transform.SetLocation(camera->MouseHitLocation);
		transform.SetScale3D(FVector(0.28f));
		camera->Grid->AIVisualiser->AddInstance(citizen, camera->Grid->AIVisualiser->HISMCitizen, transform);

		citizen->BioComponent->SetSex(faction->Citizens);
		citizen->CitizenSetup(faction);

		if (Age >= 5)
			citizen->GivePersonalityTrait(citizen->BioComponent->Mother.Get());
		if (Age == 10)
			citizen->GivePersonalityTrait(citizen->BioComponent->Father.Get());

		if (Age > 18) {
			citizen->BioComponent->MaxHealthBeforeOld = citizen->HealthComponent->MaxHealth;
			citizen->BioComponent->SpeedBeforeOld = citizen->MovementComponent->InitialSpeed;

			citizen->BioComponent->SetSexuality(faction->Citizens);

			citizen->SetReligion(faction);
		}

		int32 limit = FMath::Min(18, Age);

		float healthIncrement = limit * 5 * citizen->HealthComponent->HealthMultiplier;
		citizen->HealthComponent->MaxHealth += healthIncrement;
		citizen->HealthComponent->AddHealth(healthIncrement);

		float multiply = 1.0f;
		if (citizen->Hunger <= 25)
			multiply = 0.75f;

		float scale = (limit * 0.04f) + 0.28f;

		citizen->MovementComponent->Transform.SetScale3D(FVector(scale, scale, scale) * FVector(multiply, multiply, 1.0f) * citizen->ReachMultiplier);

		citizen->BioComponent->Age = Age - 1;
		citizen->BioComponent->Birthday();
	}
}

void UDebugManager::SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	EEventType type;
	TSubclassOf<ABuilding> building = nullptr;

	if (Type == "Holliday")
		type = EEventType::Holliday;
	else if (Type == "Festival") {
		type = EEventType::Festival;
		building = AFestival::StaticClass();
	}
	else if (Type == "Mass")
		type = EEventType::Mass;
	else
		type = EEventType::Protest;

	TArray<int32> hours;
	for (int32 i = StartHour; i < EndHour; i++)
		hours.Add(i);

	camera->EventsManager->CreateEvent(camera->ColonyName, type, building, nullptr, Period, Day, hours, bRecurring, {}, bFireFestival);
}

void UDebugManager::DamageActor(int32 Amount)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();
	AActor* actor = camera->AttachedTo.Actor;

	if (IsValid(camera->FocusedCitizen))
		actor = camera->FocusedCitizen;

	if (!IsValid(actor) || (!actor->IsA<AAI>() && !actor->IsA<ABuilding>()))
		return;

	UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
	healthComp->TakeHealth(Amount, camera);
}

void UDebugManager::SetOnFire()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	camera->Grid->AtmosphereComponent->SetOnFire(camera->AttachedTo.Actor, camera->AttachedTo.Instance);
}

void UDebugManager::GiveProblem(bool bInjury)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	AActor* actor = camera->AttachedTo.Actor;

	if (bInjury)
		if (IsValid(actor) && actor->IsA<ACitizen>())
			camera->DiseaseManager->Injure(Cast<ACitizen>(actor), 0);
	else
		camera->DiseaseManager->SpawnDisease(camera);
}

void UDebugManager::CauseNaturalDisaster()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	UNaturalDisasterComponent* ndcomp = camera->Grid->AtmosphereComponent->NaturalDisasterComponent;
	ndcomp->DisasterChance = 100.0f;
	ndcomp->IncrementDisasterChance();
}

void UDebugManager::SetFight(bool bChance)
{
	bFight = bChance;
}