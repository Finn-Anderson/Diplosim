#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI.generated.h"

UCLASS()
class DIPLOSIM_API AAI : public AActor
{
	GENERATED_BODY()

public:
	AAI();

protected:
	virtual void BeginPlay() override;

public:
	void MoveToBroch();

	bool CanReach(AActor* Actor, float Reach, int32 Instance = -1);

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class USceneComponent* RootSceneComponent;

	// Fighting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		class UAIMovementComponent* MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		TSubclassOf<class UNavigationQueryFilter> NavQueryFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		class ADiplosimAIController* AIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		float InitialRange;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
		float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
		class UNiagaraSystem* SpawnSystem;
};
