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

	UPROPERTY()
		int32 AutosaveNum;

	UPROPERTY()
		class UDiplosimSaveGame* CurrentSaveGame;

	UPROPERTY()
		class ACamera* Camera;

	UFUNCTION(BlueprintCallable)
		void StartNewSave();

	UFUNCTION(BlueprintCallable)
		void SaveGameSave(bool bAutosave = false);

	UFUNCTION(BlueprintCallable)
		void LoadGameSave(FString SlotName);

	FString GetSlotName(bool bAutosave);

	void StartAutosaveTimer();

	void UpdateAutosave(int32 NewTime);
};
