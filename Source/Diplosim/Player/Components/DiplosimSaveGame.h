#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DiplosimSaveGame.generated.h"

USTRUCT()
struct FActorSaveData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UClass* Class;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	TArray<uint8> ByteData;
};

UCLASS()
class DIPLOSIM_API UDiplosimSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
		TArray<FActorSaveData> SavedActors;
};
