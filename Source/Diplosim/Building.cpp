#include "Building.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "BuildComponent.h"
#include "Vegetation.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	Category = House;

	Capacity = 2;

	Wood = 0;
	Stone = 0;
	Money = 0;

	Upkeep = 0;

	Storage = 0;
	StorageCap = 1000;

	EcoStatus = Poor;

	Blueprint = true;

	TimeLength = 60.0f;

	ActorToGetResource = nullptr;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::FindCitizens, 30.0f, true, 2.0f);
	GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::UpkeepCost, 300.0f, true);

	BuildingMesh->OnComponentBeginOverlap.AddDynamic(this, &ABuilding::OnOverlapBegin);
	BuildingMesh->OnComponentEndOverlap.AddDynamic(this, &ABuilding::OnOverlapEnd);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

bool ABuilding::BuildCost()
{
	UResourceManager* rm = Camera->ResourceManager;
	int32 maxWood = rm->GetResource(TEXT("Wood"));
	int32 maxStone = rm->GetResource(TEXT("Stone"));
	int32 maxMoney = rm->GetResource(TEXT("Money"));

	if (maxWood >= Wood && maxStone >= Stone && maxMoney >= Money) {
		rm->ChangeResource(TEXT("Wood"), -Wood);
		rm->ChangeResource(TEXT("Stone"), -Stone);
		rm->ChangeResource(TEXT("Money"), -Money);

		return true;
	}
	else {
		return false;
	}

	return true;
}

void ABuilding::UpkeepCost()
{
	Camera->ResourceManager->ChangeResource(TEXT("Money"), -Upkeep);
}

void ABuilding::Store(int32 Amount, ACitizen* Citizen)
{
	if (Storage < StorageCap) {
		Storage += Amount;

		Camera->ResourceManager->ChangeResource(Produce, Amount);

		Production(Citizen);
	}
	else {
		GetWorldTimerManager().ClearTimer(ProdTimer);

		FTimerHandle StoreCheckTimer;
		GetWorldTimerManager().SetTimer(StoreCheckTimer, FTimerDelegate::CreateUObject(this, &ABuilding::Store, Amount, Citizen), 30.0f, false);
	}
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		for (int i = 0; i < Occupied.Num(); i++) {
			if (c == Occupied[i] && c->Goal == this) {
				AtWork.Add(c);
			}
		}
		
		if (AtWork.Num() == 1) {
			if (c->Carrying > 0) {
				Store(c->Carrying, c);

				c->Carrying = 0;
			}
			else {
				Store(0, c);
			}
		}
	}
	else if (OtherActor->IsA<AVegetation>()) {
		AVegetation* r = Cast<AVegetation>(OtherActor);
		Camera->BuildComponent->HideTree(r);
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		if (AtWork.Contains(c)) {
			AtWork.Remove(c);
		}

		if (AtWork.Num() == 0) {
			GetWorldTimerManager().ClearTimer(ProdTimer);
		}
	}
	else if (OtherActor->IsA<AVegetation>()) {
		AVegetation* r = Cast<AVegetation>(OtherActor);
		Camera->BuildComponent->HideTree(r);
	}
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