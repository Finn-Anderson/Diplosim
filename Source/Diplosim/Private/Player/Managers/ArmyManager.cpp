#include "Player/Managers/ArmyManager.h"

#include "NavigationSystem.h"

#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
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
#include "Universal/AttackComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UArmyManager::UArmyManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerSelectedArmyData = TTuple<FFactionStruct*, int32>(nullptr, INDEX_NONE);
	NumTiles = 0;
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

	for (int32 i = 0; i < Faction->Armies.Num(); i++) {
		for (ACitizen* citizen : Faction->Armies[i].Citizens) {
			if (IsValid(citizen->AIController->MoveRequest.GetGoalActor()) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
				continue;

			MoveToTarget(Faction, { citizen }, i);
		}
	}
}

bool UArmyManager::CanJoinArmy(ACitizen* Citizen)
{
	FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(Citizen);

	if (Citizen->HealthComponent->GetHealth() == 0 || Camera->DiseaseManager->Injured.Contains(Citizen) || Camera->DiseaseManager->Infected.Contains(Citizen) || Citizen->BioComponent->Age < Camera->PoliticsManager->GetLawValue(faction.Name, "Work Age") || !Citizen->BuildingComponent->WillWork() || IsCitizenInAnArmy(Citizen))
		return false;

	return true;
}

void UArmyManager::CreateArmy(FString FactionName, TArray<ACitizen*> Citizens, bool bGroup, bool bLoad)
{
	if (Citizens.IsEmpty() && !bLoad)
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	FArmyStruct army;
	army.bGroup = bGroup;
	int32 index = faction->Armies.Add(army);

	AddToArmy(index, Citizens);

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

	if (faction->Armies[Index].Citizens.IsEmpty())
		Camera->Grid->AIVisualiser->UpdateInstanceCustomData(Camera->Grid->AIVisualiser->HISMCitizen, faction->Citizens.Find(Citizens[0]), 18, 1.0f);

	faction->Armies[Index].Citizens.Append(Citizens);

	if (faction->Armies[Index].bGroup)
		for (ACitizen* citizen : Citizens)
			citizen->AIController->AIMoveTo(faction->EggTimer);
	else
		MoveToTarget(faction, Citizens, Index);

	Camera->UpdateArmyCountUI(Index, faction->Armies[Index].Citizens.Num());
}

void UArmyManager::RemoveFromArmy(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (int32 i = faction->Armies.Num() - 1; i > -1; i--) {
		if (!faction->Armies[i].Citizens.Contains(Citizen))
			continue;

		faction->Armies[i].Citizens.Remove(Citizen);

		if (faction->Armies[i].Citizens.IsEmpty()) {
			DestroyArmy(faction->Name, i);
		}
		else {
			Camera->Grid->AIVisualiser->UpdateInstanceCustomData(Camera->Grid->AIVisualiser->HISMCitizen, faction->Citizens.Find(Citizen), 18, 0.0f);
			Camera->Grid->AIVisualiser->UpdateInstanceCustomData(Camera->Grid->AIVisualiser->HISMCitizen, faction->Citizens.Find(faction->Armies[i].Citizens[0]), 18, 1.0f);
			Camera->UpdateArmyCountUI(i, faction->Armies[i].Citizens.Num());
		}

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

	if (faction->Armies[Index].Citizens.IsEmpty())
		DestroyArmy(FactionName, Index);
	else
		Camera->UpdateArmyCountUI(Index, faction->Armies[Index].Citizens.Num());
}

bool UArmyManager::IsCitizenInAnArmy(ACitizen* Citizen)
{
	FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(Citizen);

	for (FArmyStruct army : faction.Armies)
		if (army.Citizens.Contains(Citizen))
			return true;

	return false;
}

TTuple<FFactionStruct*, int32> UArmyManager::GetArmyIndex(ACitizen* Citizen)
{
	TTuple<FFactionStruct*, int32> data;
	data.Key = Camera->ConquestManager->GetFaction("", Citizen);
	data.Value = INDEX_NONE;

	for (int32 i = 0; i < data.Key->Armies.Num(); i++) {
		if (!data.Key->Armies[i].Citizens.Contains(Citizen))
			continue;

		data.Value = i;

		break;
	}

	return data;
}

void UArmyManager::DestroyArmy(FString FactionName, int32 Index)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	Camera->Grid->AIVisualiser->UpdateInstanceCustomData(Camera->Grid->AIVisualiser->HISMCitizen, faction->Citizens.Find(faction->Armies[Index].Citizens[0]), 18, 0.0f);
	faction->Armies.RemoveAt(Index);

	if (PlayerSelectedArmyData.Key == faction && PlayerSelectedArmyData.Value == Index)
		SetSelectedArmy(TTuple<FFactionStruct*, int32>(nullptr, INDEX_NONE));

	Camera->UpdateArmyCountUI(Index, 0);
}

TArray<AActor*> UArmyManager::GetReachableTargets(FFactionStruct* Faction, ACitizen* Citizen)
{
	TArray<AActor*> targets;

	for (FString name : Faction->AtWar) {
		FFactionStruct* f = Camera->ConquestManager->GetFaction(name);

		ABuilding* building = MoveArmyMember(f, Citizen, true);

		if (IsValid(building))
			targets.Add(building);
	}

	for (AAISpawner* spawner : GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->SnakeSpawners)
		if (Citizen->AIController->CanMoveTo(spawner->GetActorLocation()))
			targets.Add(spawner);

	return targets;
}

