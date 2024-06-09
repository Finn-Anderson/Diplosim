#pragma once

#include "CoreMinimal.h"
#include "Buildings/Wall.h"
#include "Gate.generated.h"

UCLASS()
class DIPLOSIM_API AGate : public AWall
{
	GENERATED_BODY()
	
public:
	AGate();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
		void CloseGate(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OpenGate(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void Enter(ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class USphereComponent* EnemyDetectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class USkeletalMeshComponent* RightGate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class USkeletalMeshComponent* LeftGate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UAnimationAsset* CloseAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UAnimationAsset* OpenAnim;

	int32 Enemies;

	bool bOpen;
};
