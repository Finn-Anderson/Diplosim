#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConstructionManager.generated.h"

UENUM()
enum class EBuildStatus : uint8
{
	Construction,
	Damaged
};

USTRUCT()
struct FConstructionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		class ABuilding* Building;

	UPROPERTY()
		class ABuilder* Builder;

	UPROPERTY()
		EBuildStatus Status;

	UPROPERTY()
		int32 BuildPercentage;

	FConstructionStruct()
	{
		Building = nullptr;
		Builder = nullptr;
		Status = EBuildStatus::Construction;
		BuildPercentage = 0;
	}

	bool operator==(const FConstructionStruct& other) const
	{
		return (other.Building == Building) || (Builder != nullptr && other.Builder == Builder);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UConstructionManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UConstructionManager();

	void AddBuilding(class ABuilding* Building, EBuildStatus Status);

	void RemoveBuilding(class ABuilding* Building);

	void FindBuilder(class ABuilding* Building);

	void FindConstruction(class ABuilder* Builder);

	UFUNCTION(BlueprintCallable)
		bool IsBeingConstructed(class ABuilding* Building, class ABuilder* Builder);

	UFUNCTION(BlueprintCallable)
		bool IsRepairJob(class ABuilding* Building, class ABuilder* Builder);

	UFUNCTION(BlueprintCallable)
		class ABuilder* GetBuilder(class ABuilding* Building);

	class ABuilding* GetBuilding(class ABuilder* Builder);

	bool IncrementBuildPercentage(class ABuilding* Building);

	UFUNCTION(BlueprintCallable)
		int32 GetBuildPercentage(class ABuilding* Building);

	bool IsFactionConstructingBuilding(FString FactionName, class ABuilding* Building);

	UPROPERTY()
		TArray<FConstructionStruct> Construction;
};
