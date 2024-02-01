#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableInterface.generated.h"

USTRUCT(BlueprintType)
struct FTextStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
		FName ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
		FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
		TMap<FString, FString> Information;

	FTextStruct()
	{
		ActorName = "";
		Description = "";
	}
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOSIM_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interface")
		FTextStruct TextStruct;

	class ACamera* Camera;

	void SetHP();

	void SetStorage();

	void SetEnergy();

	void SetHunger();

	void SetQuantity();

	void ExecuteEditEvent(FString Key);
};