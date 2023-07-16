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
	UPROPERTY()
		class AAIController* aiController;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void LookForHouse();

	void MoveTo(AActor* Location);

	FTimerHandle HouseTimer;

	UPROPERTY()
		AActor* Goal;

public:
	// Resources
	UPROPERTY()
		TSubclassOf<class AResource> ResourceActor;

public:
	// Energy
	void LoseEnergy();

	void GainEnergy();

	FTimerHandle EnergyTimer;

	int32 Energy;

public:	
	// Buildings
	UPROPERTY()
		class AActor* Employment;

	UPROPERTY()
		class AActor* House;


	// Resources
	void Carry(class AResource* Resource);

	int32 Carrying; 
};
