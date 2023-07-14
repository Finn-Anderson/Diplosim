#include "Building.h"

#include "Kismet/GameplayStatics.h"

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

	Storage = 0;
	StorageCap = 1000;

	EcoStatus = Poor;

	Blueprint = true;

	InternalProd = true;

	AtWork = 0;
	TimeLength = 60.0f;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::FindCitizens, 30.0f, true, 2.0f);

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
	else if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);
		AtWork += 1;

		if (AtWork == 1) {
			GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &ABuilding::Production, c), (TimeLength / AtWork), true);
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
	else if (OtherActor->IsA<ACitizen>()) {
		AtWork -= 1;

		if (AtWork == 0) {
			GetWorldTimerManager().ClearTimer(ProdTimer);
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

void ABuilding::FindCitizens() 
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	int32 tax = 0;

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);
		ABuilding* e = Cast<ABuilding>(c->Employment);

		if (e == nullptr) {
			AddCitizen(c);
		}
	}
}

void ABuilding::AddCitizen(ACitizen* citizen)
{
	if (Occupied.Num() != Capacity) {
		Occupied.Add(citizen);

		citizen->Employment = this;
		citizen->LookForHouse();

		citizen->ResourceActor = ActorToGetResource;

		citizen->MoveTo(this);
	}
	else {
		GetWorldTimerManager().ClearTimer(FindTimer);
	}
}

void ABuilding::RemoveCitizen(ACitizen* citizen)
{
	if (Occupied.Num() != 0) {
		Occupied.Remove(citizen);

		citizen->Employment = nullptr;

		GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::FindCitizens, 30.0f, true);
	}

	if (Occupied.Num() == 0) {
		GetWorldTimerManager().ClearTimer(ProdTimer);
	}
}

int32 ABuilding::GetCapacity()
{
	return Capacity;
}

TArray<class ACitizen*> ABuilding::GetOccupied()
{
	return Occupied;
}

void ABuilding::Production(ACitizen* Citizen)
{

}