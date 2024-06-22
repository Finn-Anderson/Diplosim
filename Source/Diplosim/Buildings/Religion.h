#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Religion.generated.h"

UCLASS()
class DIPLOSIM_API AReligion : public AWork
{
	GENERATED_BODY()
	
public:
	AReligion();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void FindCitizens() override;

	void Mass();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		float MassLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		float WaitLength;

	bool bMass;

	FTimerHandle MassTimer;

	TArray<class ACitizen*> Worshipping;
};
