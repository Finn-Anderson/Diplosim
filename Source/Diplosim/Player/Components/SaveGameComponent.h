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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY()
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
		TMap<FString, class UDiplosimSaveGame*> LoadAllSavedGames();

	FSave* CreateNewSaveStruct(FString Name, bool bAutosave);

	void CapAutosaves();

	void StartAutosaveTimer();

	void UpdateAutosave(int32 NewTime);
};
