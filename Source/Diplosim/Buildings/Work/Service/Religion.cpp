#include "Buildings/Work/Service/Religion.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

#include "Buildings/House.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

ABroadcast::ABroadcast()
{
	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	DecalComponent->SetVisibility(true);

	Belief = "Atheist";
	Allegiance = "Undecided";
	bIsPark = false;
	bHolyPlace = false;
}

void ABroadcast::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (bHolyPlace)
		Citizen->bWorshipping = true;
}

void ABroadcast::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (bHolyPlace)
		Citizen->bWorshipping = false;
}

void ABroadcast::SetBroadcastType(FString Party, FString Religion)
{
	Belief = Religion;
	Allegiance = Party;

	for (int32 i = GetOccupied().Num() - 1; i > -1; i--)
		if (!GetOccupied()[i]->CanWork(this))
			RemoveCitizen(GetOccupied()[i]);
}

void ABroadcast::RemoveInfluencedMaterial(AHouse* House)
{
	bool bAnotherHighlighting = false;

	for (ABroadcast* broadcaster : House->Influencers) {
		if (!Camera->BuildComponent->Buildings.Contains(broadcaster))
			continue;

		bAnotherHighlighting = true;

		break;
	}

	if (bAnotherHighlighting)
		return;

	House->BuildingMesh->SetOverlayMaterial(nullptr);
}

void ABroadcast::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (!OtherActor->IsA<AHouse>())
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	Houses.Add(house);

	house->Influencers.Add(this);

	if (Camera->BuildComponent->Buildings.Contains(this) && house->BuildingMesh->GetOverlayMaterial() != InfluencedMaterial)
		house->BuildingMesh->SetOverlayMaterial(InfluencedMaterial);
}

void ABroadcast::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (!Houses.Contains(OtherActor))
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	Houses.Add(house);

	house->Influencers.RemoveSingle(this);

	if (Camera->BuildComponent->Buildings.Contains(this)) {
		
	}
}