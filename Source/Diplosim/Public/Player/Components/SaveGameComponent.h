#pragma once

#include "CoreMinimal.h"
#include "DiplosimSaveGame.h"
#include "DiplosimSaveGameData.h"
#include "Components/ActorComponent.h"
#include "SaveGameComponent.generated.h"

UENUM()
enum class EAsyncLoop : uint8
{
	Timer,
	CitizenLoop,
	Disease,
	Interactions,
	Conversations,
	Vandalism,
	Report,
	Fighting,
	CitizenMovement,
	AIMovement,
	BuildingDeath,
	BuildingRotation,
	Faction
};

USTRUCT()
struct FCheckDoneStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY();
		bool bLoad;

	UPROPERTY();
		TMap<EAsyncLoop, bool> CheckLoop;

	FCheckDoneStruct()
	{
		Reset();

		bLoad = false;
	};

	void Reset()
	{
		bLoad = true;

		CheckLoop.Empty();
		for (int32 i = 0; i < StaticEnum<EAsyncLoop>()->NumEnums() - 1; i++)
			CheckLoop.Add(EAsyncLoop(i), false);
	};

	bool bIsAllChecked()
	{
		for (auto element : CheckLoop)
			if (!element.Value)
				return false;

		return true;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API USaveGameComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USaveGameComponent();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString CurrentID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		class UDiplosimSaveGame* CurrentSaveGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		class UDiplosimSaveGameData* CurrentSaveData;

	UPROPERTY()
		int32 CurrentIndex;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		FCheckDoneStruct Checklist;

	UFUNCTION(BlueprintCallable)
		void StartNewSave();

	UFUNCTION(BlueprintCallable)
		void SaveGameSave(FString Name, bool bAutosave = false);

	UFUNCTION(BlueprintCallable)
		void LoadGameSave(FString SlotName, int32 Index);

	bool IsLoading();

	void LoadGameCallback(EAsyncLoop Loop);

	void LoadSave();

	UFUNCTION()
		void OnNavMeshGenerated();

	void CompressAndSave(FString SlotName, UDiplosimSaveGame* SaveGame);

	UDiplosimSaveGame* DecompressSave(FString SlotName);

	UFUNCTION(BlueprintCallable)
		void DeleteGameSave(FString SlotName, class UDiplosimSaveGameData* SaveData, int32 Index, bool bSlot);

	UFUNCTION(BlueprintCallable)
		TMap<FString, class UDiplosimSaveGameData*> LoadAllSavedGames();

	void CreateNewSaveStruct(FString Name, bool bAutosave);

	void CapAutosaves();

	void StartAutosaveTimer();

	void UpdateAutosave(int32 NewTime);

	// Save Setup
	void SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FActorSaveData* CitizenData, FAIData& AIData, bool bVisitor);
};