void UArmyManager::MoveToTarget(FFactionStruct* Faction, TArray<ACitizen*> Citizens, int32 Index)
{
	AActor* target = nullptr;

	if (Index != INDEX_NONE && IsValid(Faction->Armies[Index].Target)) {
		UHealthComponent* healthComp = Faction->Armies[Index].Target->GetComponentByClass<UHealthComponent>();
		if (healthComp->GetHealth() > 0)
			target = Faction->Armies[Index].Target;
	}

	if (!IsValid(target)) {
		TArray<AActor*> targets = GetReachableTargets(Faction, Citizens[0]);

		if (targets.IsEmpty()) {
			for (ACitizen* citizen : Citizens)
				RemoveFromArmy(citizen);

			return;
		}

		int32 index = Camera->Stream.RandRange(0, targets.Num() - 1);
		target = targets[index];
	}

	if (Index != INDEX_NONE && Faction->Armies[Index].Target != target)
		Faction->Armies[Index].Target = target;
	
	for (ACitizen* citizen : Citizens)
		citizen->AIController->AIMoveTo(target);
}

void UArmyManager::StartRaid(FFactionStruct Faction, int32 Index)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	MoveToTarget(faction, faction->Armies[Index].Citizens, Index);
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

void UArmyManager::SetSelectedArmy(TTuple<FFactionStruct*, int32> SelectedArmyData)
{
	FArmyStruct* army = nullptr;

	if (PlayerSelectedArmyData.Value != INDEX_NONE) {
		army = &PlayerSelectedArmyData.Key->Armies[PlayerSelectedArmyData.Value];

		for (ACitizen* citizen : army->Citizens) {
			auto aihism = Camera->Grid->AIVisualiser->GetAIHISM(citizen);

			Camera->Grid->AIVisualiser->UpdateInstanceCustomData(aihism.Key, aihism.Value, 1, 0.0f);
		}
	}

	PlayerSelectedArmyData = SelectedArmyData;

	if (PlayerSelectedArmyData.Value == INDEX_NONE)
		return;

	army = &PlayerSelectedArmyData.Key->Armies[PlayerSelectedArmyData.Value];

	for (ACitizen* citizen : army->Citizens) {
		auto aihism = Camera->Grid->AIVisualiser->GetAIHISM(citizen);

		Camera->Grid->AIVisualiser->UpdateInstanceCustomData(aihism.Key, aihism.Value, 1, 2.0f);
	}
}

void UArmyManager::PlayerMoveArmy(FVector Location)
{
	if (PlayerSelectedArmyData.Key->Name != Camera->ColonyName)
		return;

	ACitizen* captain = PlayerSelectedArmyData.Key->Armies[PlayerSelectedArmyData.Value].Citizens[0];

	if (!captain->AIController->CanMoveTo(Location)) {
		Camera->ShowWarning("Army can not reach location");

		return;
	}

	NumTiles = FMath::CeilToInt32(PlayerSelectedArmyData.Key->Armies[PlayerSelectedArmyData.Value].Citizens.Num() / 9.0f);

	ArmyTileLocations.Empty();
	GetArmyTiles(Camera->Grid->GetTileFromLocation(Location), 0, Location);

	Camera->ConquestManager->AIBuildComponent->SortTileDistances(ArmyTileLocations);

	TArray<FVector> locs;
	for (ACitizen* citizen : PlayerSelectedArmyData.Key->Armies[PlayerSelectedArmyData.Value].Citizens) {
		if (locs.IsEmpty())
			ArmyTileLocations.GenerateKeyArray(locs);

		citizen->AIController->AIMoveTo(Camera->Grid, locs[0]);

		locs.RemoveAt(0);
	}

	Camera->SetInteractDecalValue(0.0f, false);
}

void UArmyManager::GetArmyTiles(FTileStruct* Tile, int32 Count, FVector Origin)
{
	FVector tileLocation = Camera->Grid->GetTransform(Tile).GetLocation();

	if (ArmyTileLocations.Contains(tileLocation))
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(Origin, targetLoc, FVector(20.0f, 20.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(tileLocation, ownerLoc, FVector(20.0f, 20.0f, 20.0f));

	double length;
	ENavigationQueryResult::Type result = nav->GetPathLength(GetWorld(), targetLoc.Location, ownerLoc.Location, length);

	if (result != ENavigationQueryResult::Success)
		return;

	for (int32 x = -33; x <= 33; x+=33)
		for (int32 y = -33; y <= 33; y += 33)
			ArmyTileLocations.Add(tileLocation + FVector(x, y, 0.0f), length);

	Count++;

	if (Count >= NumTiles)
		return;

	FHitResult hit;
	for (auto& element : Tile->AdjacentTiles) {
		if (!Tile->bRamp && Tile->Level != element.Value->Level)
			continue;

		if (Tile->bRiver) {
			tileLocation = Camera->Grid->GetTransform(Tile).GetLocation();
			GetWorld()->LineTraceSingleByChannel(hit, tileLocation + FVector(0.0f, 0.0f, 100.0f), tileLocation - FVector(0.0f, 0.0f, 50.0f), ECC_Vehicle);
					
			if (!IsValid(hit.GetActor()) || !hit.GetActor()->IsA<ABuilding>())
				continue;
		}

		GetArmyTiles(element.Value, Count, Origin);
	}
}