#pragma once

#include "InteractableInterface.generated.h"

USTRUCT()
struct FInfoStruct
{
	GENERATED_USTRUCT_BODY()

	float Percentage;

	// Extra info such as personality and other. Percentage used for storage and hp.

	FInfoStruct() 
	{
		Percentage = -1;
	}
}

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class IInteractableInterface
{
	GENERATED_BODY()

public:
	FName ActorName;

	FString Description;

	TMap<FString, FInfoStruct> Information;
};