#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

UENUM()
enum EType
{
	Water	UMETA(DisplayName = "Water"),
	Ground	UMETA(DisplayName = "Ground"),
	Hill	UMETA(DisplayName = "Hill"),
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TEnumAsByte<EType> Type;

	UPROPERTY()
		int32 Fertility;

	int32 Trees;

public:	
	void SetFertility(int32 Mean);

	int32 GetFertility();

	TEnumAsByte<EType> GetType();
};
