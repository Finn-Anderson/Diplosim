#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"
#include "NavigationSystem.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"

#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
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
	CurrentSaveData = Cast<UDiplosimSaveGameData>(UGameplayStatics::CreateSaveGameObject(UDiplosimSaveGameData::StaticClass()));
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
		FCalendarStruct calendar = Camera->Grid->AtmosphereComponent->Calendar;

		CurrentSaveData->SavedData[CurrentIndex].Period = calendar.Period;
		CurrentSaveData->SavedData[CurrentIndex].Day = calendar.Days[calendar.Index];
		CurrentSaveData->SavedData[CurrentIndex].Hour = calendar.Hour;

		CurrentSaveData->SavedData[CurrentIndex].bAutosave = bAutosave;
	}

	CurrentSaveData->LastTimeUpdated = FDateTime::Now();
	CurrentSaveData->SavedData[CurrentIndex].CitizenNum = CurrentSaveGame->SaveGame(Camera, CurrentIndex, CurrentID);

	UGameplayStatics::SaveGameToSlot(CurrentSaveData, FString(CurrentID + "Data"), 0);

	if (bAutosave)
		StartAutosaveTimer();
}

void USaveGameComponent::LoadGameSave(UDiplosimSaveGameData* SaveGameData, FString SlotName, int32 Index)
{
	Camera->PController->DisableInput(Camera->PController);
	Camera->SetPause(false);

	CurrentSaveData = SaveGameData;
	CurrentID = SlotName;
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
	Camera->Cancel();

	for (FTimerStruct timer : Camera->TimerManager->Timers)
		Camera->TimerManager->RemoveTimer(timer.ID, timer.Actor);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	FScriptDelegate delegate;
	delegate.BindUFunction(this, TEXT("OnNavMeshGenerated"));
	nav->OnNavigationGenerationFinishedDelegate.Add(delegate);

	CurrentSaveGame = DecompressSave(CurrentID);
	CurrentSaveGame->LoadGame(Camera, CurrentIndex);
}

void USaveGameComponent::OnNavMeshGenerated()
{
	StartAutosaveTimer();

	Checklist.bLoad = false;
	Camera->Grid->AIVisualiser->MainLoop(Camera);

	Camera->SetCurrentResearchUI();

	Camera->DisplayBuildUI();
	Camera->PController->EnableInput(Camera->PController);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	FScriptDelegate delegate;
	delegate.BindUFunction(this, TEXT("OnNavMeshGenerated"));
	nav->OnNavigationGenerationFinishedDelegate.Remove(delegate); 
	
	Camera->SetPause(true, true);
}

void USaveGameComponent::CompressAndSave(FString SlotName, UDiplosimSaveGame* SaveGame)
{
	TArray<uint8> compressedBytes, bytes;
	FArchiveSaveCompressedProxy compressor = FArchiveSaveCompressedProxy(compressedBytes, NAME_Zlib, ECompressionFlags::COMPRESS_BiasMemory);
	UGameplayStatics::SaveGameToMemory(SaveGame, bytes);
	compressor << bytes;
	compressor.Flush();

	const FString& saveDir = UKismetSystemLibrary::GetProjectSavedDirectory() / "SaveGames";
	const FString filePath = saveDir + "/" + SlotName + ".sav";
	FFileHelper::SaveArrayToFile(compressedBytes, *filePath);
}

UDiplosimSaveGame* USaveGameComponent::DecompressSave(FString SlotName)
{
	const FString& saveDir = UKismetSystemLibrary::GetProjectSavedDirectory() / "SaveGames";

	TArray<uint8> compressedBytes, bytes;
	FFileHelper::LoadFileToArray(compressedBytes, *(saveDir + "/" + SlotName + ".sav"));

	FArchiveLoadCompressedProxy decompressor = FArchiveLoadCompressedProxy(compressedBytes, NAME_Zlib);
	decompressor << bytes;

	return Cast<UDiplosimSaveGame>(UGameplayStatics::LoadGameFromMemory(bytes));;
}

