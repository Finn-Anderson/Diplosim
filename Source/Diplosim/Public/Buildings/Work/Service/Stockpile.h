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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stockpile")
		TMap<TSubclassOf<class AResource>, bool> Store;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stockpile")
		int32 StorageCap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
		class UHierarchicalInstancedStaticMeshComponent* HISMBox;

	virtual void Enter(class ACitizen* Citizen) override;

	void ShowBoxesInStockpile();

	UFUNCTION(BlueprintCallable)
		void SetStoreResoruce(TSubclassOf<class AResource> Resource, bool bStore);

	bool DoesStoreResource(TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable)
		int32 GetStoredResourceAmount(TSubclassOf<class AResource> Resource);

	int32 GetFreeStorage();

	virtual bool IsCapacityFull() override;

protected:
	virtual void BeginPlay() override;
};
