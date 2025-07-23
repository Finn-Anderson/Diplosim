#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI.generated.h"

UCLASS()
class DIPLOSIM_API AAI : public APawn
{
	GENERATED_BODY()

public:
	AAI();

protected:
	virtual void BeginPlay() override;

public:
	void MoveToBroch();

	bool CanReach(AActor* Actor, float Reach, int32 Instance = -1);

	void EnableCollisions(bool bEnable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapsuleComponent")
		class UCapsuleComponent* Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class USkeletalMeshComponent* Mesh;

	UPROPERTY()
		class ACamera* Camera;

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

	class ADiplosimAIController* AIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
		float InitialRange;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
		float Range;
};
