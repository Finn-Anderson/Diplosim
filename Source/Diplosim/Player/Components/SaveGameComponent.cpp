#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/EggBasket.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Map/Atmosphere/Clouds.h"
#include "AI/Citizen.h"
#include "Buildings/Work/Service/Builder.h"

USaveGameComponent::USaveGameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentID = "";
	CurrentSaveGame = nullptr;

	Camera = nullptr;
}

void USaveGameComponent::StartNewSave()
{
	CurrentSaveGame = Cast<UDiplosimSaveGame>(UGameplayStatics::CreateSaveGameObject(UDiplosimSaveGame::StaticClass()));
	CurrentID = FGuid::NewGuid().ToString();

	SaveGameSave("", true);
}

void USaveGameComponent::SaveGameSave(FString Name, bool bAutosave)
{
	TArray<FActorSaveData> allNewActorData;

	TArray<AActor*> foundActors;

	TArray<AActor*> actors = { Camera, Camera->Grid };
	TArray<AActor*> potentialWetActors;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AResource::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	for (AActor* actor : actors)
	{
		if (!IsValid(actor) || actor->IsPendingKillPending())
			continue;

		FActorSaveData actorData;
		actorData.Name = actor->GetName();
		actorData.Class = actor->GetClass();
		actorData.Transform = actor->GetActorTransform();

		if (actor->IsA<AGrid>()) {
			AGrid* grid = Cast<AGrid>(actor);

			actorData.WorldSaveData.Size = grid->Size;
			actorData.WorldSaveData.Chunks = grid->Chunks;

			actorData.WorldSaveData.Stream = grid->Stream;

			actorData.WorldSaveData.LavaSpawnLocations = grid->LavaSpawnLocations;

			actorData.WorldSaveData.Tiles.Empty();

			for (TArray<FTileStruct>& row : grid->Storage) {
				for (FTileStruct& tile : row) {
					FTileData t;
					t.Level = tile.Level;
					t.Fertility = tile.Fertility;
					t.X = tile.X;
					t.Y = tile.Y;
					t.Rotation = tile.Rotation;
					t.bRamp = tile.bRamp;
					t.bRiver = tile.bRiver;
					t.bEdge = tile.bEdge;
					t.bMineral = tile.bMineral;
					t.bUnique = tile.bUnique;

					actorData.WorldSaveData.Tiles.Add(t);
				}
			}

			actorData.WorldSaveData.HISMData.Empty();
			actorData.WorldSaveData.CloudsData.WetnessData.Empty();
			actorData.WorldSaveData.CloudsData.CloudData.Empty();

			TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea, grid->HISMWall };

			UCloudComponent* clouds = grid->AtmosphereComponent->Clouds;

			for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
				FHISMData data;
				data.Name = hism->GetName();

				for (int32 i = 0; i < hism->GetInstanceCount(); i++) {
					FTransform transform;
					hism->GetInstanceTransform(i, transform);

					data.Transforms.Add(transform);

					if (hism == grid->HISMLava || hism == grid->HISMRiver || hism == grid->HISMWall)
						continue;

					FWetnessStruct wetnessStruct;
					wetnessStruct.HISM = hism;
					wetnessStruct.Instance = i;

					int32 index = clouds->WetnessStruct.Find(wetnessStruct);

					if (index == INDEX_NONE)
						continue;

					FWetnessData wetnessData;
					wetnessData.Location = transform.GetLocation();
					wetnessData.Value = clouds->WetnessStruct[index].Value;
					wetnessData.Increment = clouds->WetnessStruct[index].Increment;

					actorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
				}
				
				data.CustomDataValues = hism->PerInstanceSMCustomData;

				actorData.WorldSaveData.HISMData.Add(data);
			}

			actorData.WorldSaveData.AtmosphereData.Calendar = grid->AtmosphereComponent->Calendar;
			actorData.WorldSaveData.AtmosphereData.bRedSun = grid->AtmosphereComponent->bRedSun;
			actorData.WorldSaveData.AtmosphereData.WindRotation = grid->AtmosphereComponent->WindRotation;

			actorData.WorldSaveData.NaturalDisasterData.bDisasterChance = grid->AtmosphereComponent->NaturalDisasterComponent->bDisasterChance;
			actorData.WorldSaveData.NaturalDisasterData.Frequency = grid->AtmosphereComponent->NaturalDisasterComponent->Frequency;
			actorData.WorldSaveData.NaturalDisasterData.Intensity = grid->AtmosphereComponent->NaturalDisasterComponent->Intensity;

			for (AActor* a : potentialWetActors) {
				FWetnessStruct wetnessStruct;
				wetnessStruct.Actor = a;

				int32 index = clouds->WetnessStruct.Find(wetnessStruct);

				if (index == INDEX_NONE)
					continue;

				FWetnessData wetnessData;
				wetnessData.Location = a->GetActorLocation();
				wetnessData.Value = clouds->WetnessStruct[index].Value;
				wetnessData.Increment = clouds->WetnessStruct[index].Increment;

				actorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
			}

			for (FCloudStruct cloud : Camera->Grid->AtmosphereComponent->Clouds->Clouds) {
				FCloudData data;
				data.Transform = cloud.HISMCloud->GetRelativeTransform();
				data.Distance = cloud.Distance;
				data.bPrecipitation = cloud.Precipitation != nullptr;
				data.bHide = cloud.bHide;
				data.lightningFrequency = cloud.lightningFrequency;
				data.lightningTimer = cloud.lightningTimer;

				actorData.WorldSaveData.CloudsData.CloudData.Add(data);
			}
		}
		else if (actor->IsA<AResource>()) {
			AResource* resource = Cast<AResource>(actor);

			actorData.ResourceData.HISMData.Name = resource->ResourceHISM->GetName(); 
			
			actorData.ResourceData.HISMData.Transforms.Empty();
			for (int32 i = 0; i < resource->ResourceHISM->GetInstanceCount(); i++) {
				FTransform transform;
				resource->ResourceHISM->GetInstanceTransform(i, transform);

				actorData.ResourceData.HISMData.Transforms.Add(transform);
			}

			actorData.ResourceData.HISMData.CustomDataValues = resource->ResourceHISM->PerInstanceSMCustomData;

			actorData.ResourceData.Workers = resource->WorkerStruct;
		}
		else if (actor->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(actor);

			actorData.BuildingData.FactionName = building->FactionName;
			actorData.BuildingData.Capacity = building->Capacity;

			actorData.BuildingData.Seed = building->SeedNum;
			actorData.BuildingData.ChosenColour = building->ChosenColour;
			actorData.BuildingData.Tier = building->Tier;
			actorData.BuildingData.ActualMesh = building->ActualMesh;

			actorData.BuildingData.Storage = building->Storage;
			actorData.BuildingData.Basket = building->Basket;

			actorData.BuildingData.DeathTime = building->DeathTime;
			actorData.BuildingData.bOperate = building->bOperate;

			if (IsA<AWork>()) {
				actorData.BuildingData.WorkHours = Cast<AWork>(building)->WorkHours;

				if (building->IsA<ABuilder>())
					 actorData.BuildingData.BuildPercentage = Cast<ABuilder>(building)->BuildPercentage;
			}

			TArray<FOccupantData> occData;

			for (FOccupantStruct occupant : building->Occupied) {
				FOccupantData occupantData;
				occupantData.OccupantName = occupant.Occupant->GetName();

				for (ACitizen* visitor : occupant.Visitors)
					occupantData.VisitorNames.Add(visitor->GetName());

				occData.Add(occupantData);
			}

			actorData.BuildingData.OccupantsData = occData;
		}

		for (FTimerStruct timer : Camera->CitizenManager->Timers)
			if (timer.Actor == actor)
				actorData.SavedTimers.Add(timer);

		allNewActorData.Add(actorData);
	}

	if (Name == "") {
		CreateNewSaveStruct(Name, bAutosave, allNewActorData);

		if (bAutosave)
			CapAutosaves();
	}
	else {
		FSave s;
		s.SaveName = Name;

		int32 index = CurrentSaveGame->Saves.Find(s);

		if (index == INDEX_NONE) {
			CreateNewSaveStruct(Name, bAutosave, allNewActorData);
		}
		else {
			CurrentSaveGame->Saves[index].SavedActors = allNewActorData;
			CurrentSaveGame->Saves[index].bAutosave = bAutosave;
		}
	}

	CurrentSaveGame->LastTimeUpdated = FDateTime::Now();

	UGameplayStatics::AsyncSaveGameToSlot(CurrentSaveGame, CurrentID, 0);

	if (bAutosave)
		StartAutosaveTimer();
}

