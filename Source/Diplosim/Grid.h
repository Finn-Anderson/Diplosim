#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid.generated.h"

UCLASS()
class DIPLOSIM_API AGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	AGrid();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Water;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Ground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TSubclassOf<class ATile> Hill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions")
		int32 Size;

	UPROPERTY()
		class ACamera* Camera;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void Render();
};
