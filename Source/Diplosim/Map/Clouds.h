#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Clouds.generated.h"

UCLASS()
class DIPLOSIM_API AClouds : public AActor
{
	GENERATED_BODY()
	
public:	
	AClouds();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMClouds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		TSubclassOf<class AGrid> GridClass;

	float X;
	float Y;

	void GetCloudBounds();

	void CloudSpawner();
};
