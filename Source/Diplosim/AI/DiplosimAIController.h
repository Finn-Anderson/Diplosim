#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DiplosimAIController.generated.h"

USTRUCT()
struct FClosestStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	double Magnitude;

	FClosestStruct()
	{
		Actor = nullptr;
		Magnitude = 0.0f;
	}
};

UCLASS()
class DIPLOSIM_API ADiplosimAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ADiplosimAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	FAIMoveRequest CreateMoveRequest();

	FClosestStruct GetClosestActor(AActor* CurrentActor, AActor* NewActor, int32 CurrentValue = 1, int32 NewValue = 1);

	FVector GetLocationOrDestination(FAIMoveRequest MoveReq);

	bool CanMoveTo(AActor* Actor);

	virtual void AIMoveTo(AActor* Actor);

	FAIMoveRequest MoveRequest;
};
