#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

UENUM()
enum ECategory
{
	House		UMETA(DisplayName = "House"),
	Military	UMETA(DisplayName = "Military"),
	Industry	UMETA(DisplayName = "Industry"),
};

UENUM()
enum EEconomy
{
	Poor	UMETA(DisplayName = "Poor"),
	Modest	UMETA(DisplayName = "Modest"),
	Rich	UMETA(DisplayName = "Rich"),
};

UCLASS()
class DIPLOSIM_API ABuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	ABuilding();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TEnumAsByte<ECategory> Category;

	bool Blueprint;

	UPROPERTY()
		class ACamera* Camera;

	void Run();

	void DestroyBuilding();

	virtual void Action(class ACitizen* Citizen);

public:
	// Building cost
	int32 Wood;

	int32 Stone;

	int32 Money;

	int32 Upkeep;

	bool BuildCost();

	void UpkeepCost();

	UFUNCTION(BlueprintCallable)
		FText GetWood();

	UFUNCTION(BlueprintCallable)
		FText GetStone();

	UFUNCTION(BlueprintCallable)
		FText GetMoney();

public:
	// Building prevent overlaps
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	// Citizens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TEnumAsByte<EEconomy> EcoStatus;

	UPROPERTY()
		TArray<class ACitizen*> Occupied;

	UPROPERTY()
		TArray<class ACitizen*> AtWork;

	int32 Capacity;

	FTimerHandle FindTimer;

	void FindCitizens();

	void AddCitizen(class ACitizen* Citizen);

	void RemoveCitizen(class ACitizen* Citizen);

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();
};
