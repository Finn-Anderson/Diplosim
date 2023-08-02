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
		int32 Amount;

	FResourceStruct()
	{
		Type = nullptr;
		Amount = 0;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResourceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FResourceStruct> ResourceList;

	void ChangeResource(TSubclassOf<class AResource> Resource, int32 Change);

	int32 GetResourceAmount(TSubclassOf<class AResource> Resource);

public:
	// UI
	UFUNCTION(BlueprintCallable)
		FResourceStruct GetUpdatedResource();

	FResourceStruct ResourceStruct;
};
