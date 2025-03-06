#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager.generated.h"

USTRUCT(BlueprintType)
struct FResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<TSubclassOf<class ABuilding>> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Committed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Stored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		FString Category;

	FResourceStruct()
	{
		Type = nullptr;
		Committed = 0;
		Value = 0;
		Stored = 0;
		Category = "";
	}

	bool operator==(const FResourceStruct& other) const
	{
		return (other.Type == Type);
	}
};

USTRUCT(BlueprintType)
struct FTrendStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 LastHourAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TMap<int32, int32> HourlyTrend;

	FTrendStruct()
	{
		Type = nullptr;
		Category = "";
		LastHourAmount = 0;

		for (int32 i = 0; i < 24; i++)
			HourlyTrend.Add(i, 0);
	}

	bool operator==(const FTrendStruct& other) const
	{
		return (other.Type == Type);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResourceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceManager();

	UPROPERTY()
		class ADiplosimGameModeBase* GameMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceStruct> ResourceList;

	void StoreBasket(TSubclassOf<class AResource> Resource, class ABuilding* Building);

	void AddCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount);

	void TakeCommittedResource(TSubclassOf<class AResource> Resource, int32 Amount);

	int32 AddLocalResource(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	bool AddUniversalResource(TSubclassOf<class AResource> Resource, int32 Amount);

	bool TakeLocalResource(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	bool TakeUniversalResource(TSubclassOf<class AResource> Resource, int32 Amount, int32 Min);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetResourceAmount(TSubclassOf<class AResource> Resource);

	TArray<TSubclassOf<class AResource>> GetResources(class ABuilding* Building);

	TArray<TSubclassOf<class ABuilding>> GetBuildings(TSubclassOf<class AResource> Resource);

	void GetNearestStockpile(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	void UpdateResourceUI(TSubclassOf<class AResource> Resource);

	// Interest
	void Interest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		TSubclassOf<class AResource> Money;

	UPROPERTY()
		FTimerHandle InterestTimer;

	// Trade
	void RandomiseMarket();

	void SetTradeValues();

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetStoredOnMarket(TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetMarketValue(TSubclassOf<class AResource> Resource);

	UPROPERTY()
		FTimerHandle ValueTimer;

	// Trends
	void SetTrendOnHour(int32 Hour);

	UFUNCTION(BlueprintCallable)
		int32 GetResourceTrend(TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable)
		int32 GetCategoryTrend(FString Category);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FTrendStruct> TrendList;
};