void USaveGameComponent::LoadGameSave(FString SlotName, class UDiplosimSaveGame* SaveGame, int32 Index)
{
	CurrentID = SlotName;
	CurrentSaveGame = SaveGame;
	CurrentSaveGame->LastTimeUpdated = FDateTime::Now();

	Camera->Cancel();

	for (FTimerStruct timer : Camera->CitizenManager->Timers)
		Camera->CitizenManager->RemoveTimer(timer.ID, timer.Actor);

	Camera->ConquestManager->Factions = CurrentSaveGame->Saves[Index].Factions;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		faction.Buildings.Empty();
		faction.Citizens.Empty();
		faction.Rebels.Empty();
		faction.Clones.Empty();
	}

	TArray<AActor*> actors;
	TArray<AActor*> foundActors;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);

	for (AActor* a : actors)
		a->Destroy();

	TMap<FString, FAIData> citizenToName;
	TArray<FWetnessData> wetnessData;

	for (FActorSaveData actorData : CurrentSaveGame->Saves[Index].SavedActors) {
		AActor* actor = nullptr;

		if (foundActors.Num() == 1) {
			if (foundActors[0]->GetName() != actorData.Name)
				continue;

			actor = foundActors[0];

			actor->SetActorTransform(actorData.Transform);

			break;
		}

		if (!IsValid(actor)) {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = GetWorld()->SpawnActor<AActor>(actorData.Class, actorData.Transform, params);
		}

		if (actor->IsA<AGrid>()) {
			AGrid* grid = Cast<AGrid>(actor);

			grid->Size = actorData.WorldSaveData.Size;
			grid->Chunks = actorData.WorldSaveData.Chunks;

			grid->Stream = actorData.WorldSaveData.Stream;

			grid->LavaSpawnLocations = actorData.WorldSaveData.LavaSpawnLocations;

			grid->Clear();

			grid->InitialiseStorage();
			auto bound = grid->GetMapBounds();

			for (FTileData t : actorData.WorldSaveData.Tiles) {
				FTileStruct* tile = &grid->Storage[t.X + (bound / 2)][t.Y + (bound / 2)];
				tile->Level = t.Level;
				tile->Fertility = t.Fertility;
				tile->X = t.X;
				tile->Y = t.Y;
				tile->Rotation = t.Rotation;
				tile->bRamp = t.bRamp;
				tile->bRiver = t.bRiver;
				tile->bEdge = t.bEdge;
				tile->bMineral = t.bMineral;
				tile->bUnique = t.bUnique;
			}

			grid->SpawnTiles(true);

			TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea, grid->HISMWall };
			
			for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
				FHISMData data;
				data.Name = hism->GetName();

				int32 index = actorData.WorldSaveData.HISMData.Find(data);
				hism->PerInstanceSMCustomData = actorData.WorldSaveData.HISMData[index].CustomDataValues;

				hism->BuildTreeIfOutdated(true, true);
			}

			grid->AtmosphereComponent->Calendar = actorData.WorldSaveData.AtmosphereData.Calendar;
			grid->AtmosphereComponent->bRedSun = actorData.WorldSaveData.AtmosphereData.bRedSun;
			grid->AtmosphereComponent->WindRotation = actorData.WorldSaveData.AtmosphereData.WindRotation;

			if (grid->AtmosphereComponent->bRedSun)
				grid->AtmosphereComponent->NaturalDisasterComponent->AlterSunGradually(0.15f, -1.00f);

			Camera->Grid->AtmosphereComponent->Clouds->ProcessRainEffect.Empty();
			Camera->Grid->AtmosphereComponent->Clouds->WetnessStruct.Empty();
			wetnessData = actorData.WorldSaveData.CloudsData.WetnessData;

			Camera->Grid->AtmosphereComponent->Clouds->Clear();
			
			for (FCloudData data : actorData.WorldSaveData.CloudsData.CloudData) {
				FCloudStruct cloudStruct = Camera->Grid->AtmosphereComponent->Clouds->CreateCloud(data.Transform, data.bPrecipitation ? 100 : 0);
				cloudStruct.Distance = data.Distance;
				cloudStruct.bHide = data.bHide;
				cloudStruct.lightningFrequency = data.lightningFrequency;
				cloudStruct.lightningTimer = data.lightningTimer;

				Camera->Grid->AtmosphereComponent->Clouds->Clouds.Add(cloudStruct);
			}

			grid->SetupEnvironment(true);
		}
		else if (actor->IsA<AResource>()) {
			AResource* resource = Cast<AResource>(actor);

			resource->ResourceHISM->AddInstances(actorData.ResourceData.HISMData.Transforms, true, true, false);
			resource->ResourceHISM->PerInstanceSMCustomData = actorData.ResourceData.HISMData.CustomDataValues;

			resource->WorkerStruct = actorData.ResourceData.Workers;

			resource->ResourceHISM->BuildTreeIfOutdated(true, true);
		}
		else if (actor->IsA<AEggBasket>()) {
			AEggBasket* eggBasket = Cast<AEggBasket>(actor);

			FTileStruct* tile = Camera->Grid->GetTileFromLocation(actorData.Transform.GetLocation());

			eggBasket->Grid = Camera->Grid;
			eggBasket->Tile = tile;

			Camera->Grid->ResourceTiles.RemoveSingle(tile);
		}
		else if (actor->IsA<ACamera>()) {
			ACamera* camera = Cast<ACamera>(actor);

			Camera->ClearPopupUI();
			Camera->SetInteractStatus(Camera->WidgetComponent->GetAttachmentRootActor(), false);

			camera->Detach();
			camera->MovementComponent->TargetLength = 3000.0f;
		}
		else if (actor->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(actor);

			building->FactionName = actorData.BuildingData.FactionName;
			Camera->ConquestManager->GetFaction(building->FactionName)->Buildings.Add(building);

			building->SetSeed(actorData.BuildingData.Seed);
			building->SetTier(actorData.BuildingData.Tier);

			building->Capacity = actorData.BuildingData.Capacity;

			FLinearColor colour = actorData.BuildingData.ChosenColour;
			building->SetBuildingColour(colour.R, colour.G, colour.B);

			building->ActualMesh = actorData.BuildingData.ActualMesh;
			// if construction manager contains building, set mesh to box.

			building->Storage = actorData.BuildingData.Storage;
			building->Basket = actorData.BuildingData.Basket;

			building->DeathTime = actorData.BuildingData.DeathTime;
			building->bOperate = actorData.BuildingData.bOperate;

			if (building->IsA<AWork>()) {
				Cast<AWork>(building)->WorkHours = actorData.BuildingData.WorkHours;

				if (building->IsA<ABuilder>())
					Cast<ABuilder>(building)->BuildPercentage = actorData.BuildingData.BuildPercentage;
			}

			for (FOccupantData data : actorData.BuildingData.OccupantsData) {
				FAIData citizenData = *citizenToName.Find(data.OccupantName);

				SetupCitizenBuilding(actorData.Name, building, citizenData, false);

				FOccupantStruct occupant;
				occupant.Occupant = citizenData.Citizen;

				for (FString name : data.VisitorNames) {
					FAIData visitorData = *citizenToName.Find(name);

					occupant.Visitors.Add(visitorData.Citizen);

					SetupCitizenBuilding(actorData.Name, building, visitorData, true);
				}

				building->Occupied.Add(occupant);
			}

			TArray<FOccupantData> occData;

			for (FOccupantStruct occupant : building->Occupied) {
				FOccupantData occupantData;
				occupantData.OccupantName = occupant.Occupant->GetName();

				for (ACitizen* visitor : occupant.Visitors)
					occupantData.VisitorNames.Add(visitor->GetName());

				occData.Add(occupantData);
			}

			actorData.BuildingData.OccupantsData = occData;
		}

		for (FTimerStruct timer : actorData.SavedTimers) {
			timer.Actor = actor;

			Camera->CitizenManager->Timers.AddTail(timer);
		}
	}

	for (FWetnessData data : wetnessData)
		Camera->Grid->AtmosphereComponent->Clouds->RainCollisionHandler(data.Location, data.Value, data.Increment);

	Camera->BuildUIInstance->AddToViewport();

	StartAutosaveTimer();
}

