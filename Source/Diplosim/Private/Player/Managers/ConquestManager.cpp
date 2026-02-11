#include "Player/Managers/ConquestManager.h"

#include "Components/SphereComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/Clone.h"
#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Gate.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Components/AIBuildComponent.h"
#include "Player/Components/DiplomacyComponent.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	AIBuildComponent = CreateDefaultSubobject<UAIBuildComponent>(TEXT("AIBuildComponent"));

	DiplomacyComponent = CreateDefaultSubobject<UDiplomacyComponent>(TEXT("DiplomacyComponent"));

	AINum = 2;
	MaxAINum = 2;
}

FFactionStruct UConquestManager::InitialiseFaction(FString Name)
{
	FFactionStruct faction;
	faction.Name = Name;

	faction.ResearchStruct = Camera->ResearchManager->InitResearchStruct;

	faction.Politics.Parties = Camera->PoliticsManager->InitParties;
	faction.Politics.Laws = Camera->PoliticsManager->InitLaws;
	faction.Events = Camera->EventsManager->InitEvents;

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		FFactionResourceStruct factionResource;
		factionResource.Type = resource.Type;

		faction.Resources.Add(factionResource);
	}

	for (TSubclassOf<ABuilding> buildingClass : BuildingClassDefaultAmount)
		faction.BuildingClassAmount.Add(buildingClass, Cast<ABuilding>(buildingClass->GetDefaultObject())->DefaultAmount);

	return faction;
}

void UConquestManager::CreatePlayerFaction()
{
	Factions.Add(InitialiseFaction(Camera->ColonyName));
}

