#pragma once

#include "CoreMinimal.h"
#include "Production.h"
#include "InternalProduction.generated.h"

UCLASS()
class DIPLOSIM_API AInternalProduction : public AProduction
{
	GENERATED_BODY()
	
public:
	AInternalProduction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		float TimeLength;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void ProductionDone(class ACitizen* Citizen);
};
