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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
		TArray<class ACitizen*> Citizens;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
		class ABuilding* Building;

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

	void SetOccupied();

	void ExecuteEditEvent(FString Key);
};