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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* AIMesh;

	UPROPERTY()
		class AAIController* AIController;

	// Fighting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		class UAttackComponent* AttackComponent;

	// Movement
	AActor* GetClosestActor(AActor* Actor, AActor* CurrentLocation, AActor* NewLocation, int32 CurrentValue = 1, int32 NewValue = 1);

	bool CanMoveTo(AActor* Location);

	void MoveTo(AActor* Location);

	AActor* Goal;

};
