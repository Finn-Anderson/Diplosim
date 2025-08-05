#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Special.generated.h"


UCLASS()
class DIPLOSIM_API ASpecial : public AInternalProduction
{
	GENERATED_BODY()
	
public:
	ASpecial();

	virtual void Rebuild() override;

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
		UStaticMesh* ReplacementMesh;
};
