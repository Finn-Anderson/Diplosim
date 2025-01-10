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

	UPROPERTY()
		TMap<class ACitizen*, FItemStruct> Gathering;

	virtual void Enter(class ACitizen* Citizen) override;

	FItemStruct GetItemToGather(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
	int32 GetStoredResourceAmount(TSubclassOf<class AResource> Resource);
};
