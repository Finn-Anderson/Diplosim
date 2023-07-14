#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Citizen.generated.h"

UCLASS()
class DIPLOSIM_API ACitizen : public APawn
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	class AAIController* aiController;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void LookForHouse();

	void MoveTo(AActor* Location);

	FTimerHandle HouseTimer;

	AActor* Goal;

public:
	// Resources
	TSubclassOf<class AResource> ResourceActor;

public:
	// Energy
	FTimerHandle EnergyTimer;

	void LoseEnergy();

	void GainEnergy();

	int32 Energy;

public:	
	// Buildings
	class AActor* Employment;

	class AActor* House;

	void CheckEmployment();

	void CheckHousing();


	// Resources
	int32 Carrying; 

	void Carry(class AResource* Resource);
};
