#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Farm.generated.h"

UCLASS()
class DIPLOSIM_API AFarm : public AWork
{
	GENERATED_BODY()

public:
	AFarm();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* CropMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
		int32 Yield;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
		int32 TimeLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crop")
		bool bAffectedByFerility;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Production(class ACitizen* Citizen) override;

	void ProductionDone(class ACitizen* Citizen);

	void StartTimer(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		int32 GetFertility();

	UFUNCTION(BlueprintCallable)
		int32 GetYield();
};
