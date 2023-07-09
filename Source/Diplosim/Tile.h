#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

UENUM()
enum Type
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* TileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TEnumAsByte<Type> Type;

	UPROPERTY()
		int32 Fertility;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void SetFertility(int32 Mean);

	int32 GetFertility();
};
