#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Portal.generated.h"

UCLASS()
class DIPLOSIM_API APortal : public ABuilding
{
	GENERATED_BODY()
	
public:
	APortal();

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0) override;

	virtual void Enter(class ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
		UStaticMesh* ReplacementMesh;
};
