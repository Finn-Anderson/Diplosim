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

enum ESex 
{
	Male	UMETA(DisplayName = "Male"),
	Female	UMETA(DisplayName = "Female")
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

	virtual void OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	// Money
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;


	// Buildings
	class AWork* Employment;

	class AHouse* House;


	// Resources
	void HarvestResource(class AResource* Resource);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	FCarryStruct Carrying;


	// Energy
	void StartLoseEnergyTimer();

	void LoseEnergy();

	void StartGainEnergyTimer(int32 Max);

	void GainEnergy(int32 Max);

	FTimerHandle EnergyTimer;

	int32 Energy;


	// Age
	void Birthday();

	int32 Age;


	// Sex
	TEnumAsByte<ESex> Sex;

	void SetSex();

	void SetPartner(ACitizen* Citizen);

	void HaveChild();

	FTimerHandle ChildTimer;

	class ACitizen* Partner;
};
