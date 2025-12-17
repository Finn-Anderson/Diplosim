#include "Player/Managers/ConquestManager.h"

#include "Components/WidgetComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/Citizen.h"
#include "AI/Clone.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Gate.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Components/AIBuildComponent.h"
#include "Player/Components/DiplomacyComponent.h"
#include "Universal/HealthComponent.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	AIBuildComponent = CreateDefaultSubobject<UAIBuildComponent>(TEXT("AIBuildComponent"));

	DiplomacyComponent = CreateDefaultSubobject<UDiplomacyComponent>(TEXT("DiplomacyComponent"));

	AINum = 2;
	MaxAINum = 2;
	PlayerSelectedArmyIndex = -1;
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
	if (Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->FactionName == "")
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

		EvaluateAIArmy(&faction);
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

		bool bEnemies = false;

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

			if (!faction.Rebels.IsEmpty())
				bEnemies = true;
		}

		ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

		ais.Add("Enemies", gamemode->Enemies);

		if (!gamemode->Enemies.IsEmpty())
			bEnemies = true;

		if (!bEnemies)
			return;

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

					if (ai->AttackComponent->OverlappingEnemies.Num() == 1)
						ai->AttackComponent->SetComponentTickEnabled(true);
				}
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

							if (tower->AttackComponent->OverlappingEnemies.Num() == 1)
								tower->AttackComponent->SetComponentTickEnabled(true);
						}
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

//
// Army
//
bool UConquestManager::CanJoinArmy(ACitizen* Citizen)
{
	FFactionStruct faction = GetCitizenFaction(Citizen);

	if (Citizen->HealthComponent->GetHealth() == 0 || Camera->DiseaseManager->Injured.Contains(Citizen) || Camera->DiseaseManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->PoliticsManager->GetLawValue(faction.Name, "Work Age") || !Citizen->WillWork() || IsCitizenInAnArmy(Citizen))
		return false;

	return true;
}

void UConquestManager::CreateArmy(FString FactionName, TArray<ACitizen*> Citizens, bool bGroup, bool bLoad)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = GetFaction(FactionName);

	FArmyStruct army;
	army.bGroup = bGroup;
	int32 index = faction->Armies.Add(army);

	AddToArmy(index, Citizens);

	UWidgetComponent* widgetComponent = NewObject<UWidgetComponent>(this, UWidgetComponent::StaticClass());
	widgetComponent->SetupAttachment(Camera->Grid->GetRootComponent());
	widgetComponent->SetRelativeLocation(Camera->GetTargetActorLocation(Citizens[0]) + FVector(0.0f, 0.0f, 300.0f));
	widgetComponent->SetWidgetSpace(EWidgetSpace::World);
	widgetComponent->SetDrawSize(FVector2D(0.0f));
	widgetComponent->SetWidgetClass(ArmyUI);
	widgetComponent->RegisterComponent();

	faction->Armies[index].WidgetComponent = widgetComponent;

	Camera->SetArmyWidgetUI(FactionName, widgetComponent->GetWidget(), index);

	if (FactionName != Camera->ColonyName && !bLoad) {
		FString id = FactionName + FString::FromInt(index) + "ArmyRaidTimer";

		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(*faction, params);
		Camera->TimerManager->SetParameter(index, params);
		Camera->TimerManager->CreateTimer(id, Camera, 60.0f, "StartRaid", params, false);
	}
}

void UConquestManager::AddToArmy(int32 Index, TArray<ACitizen*> Citizens)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = GetFaction("", Citizens[0]);

	faction->Armies[Index].Citizens.Append(Citizens);

	if (faction->Armies[Index].bGroup)
		for (ACitizen* citizen : Citizens)
			citizen->AIController->AIMoveTo(faction->EggTimer);
	else
		MoveToTarget(faction, Citizens);

	Camera->UpdateArmyCountUI(Index, faction->Armies[Index].Citizens.Num());
}

void UConquestManager::RemoveFromArmy(ACitizen* Citizen)
{
	FFactionStruct* faction = GetFaction("", Citizen);

	for (int32 i = faction->Armies.Num() - 1; i > -1; i--) {
		if (!faction->Armies[i].Citizens.Contains(Citizen))
			continue;

		faction->Armies[i].Citizens.Remove(Citizen);

		Camera->UpdateArmyCountUI(i, faction->Armies[i].Citizens.Num());

		if (faction->Armies[i].Citizens.IsEmpty())
			faction->Armies.RemoveAt(i);

		break;
	}
}

