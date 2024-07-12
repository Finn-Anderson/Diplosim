#include "InternalProduction.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"

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

	if (Timer == (TimeLength / GetCitizensAtBuilding().Num()))
		Production(GetCitizensAtBuilding()[0]);
}

void AInternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

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
	Citizen->Carry(Camera->ResourceManager->GetResource(this)->GetDefaultObject<AResource>(), FMath::RandRange(MinYield, MaxYield), this);

	Timer = 0;

	Super::Production(Citizen);
}