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
	// Buildings
	class ABuilding* Employment;

	class ABuilding* House;

	void CheckEmployment();

	void CheckHousing();


	// Resources
	class AResource* Carrying; // A Resource has types, which will go into a resource manager component to tally up. GMBase will deal with taxes.
};
