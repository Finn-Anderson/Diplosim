#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Trader.generated.h"

USTRUCT(BlueprintType)
struct FValueStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		int32 Value;

	FValueStruct()
	{
		Resource = nullptr;
		Value = 0;
	}
	
	bool operator==(const FValueStruct& other) const
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

	void SubmitOrder(class ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FValueStruct> ResourceValues;

	FTimerHandle WaitTimer;
};
