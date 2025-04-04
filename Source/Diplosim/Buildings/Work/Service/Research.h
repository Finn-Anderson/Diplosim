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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* TurretMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* TelescopeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		int32 TimeLength;

	UPROPERTY()
		FRotator TurretTargetRotation;

	UPROPERTY()
		FRotator TelescopeTargetRotation;

	void Tick(float DeltaTime) override;

	void BeginRotation();

	virtual void Build(bool bRebuild = false, bool bUpgrade = false, int32 Grade = 0) override;

	virtual void OnBuilt() override;

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	float GetTime(int32 time);

	void SetResearchTimer();

	void UpdateResearchTimer();

	virtual void Production(class ACitizen* Citizen) override;
};