void UConquestManager::FinaliseFactions(ABroch* EggTimer)
{
	Factions[0].Name = Camera->ColonyName;
	Factions[0].EggTimer = EggTimer;
	EggTimer->FactionName = Camera->ColonyName;

	TArray<FString> factions;

	FString factionNames;
	FFileHelper::LoadFileToString(factionNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/ColonyNames.txt"));

	TArray<FString> factionsParsed;
	factionNames.ParseIntoArray(factionsParsed, TEXT(","));

	for (int32 i = factionsParsed.Num() - 1; i > -1; i--)
		if (factions.Contains(factionsParsed[i]))
			factions.RemoveAt(i);

	TArray<FTileStruct*> validLocations;

	for (int32 i = 0; i < AINum; i++) {
		int32 index = Camera->Stream.RandRange(0, factionsParsed.Num() - 1);
		factions.Add(factionsParsed[index]);

		if (factionsParsed[index] != "")
			factionsParsed.RemoveAt(index);
	}

	for (FString name : factions) {
		FFactionStruct f = InitialiseFaction(name);

		FActorSpawnParameters params;
		params.bNoFail = true;

		ABroch* eggTimer = GetWorld()->SpawnActor<ABroch>(EggTimer->GetClass(), FVector::Zero(), FRotator::ZeroRotator, params);

		for (TArray<FTileStruct*> tiles : Camera->Grid->ValidMineralTiles) {
			FTileStruct* tile = tiles[0];

			eggTimer->SetActorLocation(Camera->Grid->GetTransform(tile).GetLocation());

			if (!Camera->BuildComponent->IsValidLocation(eggTimer))
				continue;

			bool bTooCloseToAnotherFaction = false;

			for (FFactionStruct& faction : Factions) {
				double dist = FVector::Dist(faction.EggTimer->GetActorLocation(), eggTimer->GetActorLocation());

				if (dist > 3000.0f)
					continue;

				bTooCloseToAnotherFaction = true;

				break;
			}

			if (!bTooCloseToAnotherFaction)
				validLocations.Add(tile);
		}

		int32 index = Camera->Stream.RandRange(0, validLocations.Num() - 1);
		FTileStruct* chosenTile = validLocations[index];

		eggTimer->SetActorLocation(Camera->Grid->GetTransform(chosenTile).GetLocation());
		eggTimer->FactionName = f.Name;

		Camera->BuildComponent->SetTreeStatus(eggTimer, true);

		f.EggTimer = eggTimer;
		f.Buildings.Add(eggTimer);

		Factions.Add(f);
	}

	for (FFactionStruct& faction : Factions) {
		FString type = "Good";

		if (faction.Name != Camera->ColonyName) {
			AIBuildComponent->InitialiseTileLocationDistances(&faction);

			type = "Neutral";
		}

		Camera->NotifyLog(type, faction.Name + " was just founded", faction.Name);

		faction.EggTimer->SpawnCitizens();

		DiplomacyComponent->SetFactionFlagColour(&faction);
		DiplomacyComponent->SetFactionCulture(&faction);
	}

	Camera->SetFactionsInDiplomacyUI();
}

ABuilding* UConquestManager::DoesFactionContainUniqueBuilding(FString FactionName, TSubclassOf<class ABuilding> BuildingClass)
{
	FFactionStruct faction = GetFactionFromName(FactionName);

	for (ABuilding* building : faction.Buildings) {
		if (!building->IsA(BuildingClass))
			continue;

		return building;
	}

	return nullptr;
}

TArray<class ABuilding*> UConquestManager::GetFactionBuildingsFromClass(TSubclassOf<class ABuilding> BuildingClass)
{
	TArray<class ABuilding*> buildings;

	for (ABuilding* building : GetFaction(Camera->ColonyName)->Buildings) {
		if (building->GetClass() != BuildingClass)
			continue;

		buildings.Add(building);
	}

	return buildings;
}

int32 UConquestManager::GetFactionIndexFromName(FString FactionName)
{
	for (int32 i = 0; i < Factions.Num(); i++) {
		if (Factions[i].Name != FactionName)
			continue;

		return i;
	}

	UE_LOGFMT(LogTemp, Fatal, "Faction not found");
}

FFactionStruct UConquestManager::GetFactionFromName(FString FactionName)
{
	return Factions[GetFactionIndexFromName(FactionName)];
}

TArray<ACitizen*> UConquestManager::GetCitizensFromFactionName(FString FactionName)
{
	return GetFactionFromName(FactionName).Citizens;
}

FPoliticsStruct UConquestManager::GetFactionPoliticsStruct(FString FactionName)
{
	FFactionStruct faction = GetFactionFromName(FactionName);

	return faction.Politics;
}

FFactionStruct UConquestManager::GetFactionFromActor(AActor* Actor)
{
	FFactionStruct faction;

	for (FFactionStruct f : Factions) {
		if (!DoesFactionContainActor(f.Name, Actor))
			continue;

		faction = f;

		break;
	}

	return faction;
}

bool UConquestManager::DoesFactionContainActor(FString FactionName, AActor* Actor)
{
	if (Actor->IsA<ABuilding>() && (Cast<ABuilding>(Actor)->FactionName == "" || Camera->ConstructionManager->IsFactionConstructingBuilding(FactionName, Cast<ABuilding>(Actor))))
		return true;

	FFactionStruct faction = GetFactionFromName(FactionName);

	return faction.Buildings.Contains(Actor) || faction.Citizens.Contains(Actor) || faction.Clones.Contains(Actor) || faction.Rebels.Contains(Actor);
}

void UConquestManager::ComputeAI()
{
	FScopeTryLock lock(&ConquestLock);
	if (!lock.IsLocked())
		return;

	for (FFactionStruct& faction : Factions) {
		if (faction.AtWar.IsEmpty() && faction.WarFatigue > 0)
			faction.WarFatigue--;
		else if (!faction.AtWar.IsEmpty())
			faction.WarFatigue++;

		DiplomacyComponent->SetFactionsHappiness(faction);

		if (faction.Name == Camera->ColonyName)
			continue;

		DiplomacyComponent->SetFactionCulture(&faction);

		Camera->UpdateFactionHappiness();

		DiplomacyComponent->EvaluateDiplomacy(faction);

		AIBuildComponent->EvaluateAIBuild(&faction);

		Camera->ArmyManager->EvaluateAIArmy(&faction);

		Camera->CitizenManager->AISetupRadioTowerBroadcasts(&faction);
	}

	for (FFactionStruct faction : FactionsToRemove) {
		for (FFactionStruct& f : Factions) {
			if (f == faction)
				continue;

			for (int32 i = f.Happiness.Num() - 1; i > -1; i--)
				if (f.Happiness[i].Owner == faction.Name)
					f.Happiness.RemoveAt(i);
		}

		int32 index = Factions.Find(faction);

		Camera->RemoveFactionBtn(index);

		Factions.RemoveAt(index);
	}

	FactionsToRemove.Empty();
}

void UConquestManager::CalculateAIFighting()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&AIFightLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Fighting);

			return;
		}

		TMap<FString, TArray<AAI*>> ais;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			if (!ais.Contains("Citizens"))
				ais.Add("Citizens");

			TArray<AAI*>* ai = ais.Find("Citizens");
			ai->Append(faction.Citizens);

			if (!ais.Contains("Clones"))
				ais.Add("Clones");

			ai = ais.Find("Clones");
			ai->Append(faction.Clones);

			if (!ais.Contains("Rebels"))
				ais.Add("Rebels");

			ai = ais.Find("Rebels");
			ai->Append(faction.Rebels);
		}

		ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

		ais.Add("Enemies", gamemode->Enemies);

		for (auto& element : ais) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			for (AAI* ai : element.Value) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(ai) || ai->HealthComponent->GetHealth() <= 0)
					continue;

				FOverlapsStruct requestedOverlaps;

				if (element.Key == "Citizens" || element.Key == "Clones")
					requestedOverlaps.GetCitizenEnemies();
				else if (element.Key == "Rebels")
					requestedOverlaps.GetRebelsEnemies();
				else
					requestedOverlaps.GetEnemyEnemies();

				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, ai, ai->Range, requestedOverlaps, EFactionType::Both);

				for (AActor* actor : actors) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					if (!IsValid(actor) || ai->AttackComponent->OverlappingEnemies.Contains(actor))
						continue;

					UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

					if ((healthComp && healthComp->GetHealth() <= 0) || (!*ai->AttackComponent->ProjectileClass && !ai->AIController->CanMoveTo(Camera->GetTargetActorLocation(actor))))
						continue;

					ai->AttackComponent->OverlappingEnemies.Add(actor);
				}

				if (!ai->AttackComponent->OverlappingEnemies.IsEmpty())
					ai->AttackComponent->PickTarget();
			}
		}
	});
}

