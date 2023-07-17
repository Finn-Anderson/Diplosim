#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

UCLASS()
class DIPLOSIM_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATile();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* TileMesh;

	int32 Fertility;

public:	
	void SetFertility(int32 Mean);

	int32 GetFertility();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
		TSubclassOf<class AResource> Tree;

	UPROPERTY()
		TArray<class AResource*> Trees;

	FTimerHandle TreeTimer;

	void GenerateTree();
};