void UConquestManager::UpdateArmy(FString FactionName, int32 Index, TArray<ACitizen*> AlteredCitizens)
{
	if (AlteredCitizens.IsEmpty())
		return;

	FFactionStruct* faction = GetFaction(FactionName);

	for (ACitizen* citizen : AlteredCitizens) {
		if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() == 0)
			return;

		if (faction->Armies[Index].Citizens.Contains(citizen))
			faction->Armies[Index].Citizens.Remove(citizen);
		else
			faction->Armies[Index].Citizens.Add(citizen);
	}
}

bool UConquestManager::IsCitizenInAnArmy(ACitizen* Citizen)
{
	FFactionStruct faction = GetCitizenFaction(Citizen);

	for (FArmyStruct army : faction.Armies)
		if (army.Citizens.Contains(Citizen))
			return true;

	return false;
}

void UConquestManager::DestroyArmy(FString FactionName, int32 Index)
{
	FFactionStruct* faction = GetFaction(FactionName);

	faction->Armies[Index].WidgetComponent->DestroyComponent();
	faction->Armies.RemoveAt(Index);

	if (PlayerSelectedArmyIndex == Index)
		SetSelectedArmy(INDEX_NONE);
}

TArray<ABuilding*> UConquestManager::GetReachableTargets(FFactionStruct* Faction, ACitizen* Citizen)
{
	TArray<ABuilding*> targets;

	for (FString name : Faction->AtWar) {
		FFactionStruct* f = GetFaction(name);

		ABuilding* building = MoveArmyMember(f, Citizen, true);

		if (IsValid(building))
			targets.Add(building);
	}

	return targets;
}

void UConquestManager::MoveToTarget(FFactionStruct* Faction, TArray<ACitizen*> Citizens)
{
	TArray<ABuilding*> targets = GetReachableTargets(Faction, Citizens[0]);

	if (targets.IsEmpty()) {
		for (ACitizen* citizen : Citizens)
			RemoveFromArmy(citizen);

		return;
	}

	int32 index = Camera->Stream.RandRange(0, targets.Num() - 1);

	for (ACitizen* citizen : Citizens)
		citizen->AIController->AIMoveTo(targets[index]);
}

void UConquestManager::StartRaid(FFactionStruct Faction, int32 Index)
{
	FFactionStruct* faction = GetFaction(Faction.Name);

	MoveToTarget(faction, faction->Armies[Index].Citizens);
}

