#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "InternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AInternalProduction : public AWork
{
	GENERATED_BODY()
	
public:
	AInternalProduction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 MinYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 MaxYield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		float TimeLength;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void Produce(class ACitizen* Citizen);
};
