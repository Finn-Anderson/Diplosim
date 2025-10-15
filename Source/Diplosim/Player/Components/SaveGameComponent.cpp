#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/BufferArchive.h"
#include "EngineUtils.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/DiplosimUserSettings.h"
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
		actorData.Class = actor->GetClass();
		actorData.Transform = actor->GetActorTransform();

		if (actor->IsA<AGrid>()) {
			actorData.WorldSaveData.Size = Camera->Grid->Size;
			actorData.WorldSaveData.Chunks = Camera->Grid->Chunks;

			actorData.WorldSaveData.Stream = Camera->Grid->Stream;

			actorData.WorldSaveData.LavaSpawnLocations = Camera->Grid->LavaSpawnLocations;

			actorData.WorldSaveData.Tiles.Empty();

			for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
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

	for (FActorSaveData actorData : CurrentSaveGame->Saves[Index].SavedActors) {
		TArray<AActor*> foundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), actorData.Class, foundActors);

		AActor* actor = nullptr;

		if (!foundActors.IsEmpty()) {
			actor = foundActors[0];

			actor->SetActorTransform(actorData.Transform);
		}
		else {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = GetWorld()->SpawnActor<AActor>(actorData.Class, actorData.Transform, params);
		}

		if (actor->IsA<AGrid>()) {
			Camera->Grid->Size = actorData.WorldSaveData.Size;
			Camera->Grid->Chunks = actorData.WorldSaveData.Chunks;

			Camera->Grid->Stream = actorData.WorldSaveData.Stream;

			Camera->Grid->LavaSpawnLocations = actorData.WorldSaveData.LavaSpawnLocations;

			Camera->Grid->Clear();

			Camera->Grid->InitialiseStorage();
			auto bound = Camera->Grid->GetMapBounds();

			for (FTileData t : actorData.WorldSaveData.Tiles) {
				FTileStruct* tile = &Camera->Grid->Storage[t.X + (bound / 2)][t.Y + (bound / 2)];
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

			Camera->Grid->SpawnTiles(true);
		}
	}

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

	if (timer == nullptr)
		Camera->CitizenManager->CreateTimer("AutosaveTimer", Camera, time * 60.0f, FTimerDelegate::CreateUObject(this, &USaveGameComponent::SaveGameSave, FString(""), true), false, true);
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