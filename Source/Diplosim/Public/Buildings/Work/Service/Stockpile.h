#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Stockpile.generated.h"

UCLASS()
class DIPLOSIM_API AStockpile : public AWork
{
	GENERATED_BODY()
	
public:
	AStockpile();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stockpile")
		TMap<TSubclassOf<class AResource>, bool> Store;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stockpile")
		int32 StorageCap;

	UPROPERTY()
		TMap<class ACitizen*, FItemStruct> Gathering;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMBox;

	virtual void Enter(class ACitizen* Citizen) override;

	bool DoesStoreResource(TSubclassOf<class AResource> Resource);

	void SetItemToGather(TSubclassOf<class AResource> Resource, ACitizen* Citizen, ABuilding* Building);

	FItemStruct GetItemToGather(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		int32 GetStoredResourceAmount(TSubclassOf<class AResource> Resource);

private:
	void ShowBoxesInStockpile();
};
