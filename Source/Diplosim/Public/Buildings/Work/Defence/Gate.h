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

	void Tick(float DeltaTime) override;

	virtual void Enter(class ACitizen* Citizen) override;

	void OpenGate();

	void CloseGate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UStaticMeshComponent* RightGate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
		class UStaticMeshComponent* LeftGate;

protected:
	UPROPERTY()
		bool bOpen;
};
