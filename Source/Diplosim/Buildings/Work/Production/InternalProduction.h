#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "InternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AInternalProduction : public AWork
{
	GENERATED_BODY()
	
public:
	AInternalProduction();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	float GetTime();

	void SetTimer(class ACitizen* Citizen);

	void AlterTimer();

	void PauseTimer(bool bPause);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 MinYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 MaxYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 TimeLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		TArray<FItemStruct> Intake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overlap")
		TSubclassOf<AResource> ResourceToOverlap;
};
