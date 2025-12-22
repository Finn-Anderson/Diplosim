#include "Player/Managers/ArmyManager.h"

#include "Components/WidgetComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/AttackComponent.h"
#include "AI/AIMovementComponent.h"
#include "AI/BuildingComponent.h"
#include "Buildings/Misc/Broch.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/AIBuildComponent.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Universal/HealthComponent.h"

UArmyManager::UArmyManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerSelectedArmyIndex = -1;
}

void UArmyManager::EvaluateAIArmy(FFactionStruct* Faction)
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
		FFactionStruct* f = Camera->ConquestManager->GetFaction(name);

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

bool UArmyManager::CanJoinArmy(ACitizen* Citizen)
{
	FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(Citizen);

	if (Citizen->HealthComponent->GetHealth() == 0 || Camera->DiseaseManager->Injured.Contains(Citizen) || Camera->DiseaseManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->PoliticsManager->GetLawValue(faction.Name, "Work Age") || !Citizen->BuildingComponent->WillWork() || IsCitizenInAnArmy(Citizen))
		return false;

	return true;
}

void UArmyManager::CreateArmy(FString FactionName, TArray<ACitizen*> Citizens, bool bGroup, bool bLoad)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

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

void UArmyManager::AddToArmy(int32 Index, TArray<ACitizen*> Citizens)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizens[0]);

	faction->Armies[Index].Citizens.Append(Citizens);

	if (faction->Armies[Index].bGroup)
		for (ACitizen* citizen : Citizens)
			citizen->AIController->AIMoveTo(faction->EggTimer);
	else
		MoveToTarget(faction, Citizens);

	Camera->UpdateArmyCountUI(Index, faction->Armies[Index].Citizens.Num());
}

void UArmyManager::RemoveFromArmy(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

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

void UArmyManager::UpdateArmy(FString FactionName, int32 Index, TArray<ACitizen*> AlteredCitizens)
{
	if (AlteredCitizens.IsEmpty())
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (ACitizen* citizen : AlteredCitizens) {
		if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() == 0)
			return;

		if (faction->Armies[Index].Citizens.Contains(citizen))
			faction->Armies[Index].Citizens.Remove(citizen);
		else
			faction->Armies[Index].Citizens.Add(citizen);
	}
}

bool UArmyManager::IsCitizenInAnArmy(ACitizen* Citizen)
{
	FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(Citizen);

	for (FArmyStruct army : faction.Armies)
		if (army.Citizens.Contains(Citizen))
			return true;

	return false;
}

void UArmyManager::DestroyArmy(FString FactionName, int32 Index)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->Armies[Index].WidgetComponent->DestroyComponent();
	faction->Armies.RemoveAt(Index);

	if (PlayerSelectedArmyIndex == Index)
		SetSelectedArmy(INDEX_NONE);
}

TArray<ABuilding*> UArmyManager::GetReachableTargets(FFactionStruct* Faction, ACitizen* Citizen)
{
	TArray<ABuilding*> targets;

	for (FString name : Faction->AtWar) {
		FFactionStruct* f = Camera->ConquestManager->GetFaction(name);

		ABuilding* building = MoveArmyMember(f, Citizen, true);

		if (IsValid(building))
			targets.Add(building);
	}

	return targets;
}

void UArmyManager::MoveToTarget(FFactionStruct* Faction, TArray<ACitizen*> Citizens)
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

void UArmyManager::StartRaid(FFactionStruct Faction, int32 Index)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	MoveToTarget(faction, faction->Armies[Index].Citizens);
}

ABuilding* UArmyManager::MoveArmyMember(FFactionStruct* Faction, AAI* AI, bool bReturn, ABuilding* CurrentTarget)
{
	ABuilding* target = CurrentTarget;

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

void UArmyManager::SetSelectedArmy(int32 Index)
{
	FFactionStruct* faction = nullptr;
	FArmyStruct* army = nullptr;

	if (PlayerSelectedArmyIndex != INDEX_NONE) {
		faction = Camera->ConquestManager->GetFaction(Camera->ColonyName);
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
		faction = Camera->ConquestManager->GetFaction(Camera->ColonyName);

	army = &faction->Armies[PlayerSelectedArmyIndex];

	Camera->UpdateArmyWidgetSelectionUI(army->WidgetComponent->GetWidget(), true);

	for (ACitizen* citizen : army->Citizens) {
		auto aihism = Camera->Grid->AIVisualiser->GetAIHISM(citizen);

		Camera->Grid->AIVisualiser->UpdateInstanceCustomData(aihism.Key, aihism.Value, 1, 1.0f);
	}
}

void UArmyManager::PlayerMoveArmy(FVector Location)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Camera->ColonyName);

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

					locations.Add(loc, Camera->ConquestManager->AIBuildComponent->CalculateTileDistance(Location, loc));
				}
			}

		}
	}

	Camera->ConquestManager->AIBuildComponent->SortTileDistances(locations);

	TArray<FVector> locs;
	locations.GenerateKeyArray(locs);

	for (ACitizen* citizen : faction->Armies[PlayerSelectedArmyIndex].Citizens) {
		if (!citizen->AIController->CanMoveTo(locs[0]))
			continue;

		citizen->AIController->AIMoveTo(Camera->Grid, locs[0]);

		locs.RemoveAt(0);
	}
}