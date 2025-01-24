#include "Buildings/Work/Service/Religion.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

#include "Buildings/House.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

AReligion::AReligion()
{
	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	DecalComponent->SetVisibility(true);
}

void AReligion::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	Citizen->bWorshipping = true;
}

void AReligion::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	Citizen->bWorshipping = false;
}

void AReligion::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (!OtherActor->IsA<AHouse>())
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	house->Religions.Add(Belief);

	Houses.Add(house);
}

void AReligion::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (!Houses.Contains(OtherActor))
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	house->Religions.RemoveSingle(Belief);

	Houses.Remove(house);
}