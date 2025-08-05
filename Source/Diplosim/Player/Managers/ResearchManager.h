#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResearchManager.generated.h"

USTRUCT(BlueprintType)
struct FResearchStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		FString ResearchName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 MaxLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		float AmountResearched;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		int32 Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		TMap<FString, float> Modifiers;

	FResearchStruct()
	{
		ResearchName = "";
		Level = 0;
		MaxLevel = 20;
		AmountResearched = 0;
		Target = 0;
	}

	bool operator==(const FResearchStruct& other) const
	{
		return (other.ResearchName == ResearchName);
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResearchManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResearchManager();

protected:
	void ReadJSONFile(FString Path);

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		TArray<FResearchStruct> ResearchStruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Research")
		int32 CurrentIndex;

	UFUNCTION(BlueprintCallable)
		bool IsReseached(FString Name);

	void Research(float Amount);
};
