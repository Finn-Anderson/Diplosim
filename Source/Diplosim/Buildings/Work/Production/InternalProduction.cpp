#include "InternalProduction.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"

AInternalProduction::AInternalProduction()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(1.0f);

	MinYield = 1;
	MaxYield = 5;

	TimeLength = 10;
	Timer = 0;
}

void AInternalProduction::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Timer++;

	float tally = 0;

	for (ACitizen* citizen : GetCitizensAtBuilding())
		tally += FMath::LogX(citizen->InitialSpeed, citizen->GetCharacterMovement()->MaxWalkSpeed);

	float value = tally / GetCitizensAtBuilding().Num();

	if (Timer >= (TimeLength / GetCitizensAtBuilding().Num() / value))
		Production(GetCitizensAtBuilding()[0]);
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	for (FItemStruct item : Intake) {
		if (item.Use > item.Stored) {
			CheckStored(Citizen, Intake);

			return;
		}
	}

	if (GetCitizensAtBuilding().Num() == 1)
		SetActorTickEnabled(true);
}

void AInternalProduction::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (GetCitizensAtBuilding().IsEmpty())
		SetActorTickEnabled(false);
}

void AInternalProduction::Production(ACitizen* Citizen)
{
	Citizen->Carry(Camera->ResourceManager->GetResources(this)[0]->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

	for (ACitizen* citizen : GetCitizensAtBuilding())
		Camera->CitizenManager->Injure(citizen);

	Timer = 0;

	Super::Production(Citizen);

	for (FItemStruct item : Intake) {
		if (item.Use > item.Stored) {
			for (ACitizen* citizen : GetCitizensAtBuilding())
				CheckStored(citizen, Intake);

			SetActorTickEnabled(false);
		}
	}
}