void UConquestManager::EvaluateAIArmy(FFactionStruct* Faction)
{
	if (Faction->AtWar.IsEmpty()) {
		if (!Faction->Armies.IsEmpty())
			for (int32 i = Faction->Armies.Num() - 1; i > -1; i--)
				DestroyArmy(Faction->Name, i);

		return;
	}

	int32 currentCitizens = Faction->Citizens.Num();
	int32 enemyCitizens = 0;

	for (FString name : Faction->AtWar) {
		FFactionStruct* f = GetFaction(name);

		enemyCitizens += f->Citizens.Num();
	}

	int32 diff = currentCitizens - enemyCitizens;

	TArray<ACitizen*> availableCitizens;
	TArray<ACitizen*> chosenCitizens;

	for (ACitizen* citizen : Faction->Citizens) {
		if (!CanJoinArmy(citizen) || GetReachableTargets(Faction, citizen).IsEmpty())
			continue;

		availableCitizens.Add(citizen);
	}

	if (!availableCitizens.IsEmpty()) {
		for (int32 i = 0; i < diff; i++) {
			int32 index = Camera->Stream.RandRange(0, availableCitizens.Num() - 1);

			chosenCitizens.Add(availableCitizens[index]);
			availableCitizens.RemoveAt(index);
		}

		if (currentCitizens > enemyCitizens) {
			if (Faction->Armies.Num() == 5 || (diff < 20 && !Faction->Armies.IsEmpty())) {
				int32 index = Camera->Stream.RandRange(0, Faction->Armies.Num() - 1);

				AddToArmy(index, chosenCitizens);
			}
			else {
				CreateArmy(Faction->Name, chosenCitizens);
			}
		}
	}

	for (FArmyStruct army : Faction->Armies) {
		for (ACitizen* citizen : army.Citizens) {
			if (IsValid(citizen->AIController->MoveRequest.GetGoalActor()) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
				continue;

			MoveToTarget(Faction, { citizen });
		}
	}
}

ABuilding* UConquestManager::MoveArmyMember(FFactionStruct* Faction, AAI* AI, bool bReturn)
{
	ABuilding* target = nullptr;

	for (ABuilding* building : Faction->Buildings) {
		if (!IsValid(building) || building->HealthComponent == 0 || !AI->AIController->CanMoveTo(building->GetActorLocation()))
			continue;

		if (!IsValid(target)) {
			target = building;

			if (building->IsA<ABroch>())
				break;
			else
				continue;
		}

		int32 targetValue = 1.0f;
		int32 buildingValue = 1.0f;

		if (target->IsA<ABroch>())
			targetValue = 5.0f;

		if (building->IsA<ABroch>())
			buildingValue = 5.0f;

		double magnitude = AI->AIController->GetClosestActor(400.0f, AI->MovementComponent->Transform.GetLocation(), target->GetActorLocation(), building->GetActorLocation(), true, targetValue, buildingValue);

		if (magnitude > 0.0f)
			target = building;
	}

	if (bReturn)
		return target;
	else if (IsValid(target))
		AI->AIController->AIMoveTo(target);

	if (AI->IsA<ACitizen>())
		RemoveFromArmy(Cast<ACitizen>(AI));

	return nullptr;
}

void UConquestManager::SetSelectedArmy(int32 Index)
{
	FFactionStruct* faction = nullptr;
	FArmyStruct* army = nullptr;

	if (PlayerSelectedArmyIndex != INDEX_NONE) {
		faction = GetFaction(Camera->ColonyName);
		army = &faction->Armies[PlayerSelectedArmyIndex];

		Camera->UpdateArmyWidgetSelectionUI(army->WidgetComponent->GetWidget(), false);

		for (ACitizen* citizen : army->Citizens) {
			auto aihism = Camera->Grid->AIVisualiser->GetAIHISM(citizen);

			Camera->Grid->AIVisualiser->UpdateInstanceCustomData(aihism.Key, aihism.Value, 1, 0.0f);
		}
	}

	PlayerSelectedArmyIndex = Index;

	if (Index == INDEX_NONE)
		return;

	if (faction == nullptr)
		faction = GetFaction(Camera->ColonyName);

	army = &faction->Armies[PlayerSelectedArmyIndex];

	Camera->UpdateArmyWidgetSelectionUI(army->WidgetComponent->GetWidget(), true);
	
	for (ACitizen* citizen : army->Citizens) {
		auto aihism = Camera->Grid->AIVisualiser->GetAIHISM(citizen);

		Camera->Grid->AIVisualiser->UpdateInstanceCustomData(aihism.Key, aihism.Value, 1, 1.0f);
	}
}

void UConquestManager::PlayerMoveArmy(FVector Location)
{
	FFactionStruct* faction = GetFaction(Camera->ColonyName);

	TMap<FVector, double> locations;

	for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.bMineral)
				continue;

			FTransform transform = Camera->Grid->GetTransform(&tile);

			for (int32 x = -1; x < 2; x += 2) {
				for (int32 y = -1; y < 2; y += 2) {
					FVector offset = FVector(x * 25.0f, y * 25.0f, 0.0f);
					FVector loc = transform.GetLocation() + offset;

					locations.Add(loc, AIBuildComponent->CalculateTileDistance(Location, loc));
				}
			}

		}
	}

	AIBuildComponent->SortTileDistances(locations);

	TArray<FVector> locs;
	locations.GenerateKeyArray(locs);

	for (ACitizen* citizen : faction->Armies[PlayerSelectedArmyIndex].Citizens) {
		if (!citizen->AIController->CanMoveTo(locs[0]))
			continue;

		citizen->AIController->AIMoveTo(Camera->Grid, locs[0]);

		locs.RemoveAt(0);
	}
}

//
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}