#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

class ATile;

UCLASS()
class DIPLOSIM_API AGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrid();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ATile* Water;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ATile* Ground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ATile* Hill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 size;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void Render();
};
