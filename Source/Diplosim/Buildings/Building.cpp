#include "Building.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Player/BuildComponent.h"
#include "Map/Vegetation.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	Capacity = 2;

	Resource1 = nullptr;
	Resource2 = nullptr;

	R1Cost = 0;
	R2Cost = 0;
	MoneyCost = 0;

	Upkeep = 0;

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

	OnBuilt();
}

bool ABuilding::BuildCost()
{
	UResourceManager* rm = Camera->ResourceManager;

	int32 maxR1 = 0;
	int32 maxR2 = 0;

	if (Resource1 != nullptr) {
		maxR1 = rm->GetResource(Resource1);
	}

	if (Resource2 != nullptr) {
		maxR2 = rm->GetResource(Resource2);
	}

	int32 maxMoney = rm->GetResource(Money);

	if (maxR1 >= R1Cost && maxR2 >= R2Cost && maxMoney >= MoneyCost) {
		if (Resource1 != nullptr) {
			rm->ChangeResource(Resource1, -R1Cost);
		}

		if (Resource2 != nullptr) {
			rm->ChangeResource(Resource2, -R1Cost);
		}

		rm->ChangeResource(Money, -MoneyCost);

		return true;
	}
	else {
		return false;
	}
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		Enter(c);
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

		Leave(c);
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

			if (Cast<ABuilding>(c->House) == this) {
				c->House = nullptr;
			}
			else {
				c->Employment = nullptr;
			}
		}
	}

	Destroy();
}

int32 ABuilding::GetCapacity()
{
	return Capacity;
}

TArray<class ACitizen*> ABuilding::GetOccupied()
{
	return Occupied;
}

FText ABuilding::GetR1()
{
	FString s = "";
	if (Resource1 != nullptr) {
		s = Resource1->GetName() + ": " + FString::FromInt(R1Cost);
	}

	return FText::FromString(s);
}

FText ABuilding::GetR2()
{
	FString s = "";
	if (Resource2 != nullptr) {
		s = Resource2->GetName() + ": " + FString::FromInt(R2Cost);
	}

	return FText::FromString(s);
}

FText ABuilding::GetMoney()
{
	FString s = "Money: " + FString::FromInt(MoneyCost);

	return FText::FromString(s);
}

void ABuilding::OnBuilt()
{

}

void ABuilding::UpkeepCost()
{

}

void ABuilding::FindCitizens()
{

}

void ABuilding::AddCitizen(ACitizen* Citizen)
{

}

void ABuilding::RemoveCitizen(ACitizen* Citizen)
{

}

void ABuilding::Enter(ACitizen* Citizen)
{

}

void ABuilding::Leave(ACitizen* Citizen)
{

}