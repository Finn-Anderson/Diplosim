#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DiplosimSaveGame.generated.h"

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		UClass* Class;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		TArray<uint8> ByteData;

	FActorSaveData()
	{
		Class = nullptr;
		Transform = FTransform(FQuat::Identity, FVector(0.0f));
	}
};

USTRUCT(BlueprintType)
struct FSave
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

	UPROPERTY()
		TArray<FActorSaveData> SavedActors;

	FSave()
	{
		SaveName = "";
		Period = "";
		Day = 0;
		Hour = 0;
		CitizenNum = 0;
		bAutosave = false;
	}
	
	bool operator==(const FSave& other) const
	{
		return (other.SaveName == SaveName);
	}
};

UCLASS()
class DIPLOSIM_API UDiplosimSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		TArray<FSave> Saves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
		FString ColonyName;

	UPROPERTY()
		FDateTime LastTimeUpdated;
};