void UConquestManager::CalculateBuildingFighting()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&BuildingFightLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Fighting);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			if (faction.Rebels.IsEmpty() && GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty())
				continue;

			for (ABuilding* building : faction.Buildings) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!building->IsA<AGate>() && !building->IsA<ATower>() && !building->IsA<ATrap>())
					continue;

				UHealthComponent* healthComp = building->GetComponentByClass<UHealthComponent>();

				if (healthComp->GetHealth() <= 0)
					continue;

				if (building->IsA<ATrap>()) {
					ATrap* trap = Cast<ATrap>(building);

					if (Camera->TimerManager->DoesTimerExist("Trap", trap))
						continue;

					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.GetCitizenEnemies();

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, building, trap->ActivateRange, requestedOverlaps, EFactionType::Both);

					if (!actors.IsEmpty())
						trap->StartTrapFuse();
				}
				else {
					USphereComponent* rangeComp = building->GetComponentByClass<USphereComponent>();

					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.GetCitizenEnemies();

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, building, rangeComp->GetUnscaledSphereRadius(), requestedOverlaps, EFactionType::Both);

					if (building->IsA<AGate>()) {
						AGate* gate = Cast<AGate>(building);

						if (actors.IsEmpty())
							gate->OpenGate();
						else
							gate->CloseGate();
					}
					else {
						ATower* tower = Cast<ATower>(building);

						for (AActor* actor : actors) {
							if (Camera->SaveGameComponent->IsLoading())
								return;

							if (!IsValid(actor) || tower->AttackComponent->OverlappingEnemies.Contains(actor))
								continue;

							UHealthComponent* hpComp = actor->GetComponentByClass<UHealthComponent>();

							if (hpComp && hpComp->GetHealth() <= 0)
								continue;

							tower->AttackComponent->OverlappingEnemies.Add(actor);
						}

						if (!tower->AttackComponent->OverlappingEnemies.IsEmpty())
							tower->AttackComponent->PickTarget();
					}
				}
			}
		}
	});
}

void UConquestManager::CheckLoadFactionLock()
{
	if (Camera->SaveGameComponent->IsLoading()) {
		FScopeTryLock lock(&Camera->ConquestManager->ConquestLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Faction);

			return;
		}
	}
}

FFactionStruct UConquestManager::GetCitizenFaction(ACitizen* Citizen)
{
	FFactionStruct faction;

	for (FFactionStruct f : Factions) {
		if (!f.Citizens.Contains(Citizen))
			continue;

		faction = f;

		break;
	}

	return faction;
}

FFactionStruct* UConquestManager::GetFaction(FString Name, AActor* Actor)
{
	FFactionStruct* faction = nullptr;

	for (FFactionStruct& f : Factions) {
		if (f.Name != Name && !f.Citizens.Contains(Actor) && !f.Buildings.Contains(Actor) && !f.Rebels.Contains(Actor) && !f.Clones.Contains(Actor))
			continue;

		faction = &f;

		break;
	}

	return faction;
}

float UConquestManager::GetBuildingClassAmount(FString FactionName, TSubclassOf<class ABuilding> BuildingClass)
{
	FFactionStruct* faction = GetFaction(FactionName);

	if (faction == nullptr)
		return Cast<ABuilding>(BuildingClass->GetDefaultObject())->DefaultAmount;

	return *faction->BuildingClassAmount.Find(BuildingClass);
}

//
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}