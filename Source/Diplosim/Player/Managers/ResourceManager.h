#pragma once

#include "CoreMinimal.h"
#include "Player/Managers/ConquestManager.h"
#include "Components/ActorComponent.h"
#include "ResourceManager.generated.h"

USTRUCT(BlueprintType)
struct FResourceStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TMap<TSubclassOf<class ABuilding>, int32> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Stored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		UTexture2D* Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<int32> TextureDimensions;

	FResourceStruct()
	{
		Type = nullptr;
		Value = 0;
		Stored = 0;
		Category = "";

		Texture = nullptr;
		TextureDimensions = { 32, 32 };
	}

	bool operator==(const FResourceStruct& other) const
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		TSubclassOf<class AResource> Money;

	void StoreBasket(TSubclassOf<class AResource> Resource, class ABuilding* Building);

	FFactionResourceStruct* GetFactionResourceStruct(FFactionStruct* Faction, TSubclassOf<class AResource> Resource);

	void AddCommittedResource(FFactionStruct* Faction, TSubclassOf<class AResource> Resource, int32 Amount);

	void TakeCommittedResource(FFactionStruct* Faction, TSubclassOf<class AResource> Resource, int32 Amount);

	int32 AddLocalResource(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	bool AddUniversalResource(FFactionStruct* Faction, TSubclassOf<class AResource> Resource, int32 Amount);

	bool TakeLocalResource(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	bool TakeUniversalResource(FFactionStruct* Faction, TSubclassOf<class AResource> Resource, int32 Amount, int32 Min);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetResourceAmount(FString FactionName, TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetResourceCapacity(FString FactionName, TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		float GetBuildingCapacityPercentage(class ABuilding* Building);

	TMap<TSubclassOf<class AResource>, int32> GetBuildingCapacities(class ABuilding* Building, TSubclassOf<class AResource> Resource = nullptr);

	TArray<TSubclassOf<class AResource>> GetResources(class ABuilding* Building);

	TMap<TSubclassOf<class ABuilding>, int32> GetBuildings(TSubclassOf<class AResource> Resource);

	TArray<class ABuilding*> GetBuildingsOfClass(FFactionStruct* Faction, TSubclassOf<AActor> Class);

	void GetNearestStockpile(TSubclassOf<class AResource> Resource, class ABuilding* Building, int32 Amount);

	void UpdateResourceUI(FFactionStruct* Faction, TSubclassOf<class AResource> Resource);

	void UpdateResourceCapacityUI(ABuilding* Building);

	TArray<TSubclassOf<class AResource>> GetResourcesFromCategory(FString Category);

	// Trade
	void RandomiseMarket();

	void SetTradeValues();

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetStoredOnMarket(TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable, Category = "Resource")
		int32 GetMarketValue(TSubclassOf<class AResource> Resource);

	// Trends
	void SetTrendOnHour(int32 Hour);

	UFUNCTION(BlueprintCallable)
		int32 GetResourceTrend(FString FactionName, TSubclassOf<class AResource> Resource);

	UFUNCTION(BlueprintCallable)
		int32 GetCategoryTrend(FString FactionName, FString Category);
};
