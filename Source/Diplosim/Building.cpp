#include "Building.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	Category = House;

	Wood = 0;
	Stone = 0;
	Money = 0;
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
		Blocked = true;
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA<ABuilding>())
	{
		Blocked = false;
	}
}

bool ABuilding::IsBlocked()
{
	return Blocked;
}