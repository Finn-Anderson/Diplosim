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

USaveGameComponent::USaveGameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentID = "";
	CurrentSaveGame = nullptr;

	Camera = nullptr;
}

void USaveGameComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentID = FGuid::NewGuid().ToString();
}

void USaveGameComponent::StartNewSave()
{
	CurrentSaveGame = Cast<UDiplosimSaveGame>(UGameplayStatics::CreateSaveGameObject(UDiplosimSaveGame::StaticClass()));

	SaveGameSave("", true);
}

void USaveGameComponent::SaveGameSave(FString Name, bool bAutosave)
{
	TArray<FActorSaveData> allNewActorData;

	for (FActorIterator it(GetWorld()); it; ++it)
	{
		AActor* actor = *it;

		if (actor->IsPendingKillPending())
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

			TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea, grid->HISMWall };

			for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
				FHISMData data;
				data.Name = hism->GetName();

				for (int32 i = 0; i < hism->GetInstanceCount(); i++) {
					FTransform transform;
					hism->GetInstanceTransform(i, transform);

					data.Transforms.Add(transform);
				}
				
				data.CustomDataValues = hism->PerInstanceSMCustomData;

				actorData.WorldSaveData.HISMData.Add(data);
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

		allNewActorData.Add(actorData);
	}

	FSave* save;

	if (Name == "") {
		save = CreateNewSaveStruct(Name, bAutosave);

		if (bAutosave)
			CapAutosaves();
	}
	else {
		FSave s;
		s.SaveName = Name;

		int32 index = CurrentSaveGame->Saves.Find(s);

		if (index == INDEX_NONE) {
			save = CreateNewSaveStruct(Name, bAutosave);
		}
		else {
			save = &CurrentSaveGame->Saves[index];

			save->bAutosave = bAutosave;
		}
	}

	for (FTimerStruct timer : Camera->CitizenManager->Timers)
		save->SavedTimers.Add(timer);

	save->SavedActors.Empty();
	save->SavedActors.Append(allNewActorData);

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

	for (FTimerStruct timer : CurrentSaveGame->Saves[Index].SavedTimers)
		Camera->CitizenManager->Timers.AddTail(timer);

	for (FActorSaveData actorData : CurrentSaveGame->Saves[Index].SavedActors) {
		TArray<AActor*> foundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), actorData.Class, foundActors);

		AActor* actor = nullptr;

		for (AActor* a : foundActors) {
			if (a->IsA<AEggBasket>() || a->IsA<AAI>() || a->IsA<ABuilding>()) {
				a->Destroy();

				continue;
			}

			if (a->GetName() != actorData.Name)
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
	}

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

FSave* USaveGameComponent::CreateNewSaveStruct(FString Name, bool bAutosave)
{
	FCalendarStruct calendar = Camera->Grid->AtmosphereComponent->Calendar;

	FSave save;
	save.SaveName = Name;
	save.Period = calendar.Period;
	save.Day = calendar.Days[calendar.Index];
	save.Hour = calendar.Hour;

	if (Name == "")
		save.SaveName = FDateTime::Now().ToString();

	save.bAutosave = bAutosave;

	int32 index = CurrentSaveGame->Saves.Add(save);

	return &CurrentSaveGame->Saves[index];
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

		Camera->CitizenManager->CreateTimer("AutosaveTimer", Camera, time * 60.0f, this, "SaveGameSave", params, false, true);
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