void USaveGameComponent::DeleteGameSave(FString SlotName, UDiplosimSaveGame* SaveGame, int32 Index, bool bSlot)
{
	if (bSlot)
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	else
		SaveGame->Saves.RemoveAt(Index);
}

TMap<FString, class UDiplosimSaveGame*> USaveGameComponent::LoadAllSavedGames()
{
	TMap<FString, class UDiplosimSaveGame*> gameSavesList;

	const FString& saveDir = UKismetSystemLibrary::GetProjectSavedDirectory() / "SaveGames";

	TArray<FString> foundFiles;
	const FString& ext = ".sav";

	IFileManager::Get().FindFiles(foundFiles, *saveDir, *ext);

	TArray<FString> fileNames;
	for (const FString& name : foundFiles)
		fileNames.AddUnique(name.LeftChop(4));

	TArray<UDiplosimSaveGame*> sortedGames;
	TArray<FString> sortedNames;
	for (const FString& name : fileNames) {
		UDiplosimSaveGame* gameSave = Cast<UDiplosimSaveGame>(UGameplayStatics::LoadGameFromSlot(name, 0));

		int32 index = 0;

		for (int32 i = 0; i < sortedGames.Num(); i++) {
			if (gameSave->LastTimeUpdated > sortedGames[i]->LastTimeUpdated)
				break;

			index++;
		}

		sortedGames.Insert(gameSave, index);
		sortedNames.Insert(name, index);
	}

	for (int32 i = 0; i < sortedGames.Num(); i++)
		gameSavesList.Add(sortedNames[i], sortedGames[i]);

	return gameSavesList;
}

