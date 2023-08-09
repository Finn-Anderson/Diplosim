#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Citizen.generated.h"

enum ESex 
{
	Male	UMETA(DisplayName = "Male"),
	Female	UMETA(DisplayName = "Female")
};

UCLASS()
class DIPLOSIM_API ACitizen : public ACharacter
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* CitizenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY()
		class AAIController* AIController;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void MoveTo(AActor* Location);

	UPROPERTY()
		AActor* Goal;


	// Money
	UPROPERTY()
		int32 Balance;


	// Buildings
	class AWork* Employment;

	class AHouse* House;


	// Resources
	void Carry(class AResource* Resource);

	int32 Carrying; 


	// Energy
	void StartLoseEnergyTimer();

	void LoseEnergy();

	void StartGainEnergyTimer();

	void GainEnergy();

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
