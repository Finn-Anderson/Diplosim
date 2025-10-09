#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "EngineUtils.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/DiplosimUserSettings.h"
#include "Map/Grid.h"

USaveGameComponent::USaveGameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentID = "";
	AutosaveNum = 1;
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
	CurrentSaveGame = Cast<UDiplosimSaveGame>(UGameplayStatics::CreateSaveGameObject(USaveGame::StaticClass()));

	StartAutosaveTimer();
}

void USaveGameComponent::SaveGameSave(bool bAutosave)
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

		FMemoryWriter MemWriter(actorData.ByteData);

		FObjectAndNameAsStringProxyArchive ar(MemWriter, true);
		ar.ArIsSaveGame = false;

		actor->Serialize(ar);

		allNewActorData.Add(actorData);
	}

	CurrentSaveGame->SavedActors.Empty();
	CurrentSaveGame->SavedActors.Append(allNewActorData);

	UGameplayStatics::AsyncSaveGameToSlot(CurrentSaveGame, GetSlotName(bAutosave), 0);
}

void USaveGameComponent::LoadGameSave(FString SlotName)
{
	CurrentSaveGame = Cast<UDiplosimSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));

	for (FActorSaveData actorData : CurrentSaveGame->SavedActors) {
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

		FMemoryReader MemReader(actorData.ByteData);

		FObjectAndNameAsStringProxyArchive ar(MemReader, true);
		ar.ArIsSaveGame = false;

		actor->Serialize(ar);

		break;
	}

	Camera->Grid->RebuildAll();

	StartAutosaveTimer();
}

FString USaveGameComponent::GetSlotName(bool bAutosave)
{
	FString slotName = CurrentID;

	if (bAutosave) {
		slotName += FString::FromInt(AutosaveNum);

		AutosaveNum++;

		if (AutosaveNum > 3)
			AutosaveNum -= 3;

		StartAutosaveTimer();
	}
	else
		slotName += FDateTime::Now().ToString();

	return slotName;
}

void USaveGameComponent::StartAutosaveTimer()
{
	int32 timer = Camera->Settings->GetAutosaveTimer();

	if (timer == 0)
		return;

	Camera->CitizenManager->CreateTimer("AutosaveTimer", Camera, timer * 60.0f, FTimerDelegate::CreateUObject(this, &USaveGameComponent::SaveGameSave, true), false, true);
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