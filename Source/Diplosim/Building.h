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

	TArray<class ACitizen*> AtWork;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
		float TimeLength;

	class ACamera* Camera;

public:
	// Resource
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> ActorToGetResource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		FString Produce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Storage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 StorageCap;

	FTimerHandle ProdTimer;

	virtual void Production(class ACitizen* Citizen);

	void Store(int32 Amount, class ACitizen* Citizen);


	// Building cost
	int32 Wood;

	int32 Stone;

	int32 Money;

	int32 Upkeep;

	bool BuildCost();

	void UpkeepCost();

public:
	// Building prevent overlaps
	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool IsBlocked();

	TArray<class AActor*> Blocked;


	// Remove
	void DestroyBuilding();

	
	// Building capacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		TEnumAsByte<EEconomy> EcoStatus;

	void FindCitizens();

	FTimerHandle FindTimer;

	void AddCitizen(class ACitizen* citizen);

	void RemoveCitizen(class ACitizen* citizen);

	int32 Capacity;

	TArray<class ACitizen*> Occupied;

	int32 GetCapacity();

	TArray<class ACitizen*> GetOccupied();
};
