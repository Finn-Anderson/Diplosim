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

	EcoStatus = Poor;

	Blueprint = true;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	BuildingMesh->OnComponentBeginOverlap.AddDynamic(this, &ABuilding::OnOverlapBegin);
	BuildingMesh->OnComponentEndOverlap.AddDynamic(this, &ABuilding::OnOverlapEnd);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

void ABuilding::Run()
{
	Blueprint = false;

	GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::FindCitizens, 30.0f, true, 2.0f);
	GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::UpkeepCost, 300.0f, true);
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
}

void ABuilding::UpkeepCost()
{
	Camera->ResourceManager->ChangeResource(TEXT("Money"), -Upkeep);
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		Action(c);
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

		Action(c);
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

void ABuilding::AddCitizen(ACitizen* Citizen)
{
	if (Occupied.Num() != GetCapacity()) {
		Occupied.Add(Citizen);

		Citizen->Employment = this;
		Citizen->LookForHouse();

		Action(Citizen);
	}
	else {
		GetWorldTimerManager().ClearTimer(FindTimer);
	}
}

void ABuilding::RemoveCitizen(ACitizen* Citizen)
{
	if (Occupied.Contains(Citizen)) {
		Occupied.Remove(Citizen);

		if (Occupied.Num() == 0) {
			Action(Citizen);
		}

		Citizen->Employment = nullptr;

		GetWorldTimerManager().SetTimer(FindTimer, this, &ABuilding::FindCitizens, 30.0f, true);
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

void ABuilding::Action(ACitizen* Citizen)
{

}