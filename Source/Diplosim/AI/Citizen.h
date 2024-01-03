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

	class ACitizen* Mother;

	class ACitizen* Father;

	class ACitizen* Partner;

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

UCLASS()
class DIPLOSIM_API ACitizen : public AAI
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		class UCapsuleComponent* CapsuleCollision;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
		
	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Money
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;


	// Buildings
	FBuildingStruct Building;

	TArray<AActor*> StillColliding;


	// Resources
	void StartHarvestTimer(AResource* Resource);

	void HarvestResource(class AResource* Resource);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	FCarryStruct Carrying;


	// Food
	void Eat();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
		TArray<TSubclassOf<AResource>> Food;

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
