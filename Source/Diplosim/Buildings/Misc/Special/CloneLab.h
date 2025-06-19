#pragma once

#include "CoreMinimal.h"
#include "Buildings/Misc/Special/Special.h"
#include "CloneLab.generated.h"

UCLASS()
class DIPLOSIM_API ACloneLab : public ASpecial
{
	GENERATED_BODY()
	
public:
	ACloneLab();

public:
	virtual void OnBuilt() override;

	virtual void Production(class ACitizen* Citizen) override;

	virtual void SetTimer() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clone")
		TSubclassOf<class AClone> Clone;
};
