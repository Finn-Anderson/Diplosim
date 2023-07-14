#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resource.generated.h"

UCLASS()
class DIPLOSIM_API AResource : public AActor
{
	GENERATED_BODY()
	
public:	
	AResource();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	int32 GetYield();

	int32 Yield;
};
