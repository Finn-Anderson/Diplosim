#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Research.generated.h"

UCLASS()
class DIPLOSIM_API AResearch : public AWork
{
	GENERATED_BODY()
	
public:
	AResearch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 TimeLength;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	float GetTime();

	void SetResearchTimer();

	void UpdateResearchTimer();

	virtual void Production(class ACitizen* Citizen) override;
};