void USaveGameComponent::CreateNewSaveStruct(FString Name, bool bAutosave, TArray<FActorSaveData> NewActorData)
{
	FCalendarStruct calendar = Camera->Grid->AtmosphereComponent->Calendar;

	FSave save;
	save.SaveName = Name;
	save.Period = calendar.Period;
	save.Day = calendar.Days[calendar.Index];
	save.Hour = calendar.Hour;

	if (Name == "")
		save.SaveName = FDateTime::Now().ToString();

	save.Factions = Camera->ConquestManager->Factions;
	save.SavedActors = NewActorData;

	save.bAutosave = bAutosave;

	CurrentSaveGame->Saves.Add(save);
}

void USaveGameComponent::CapAutosaves()
{
	int32 firstAutosaveIndex = INDEX_NONE;
	int32 count = 0;

	for (int32 i = 0; i < CurrentSaveGame->Saves.Num(); i++) {
		if (!CurrentSaveGame->Saves[i].bAutosave)
			continue;

		if (firstAutosaveIndex == INDEX_NONE)
			firstAutosaveIndex = i;

		count++;
	}

	if (count <= 3)
		return;

	CurrentSaveGame->Saves.RemoveAt(firstAutosaveIndex);
}

void USaveGameComponent::StartAutosaveTimer()
{
	int32 time = Camera->Settings->GetAutosaveTimer();

	if (time == 0)
		return;

	FTimerStruct* timer = Camera->CitizenManager->FindTimer("AutosaveTimer", Camera);

	if (timer == nullptr) {
		TArray<FTimerParameterStruct> params;
		Camera->CitizenManager->SetParameter("", params);
		Camera->CitizenManager->SetParameter(true, params);

		Camera->CitizenManager->CreateTimer("AutosaveTimer", Camera, time * 60.0f, "SaveGameSave", params, false, true);
	}
	else
		timer->Timer = 0.0f;
}

void USaveGameComponent::UpdateAutosave(int32 NewTime)
{
	if (NewTime == 0) {
		Camera->CitizenManager->RemoveTimer("AutosaveTimer", Camera);
	}
	else {
		FTimerStruct* timer = Camera->CitizenManager->FindTimer("AutosaveTimer", Camera);

		if (timer == nullptr)
			StartAutosaveTimer();
		else if (NewTime - timer->Timer > 0.0f)
			timer->Target = NewTime;
	}
}

void USaveGameComponent::SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FAIData CitizenData, bool bVisitor)
{
	if (Building->IsA<AHouse>())
		CitizenData.Citizen->Building.House = Cast<AHouse>(Building);
	else if (Building->IsA<AOrphanage>() && bVisitor)
		CitizenData.Citizen->Building.Orphanage = Cast<AOrphanage>(Building);
	else if (Building->IsA<ASchool>() && bVisitor)
		CitizenData.Citizen->Building.School = Cast<ASchool>(Building);
	else
		CitizenData.Citizen->Building.Employment = Cast<AWork>(Building);

	if (CitizenData.BuildingAtName == BuildingName)
		Building->Enter(CitizenData.Citizen);
}