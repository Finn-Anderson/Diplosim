#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Trader.generated.h"

USTRUCT(BlueprintType)
struct FQueueStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		class UWidget* OrderWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		bool bCancelled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Wait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		bool bRepeat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FItemStruct> SellingItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FItemStruct> BuyingItems;

	FQueueStruct()
	{
		OrderWidget = nullptr;
		bCancelled = false;
		Wait = 0;
		bRepeat = false;
	}
};

USTRUCT(BlueprintType)
struct FMinStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		bool bSell;

	FMinStruct()
	{
		Resource = nullptr;
		Min = 0;
		bSell = false;
	}

	bool operator==(const FMinStruct& other) const
	{
		return (other.Resource == Resource);
	}
};

UCLASS()
class DIPLOSIM_API ATrader : public AWork
{
	GENERATED_BODY()
	
public:
	ATrader();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual bool CheckStored(class ACitizen* Citizen, TArray<FItemStruct> Items);

	virtual bool IsAtWork(class ACitizen* Citizen) override;

	UFUNCTION()
		void SubmitOrder();

	void ReturnResource(class ACitizen* Citizen);

	void ClearOrder();

	UFUNCTION(BlueprintCallable)
		void SetNewOrder(FQueueStruct Order);

	UFUNCTION(BlueprintCallable)
		void SetOrderWidget(int32 index, class UWidget* Widget);

	UFUNCTION(BlueprintCallable)
		void SetOrderCancelled(int32 index, bool bCancel);

	UPROPERTY(BlueprintReadOnly, Category = "Resource")
		TArray<FQueueStruct> Orders;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
		TSubclassOf<class UCameraShakeBase> CannonShake;
};
