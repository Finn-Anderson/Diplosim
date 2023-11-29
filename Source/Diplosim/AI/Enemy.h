#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Enemy.generated.h"

UCLASS()
class DIPLOSIM_API AEnemy : public AAI
{
	GENERATED_BODY()
	
public:
	AEnemy();

protected:
	void BeginPlay();

public:
	virtual void Tick(float DeltaTime) override;

	void MoveToBroch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AWatchtower> WatchtowerClass;
};
