#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DiplosimSaveGameData.generated.h"

USTRUCT(BlueprintType)
struct FSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString SaveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString Period;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 Hour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		int32 CitizenNum;

	UPROPERTY()
		bool bAutosave;

	FSaveData()
	{
		SaveName = "";
		Period = "";
		Day = 0;
		Hour = 0;
		CitizenNum = 0;
		bAutosave = false;
	}

	bool operator==(const FSaveData& other) const
	{
		return (other.SaveName == SaveName);
	}
};

UCLASS()
class DIPLOSIM_API UDiplosimSaveGameData : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		TArray<FSaveData> SavedData;

	UPROPERTY()
		FDateTime LastTimeUpdated;
};
