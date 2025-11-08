#pragma once

#include "CoreMinimal.h"
#include "DiplosimSaveGame.h"
#include "Components/ActorComponent.h"
#include "SaveGameComponent.generated.h"

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

	UPROPERTY()
		class ACamera* Camera;

	UFUNCTION(BlueprintCallable)
		void StartNewSave();

	UFUNCTION(BlueprintCallable)
		void SaveGameSave(FString Name, bool bAutosave = false);

	UFUNCTION(BlueprintCallable)
		void LoadGameSave(FString SlotName, class UDiplosimSaveGame* SaveGame, int32 Index);

	UFUNCTION(BlueprintCallable)
		void DeleteGameSave(FString SlotName, class UDiplosimSaveGame* SaveGame, int32 Index, bool bSlot);

	UFUNCTION(BlueprintCallable)
		TMap<FString, class UDiplosimSaveGame*> LoadAllSavedGames();

	void CreateNewSaveStruct(FString Name, bool bAutosave, TArray<FActorSaveData> NewActorData);

	void CapAutosaves();

	void StartAutosaveTimer();

	void UpdateAutosave(int32 NewTime);

	AActor* GetSaveActorFromName(TArray<FActorSaveData> SavedData, FString Name);

	// Save Setup
	void SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FActorSaveData CitizenData, bool bVisitor);
};
