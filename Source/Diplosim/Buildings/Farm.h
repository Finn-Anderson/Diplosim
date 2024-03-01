#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Farm.generated.h"

UCLASS()
class DIPLOSIM_API AFarm : public AWork
{
	GENERATED_BODY()

public:
	AFarm();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* CropMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
		int32 Yield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
		int32 TimeLength;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void ProductionDone(class ACitizen* Citizen);
};