void USaveGameComponent::DeleteGameSave(FString SlotName, UDiplosimSaveGameData* SaveData, int32 Index, bool bSlot)
{
	UDiplosimSaveGame* gameSave = nullptr;
	if (CurrentSaveData && CurrentSaveData->LastTimeUpdated == SaveData->LastTimeUpdated)
		gameSave = CurrentSaveGame;

	if (bSlot) {
		if (IsValid(gameSave)) {
			gameSave->Saves.Empty();
			SaveData->SavedData.Empty();
		}

		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
		UGameplayStatics::DeleteGameInSlot(SlotName + "Data", 0);
	}
	else {
		if (!IsValid(gameSave))
			gameSave = DecompressSave(SlotName);

		gameSave->Saves.RemoveAt(Index);
		SaveData->SavedData.RemoveAt(Index);

		CompressAndSave(SlotName, gameSave);
		UGameplayStatics::SaveGameToSlot(SaveData, SlotName + "Data", 0);
	}

	if (CurrentSaveData && CurrentSaveData->LastTimeUpdated == SaveData->LastTimeUpdated) {
		CurrentSaveData = SaveData;
		CurrentSaveGame = gameSave;
	}
}

TMap<FString, UDiplosimSaveGameData*> USaveGameComponent::LoadAllSavedGames()
{
	TMap<FString, UDiplosimSaveGameData*> gameSavesList;
	const FString& saveDir = UKismetSystemLibrary::GetProjectSavedDirectory() / "SaveGames";

	TArray<FString> foundFiles;
	const FString& ext = ".sav";

	IFileManager::Get().FindFiles(foundFiles, *saveDir, *ext);

	TArray<UDiplosimSaveGameData*> sortedGames;
	TArray<FString> sortedNames;
	for (const FString& name : foundFiles) {
		if (!name.Contains("Data.sav"))
			continue;

		UDiplosimSaveGameData* saveData = Cast<UDiplosimSaveGameData>(UGameplayStatics::LoadGameFromSlot(name.LeftChop(4), 0));

		int32 index = 0;
		for (int32 i = 0; i < sortedGames.Num(); i++) {
			if (saveData->LastTimeUpdated > sortedGames[i]->LastTimeUpdated)
				break;

			index++;
		}

		sortedGames.Insert(saveData, index);
		sortedNames.Insert(name.LeftChop(8), index);
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

	FSaveData saveData;
	saveData.SaveName = Name;
	saveData.Period = calendar.Period;
	saveData.Day = calendar.Days[calendar.Index];
	saveData.Hour = calendar.Hour;

	if (Name == "")
		saveData.SaveName = FDateTime::Now().ToString();

	saveData.bAutosave = bAutosave;

	CurrentIndex = CurrentSaveData->SavedData.Add(saveData);
	CurrentSaveGame->Saves.Add(save);
}

void USaveGameComponent::CapAutosaves()
{
	int32 firstAutosaveIndex = INDEX_NONE;
	int32 count = 0;

	for (int32 i = 0; i < CurrentSaveData->SavedData.Num(); i++) {
		if (!CurrentSaveData->SavedData[i].bAutosave)
			continue;

		if (firstAutosaveIndex == INDEX_NONE)
			firstAutosaveIndex = i;

		count++;
	}

	if (count <= 3)
		return;

	CurrentSaveGame->Saves.RemoveAt(firstAutosaveIndex);
	CurrentSaveData->SavedData.RemoveAt(firstAutosaveIndex);
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

void USaveGameComponent::SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FActorSaveData* CitizenData, FAIData& AIData, bool bVisitor)
{
	ACitizen* citizen = Cast<ACitizen>(CitizenData->Actor);

	if (Building->IsA<AHouse>())
		citizen->BuildingComponent->House = Cast<AHouse>(Building);
	else if (Building->IsA<AOrphanage>() && bVisitor)
		citizen->BuildingComponent->Orphanage = Cast<AOrphanage>(Building);
	else if (Building->IsA<ASchool>() && bVisitor)
		citizen->BuildingComponent->School = Cast<ASchool>(Building);
	else
		citizen->BuildingComponent->Employment = Cast<AWork>(Building);

	if (AIData.BuildingAtName == BuildingName) {
		Building->Enter(citizen);

		citizen->BuildingComponent->EnterLocation = AIData.CitizenData.EnterLocation;
	}
}