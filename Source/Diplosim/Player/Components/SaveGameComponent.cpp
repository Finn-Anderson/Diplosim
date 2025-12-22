#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"

#include "AI/Citizen.h"
#include "AI/BuildingComponent.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Universal/DiplosimUserSettings.h"

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
	FSave s;
	s.SaveName = Name;

	CurrentIndex = CurrentSaveGame->Saves.Find(s);

	if (CurrentIndex == INDEX_NONE) {
		CreateNewSaveStruct(Name, bAutosave);

		if (bAutosave)
			CapAutosaves();
	}
	else {
		CurrentSaveGame->Saves[CurrentIndex].bAutosave = bAutosave;
	}

	CurrentSaveGame->SaveGame(Camera, CurrentIndex, CurrentID);

	if (bAutosave)
		StartAutosaveTimer();
}

void USaveGameComponent::LoadGameSave(FString SlotName, class UDiplosimSaveGame* SaveGame, int32 Index)
{
	Camera->bInMenu = false;
	Camera->SetPause(false);

	CurrentID = SlotName;
	CurrentSaveGame = SaveGame;
	CurrentIndex = Index;

	Camera->Grid->MapUIInstance->RemoveFromParent();
	Camera->BuildUIInstance->RemoveFromParent();

	Checklist.Reset();

	Camera->Grid->LoadUIInstance->AddToViewport();
	Camera->UpdateLoadingText("Loading...");
}

bool USaveGameComponent::IsLoading()
{
	return Checklist.bLoad;
}

void USaveGameComponent::LoadGameCallback(EAsyncLoop Loop)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Loop]() {
		if (*Checklist.CheckLoop.Find(Loop))
			return;

		Checklist.CheckLoop.Add(Loop, true);

		if (Checklist.bIsAllChecked())
			LoadSave();
	});
}

void USaveGameComponent::LoadSave()
{
	Camera->SetPause(true, false);

	Camera->Cancel();

	for (FTimerStruct timer : Camera->TimerManager->Timers)
		Camera->TimerManager->RemoveTimer(timer.ID, timer.Actor);
	
	CurrentSaveGame->LoadGame(Camera, CurrentIndex);

	StartAutosaveTimer();

	Checklist.bLoad = false;
	Camera->Grid->AIVisualiser->MainLoop(Camera);

	Camera->SetMouseCapture(false, false, true);

	Camera->SetCurrentResearchUI();

	Camera->Grid->LoadUIInstance->RemoveFromParent();
	Camera->PauseUIInstance->AddToViewport();
	Camera->BuildUIInstance->AddToViewport();
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

void USaveGameComponent::CreateNewSaveStruct(FString Name, bool bAutosave)
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

	CurrentIndex = CurrentSaveGame->Saves.Add(save);
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

	FTimerStruct* timer = Camera->TimerManager->FindTimer("AutosaveTimer", Camera);

	if (timer == nullptr) {
		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter("", params);
		Camera->TimerManager->SetParameter(true, params);

		Camera->TimerManager->CreateTimer("AutosaveTimer", Camera, time * 60.0f, "SaveGameSave", params, false, true);
	}
	else
		timer->Timer = 0.0f;
}

void USaveGameComponent::UpdateAutosave(int32 NewTime)
{
	if (NewTime == 0) {
		Camera->TimerManager->RemoveTimer("AutosaveTimer", Camera);
	}
	else {
		FTimerStruct* timer = Camera->TimerManager->FindTimer("AutosaveTimer", Camera);

		if (timer == nullptr)
			StartAutosaveTimer();
		else if (NewTime - timer->Timer > 0.0f)
			timer->Target = NewTime;
	}
}

AActor* USaveGameComponent::GetSaveActorFromName(TArray<FActorSaveData> SavedData, FString Name)
{
	if (Name == "")
		return nullptr;

	FActorSaveData data;
	data.Name = Name;

	int32 i = SavedData.Find(data);

	if (i == INDEX_NONE)
		return nullptr;

	return SavedData[i].Actor;
}

void USaveGameComponent::SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FActorSaveData CitizenData, bool bVisitor)
{
	ACitizen* citizen = Cast<ACitizen>(CitizenData.Actor);

	if (Building->IsA<AHouse>())
		citizen->BuildingComponent->House = Cast<AHouse>(Building);
	else if (Building->IsA<AOrphanage>() && bVisitor)
		citizen->BuildingComponent->Orphanage = Cast<AOrphanage>(Building);
	else if (Building->IsA<ASchool>() && bVisitor)
		citizen->BuildingComponent->School = Cast<ASchool>(Building);
	else
		citizen->BuildingComponent->Employment = Cast<AWork>(Building);

	if (CitizenData.AIData.BuildingAtName == BuildingName) {
		Building->Enter(citizen);

		citizen->BuildingComponent->EnterLocation = CitizenData.AIData.CitizenData.EnterLocation;
	}
}