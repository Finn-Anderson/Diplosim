#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI.generated.h"

UCLASS()
class DIPLOSIM_API AAI : public ACharacter
{
	GENERATED_BODY()

public:
	AAI();

protected:
	virtual void BeginPlay() override;

public:
	void MoveToBroch();

	// Fighting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	// Movement
	class ADiplosimAIController* AIController;
};
