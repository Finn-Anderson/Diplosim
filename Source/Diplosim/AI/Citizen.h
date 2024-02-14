#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Citizen.generated.h"

USTRUCT()
struct FCarryStruct
{
	GENERATED_USTRUCT_BODY()

	class AResource* Type;

	int32 Amount;

	FCarryStruct()
	{
		Type = nullptr;
		Amount = 0;
	}
};

USTRUCT()
struct FBuildingStruct
{
	GENERATED_USTRUCT_BODY()

	class ABuilding* House;

	class ABuilding* Employment;

	class ABuilding* BuildingAt;

	FVector EnterLocation;

	FBuildingStruct()
	{
		House = nullptr;
		Employment = nullptr;
		BuildingAt = nullptr;
		EnterLocation = FVector(0.0f, 0.0f, 0.0f);
	}
};

UENUM()
enum class ESex : uint8
{
	NaN,
	Male,
	Female
};

USTRUCT()
struct FBioStruct
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<class ACitizen> Mother;

	TWeakObjectPtr<class ACitizen> Father;

	TWeakObjectPtr<class ACitizen> Partner;

	ESex Sex;

	int32 Age;

	FBioStruct()
	{
		Mother = nullptr;
		Father = nullptr;
		Partner = nullptr;
		Sex = ESex::NaN;
		Age = 0;
	}
};

USTRUCT()
struct FCollidingStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	int32 Instance;

	FCollidingStruct()
	{
		Actor = nullptr;
		Instance = -1;
	}

	bool operator==(const FCollidingStruct& other) const
	{
		return (other.Actor == Actor) && (other.Instance == Instance);
	}
};

UCLASS()
class DIPLOSIM_API ACitizen : public AAI
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	virtual void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
		
	virtual void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	// Money
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;


	// Buildings
	FBuildingStruct Building;

	TArray<FCollidingStruct> StillColliding;


	// Resources
	void StartHarvestTimer(class AResource* Resource, int32 Instance);

	void HarvestResource(class AResource* Resource, int32 Instance);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	FCarryStruct Carrying;


	// Food
	void Eat();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
		TArray<TSubclassOf<class AResource>> Food;

	FTimerHandle HungerTimer;

	int32 Hunger;


	// Energy
	void SetEnergyTimer(bool bGain);

	void LoseEnergy();

	void GainEnergy();

	FTimerHandle EnergyTimer;

	int32 Energy;


	// Bio
	void Birthday();

	void SetSex();

	void FindPartner();

	void SetPartner(ACitizen* Citizen);

	void HaveChild();

	FBioStruct BioStruct;

	FTimerHandle ChildTimer;
};
