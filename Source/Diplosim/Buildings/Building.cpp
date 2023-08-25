#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Player/BuildComponent.h"
#include "Map/Vegetation.h"
#include "Map/Mineral.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCanEverAffectNavigation(false);
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetupAttachment(BuildingMesh);
	BoxCollision->bDynamicObstacle = true;

	Capacity = 2;

	Upkeep = 0;

	Storage = 0;
	StorageCap = 1000;

	BuildStatus = EBuildStatus::Blueprint;
	bMoved = false;

	bHideCitizen = true;

	bInstantConstruction = false;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	FVector size = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.5;
	BoxCollision->SetBoxExtent(size);

	BuildingMesh->OnComponentBeginOverlap.AddDynamic(this, &ABuilding::OnOverlapBegin);
	BuildingMesh->OnComponentEndOverlap.AddDynamic(this, &ABuilding::OnOverlapEnd);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

void ABuilding::Build()
{
	BuildStatus = EBuildStatus::Construction;

	UResourceManager* rm = Camera->ResourceManagerComponent;

	for (int32 i = 0; i < CostList.Num(); i++) {
		rm->TakeResource(CostList[i].Type, CostList[i].Cost);
	}

	if (CheckInstant()) {
		OnBuilt();
	} else {
		UStaticMesh* building = BuildingMesh->GetStaticMesh();
		FVector bSize = building->GetBounds().GetBox().GetSize();
		FVector cSize = ConstructionMesh->GetBounds().GetBox().GetSize();

		FVector size = bSize / cSize;
		if (size.Z < 1.0f)
			size.Z = 1.0f;

		BuildingMesh->SetRelativeScale3D(size);
		BuildingMesh->SetStaticMesh(ConstructionMesh);

		// Setup builders instead of raw timer
		FTimerHandle constructTimer;
		GetWorldTimerManager().SetTimer(constructTimer, FTimerDelegate::CreateUObject(this, &ABuilding::OnBuilt, building), 5.0f, false);
	}
}

bool ABuilding::CheckBuildCost()
{
	UResourceManager* rm = Camera->ResourceManagerComponent;

	for (int i = 0; i < CostList.Num(); i++) {
		int32 maxAmount = rm->GetResourceAmount(CostList[i].Type);

		if (maxAmount < CostList[i].Cost) {
			return false;
		}
	}

	return true;
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	bMoved = true;

	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		if (!AtWork.Contains(c) && c->Goal == this) {
			Enter(c);
		}
	}
	else if (BuildStatus == EBuildStatus::Blueprint) {
		if (OtherActor->IsA<AVegetation>() && !OtherActor->IsHidden()) {
			AVegetation* r = Cast<AVegetation>(OtherActor);
			Camera->BuildComponent->HideTree(r, true);

			TreeList.Add(OtherActor);
		}
		else if (OtherActor->IsA<AMineral>() || OtherActor->IsA<ABuilding>()) {
			Blocking.Add(OtherActor);
		}
		else if (OtherComp->GetName() == "HISMWater" || OtherComp->GetName() == "HISMHill") {
			Blocking.Add(OtherComp);
		}
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		if (AtWork.Contains(c)) {
			Leave(c);
		}
	}
	else if (BuildStatus == EBuildStatus::Blueprint) {
		if (OtherActor->IsA<AVegetation>() && TreeList.Contains(OtherActor)) {
			AVegetation* r = Cast<AVegetation>(OtherActor);
			Camera->BuildComponent->HideTree(r, false);
		}
		else if (Blocking.Contains(OtherActor)) {
			Blocking.RemoveSingle(OtherActor);
		}
		else if (Blocking.Contains(OtherComp)) {
			Blocking.RemoveSingle(OtherComp);
		}
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

TArray<FCostStruct> ABuilding::GetCosts()
{
	return CostList;
}

void ABuilding::OnBuilt(UStaticMesh* Building)
{
	if (Building != nullptr) {
		BuildingMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		BuildingMesh->SetStaticMesh(Building);
	}

	BuildStatus = EBuildStatus::Complete;

	GetWorldTimerManager().SetTimer(CostTimer, this, &ABuilding::UpkeepCost, 300.0f, true);

	FindCitizens();
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
	if (BuildStatus != EBuildStatus::Construction) {
		Citizen->SetActorHiddenInGame(bHideCitizen);

		if (Occupied.Contains(Citizen) && !AtWork.Contains(Citizen)) {
			AtWork.Add(Citizen);
		}
	}
	else {
		Citizen->SetActorHiddenInGame(true);
	}
}

void ABuilding::Leave(ACitizen* Citizen)
{
	Citizen->SetActorHiddenInGame(false);

	AtWork.Remove(Citizen);
}

bool ABuilding::CheckInstant()
{
	return bInstantConstruction;
}