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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* TileMesh;
};
