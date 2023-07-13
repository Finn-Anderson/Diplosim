#include "Building.h"

#include "Citizen.h"
#include "Tile.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	Category = House;

	Capacity = 2;

	Wood = 0;
	Stone = 0;
	Money = 0;

	EcoStatus = Poor;

	Blueprint = true;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	BuildingMesh->OnComponentBeginOverlap.AddDynamic(this, &ABuilding::OnOverlapBegin);
	BuildingMesh->OnComponentEndOverlap.AddDynamic(this, &ABuilding::OnOverlapEnd);
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ABuilding>()) {
		Blocked.Add(OtherActor);
	}
	else if (OtherActor->IsA<ATile>()) {
		ATile* tile = Cast<ATile>(OtherActor);

		if (tile->GetType() == EType::Water) {
			Blocked.Add(OtherActor);
		}
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA<ABuilding>())
	{
		Blocked.Remove(OtherActor);
	}
	else if (OtherActor->IsA<ATile>()) {
		ATile* tile = Cast<ATile>(OtherActor);

		if (tile->GetType() == EType::Water) {
			Blocked.Remove(OtherActor);
		}
	}
}

bool ABuilding::IsBlocked()
{
	bool isBlocked = true;
	if (Blocked.Num() > 0) {
		isBlocked = true;
	}
	else {
		isBlocked = false;
	}

	return isBlocked;
}

void ABuilding::DestroyBuilding()
{
	if (Occupied.Num() > 0) {
		for (int i = 0; i < Occupied.Num(); i++) {
			ACitizen* c = Occupied[i];

			if (Category == House) {
				c->House = nullptr;
			}
			else {
				c->Employment = nullptr;
			}
		}
	}

	Destroy();
}

void ABuilding::AddCitizen(ACitizen* citizen)
{
	if (Occupied.Num() != Capacity) {
		Occupied.Add(citizen);
	}
}

void ABuilding::RemoveCitizen(ACitizen* citizen)
{
	if (Occupied.Num() != 0) {
		Occupied.Remove(citizen);
	}

	// Add citizen to work if available when they come to live, then find closest available house to place. Each new house recalculate based on citizen furthest from work.
}

int32 ABuilding::GetCapacity()
{
	return Capacity;
}

TArray<class ACitizen*> ABuilding::GetOccupied()
{
	return Occupied;
}