#include "DebugManager.h"

#include "Components/WidgetComponent.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "AI/Citizen.h"

UDebugManager::UDebugManager()
{
	bInstantBuildCheat = false;
	bInstantEnemies = false;
}

void UDebugManager::SpawnEnemies()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	if (camera->bInMenu)
		return;

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

	if (camera->bInMenu)
		return;

	camera->Grid->AtmosphereComponent->SetSeasonAffect(Season, 0.02f);
}

void UDebugManager::CompleteResearch()
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	if (camera->bInMenu)
		return;

	camera->ResearchManager->Research(100000000000.0f, camera->ColonyName);
}

void UDebugManager::TurnOnInstantBuild(bool Value)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	if (camera->bInMenu)
		return;

	bInstantBuildCheat = Value;
}

void UDebugManager::SpawnCitizen(int32 Amount, bool bAdult)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	if (camera->bInMenu)
		return;

	FFactionStruct* faction = camera->ConquestManager->GetFaction(camera->ColonyName);

	for (int32 i = 0; i < Amount; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(Cast<ABroch>(faction->EggTimer)->CitizenClass, camera->MouseHitLocation, FRotator(0.0f));
		citizen->CitizenSetup(faction);

		if (!bAdult || !IsValid(citizen))
			continue;

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);
	}
}

void UDebugManager::SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	if (camera->bInMenu)
		return;

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

	camera->CitizenManager->CreateEvent(camera->ColonyName, type, building, nullptr, Period, Day, hours, bRecurring, {}, bFireFestival);
}

void UDebugManager::DamageActor(int32 Amount)
{
	ACamera* camera = GetPlayerController()->GetPawn<ACamera>();

	AActor* actor = camera->WidgetComponent->GetAttachParentActor();

	if (IsValid(camera->FocusedCitizen))
		actor = camera->FocusedCitizen;

	if (!IsValid(actor) || (!actor->IsA<AAI>() && !actor->IsA<ABuilding>()))
		return;

	UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
	healthComp->TakeHealth(Amount, camera);
}