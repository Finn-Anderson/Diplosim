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

UCLASS()
class DIPLOSIM_API ATrader : public AWork
{
	GENERATED_BODY()
	
public:
	ATrader();

	virtual void Enter(class ACitizen* Citizen) override;

	void SubmitOrder(class ACitizen* Citizen);

	void ReturnResource(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void SetNewOrder(FQueueStruct Order);

	UFUNCTION(BlueprintCallable)
		void SetOrderWidget(int32 index, class UWidget* Widget);

	UFUNCTION(BlueprintCallable)
		void SetOrderCancelled(int32 index, bool bCancel);

	UPROPERTY()
		FTimerHandle WaitTimer;

	UPROPERTY(BlueprintReadOnly, Category = "Resource")
		TArray<FQueueStruct> Orders;
};
