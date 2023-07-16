#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResourceManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResourceManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<FString> ResourceNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
		TArray<int32> ResourceAmounts;

	void ChangeResource(FString Name, int32 Change);

	int32 GetResource(FString Name);
};
