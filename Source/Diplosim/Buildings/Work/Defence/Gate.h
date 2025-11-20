#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Gate.generated.h"

UCLASS()
class DIPLOSIM_API AGate : public AWall
{
	GENERATED_BODY()
	
public:
	AGate();

	virtual void Enter(class ACitizen* Citizen) override;

	void OpenGate();

	void CloseGate();

	UFUNCTION()
		void UpdateNavigation();

	void SetTimer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class USkeletalMeshComponent* RightGate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class USkeletalMeshComponent* LeftGate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UAnimationAsset* CloseAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UAnimationAsset* OpenAnim;

	UPROPERTY()
		bool bOpen;
};
