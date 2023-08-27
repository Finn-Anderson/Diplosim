#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Player/BuildComponent.h"
#include "Map/Vegetation.h"
#include "Map/Mineral.h"
#include "Buildings/Builder.h"

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

	ActualMesh = nullptr;
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
		rm->TakeUniversalResource(CostList[i].Type, CostList[i].Cost);
	}

	if (CheckInstant()) {
		OnBuilt();
	} else {
		ActualMesh = BuildingMesh->GetStaticMesh();
		FVector bSize = ActualMesh->GetBounds().GetBox().GetSize();
		FVector cSize = ConstructionMesh->GetBounds().GetBox().GetSize();

		FVector size = bSize / cSize;
		if (size.Z < 1.0f)
			size.Z = 1.0f;

		BuildingMesh->SetRelativeScale3D(size);
		BuildingMesh->SetStaticMesh(ConstructionMesh);

		TArray<AActor*> foundBuilders;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilder::StaticClass(), foundBuilders);

		ABuilder* target = nullptr;
		for (int32 i = 0; i < foundBuilders.Num(); i++) {
			ABuilder* builder = Cast<ABuilder>(foundBuilders[i]);

			if (builder->Constructing != nullptr)
				continue;

			if (target == nullptr) {
				target = Cast<ABuilder>(foundBuilders[i]);
			}
			else {
				float dT = FVector::Dist(target->GetActorLocation(), GetActorLocation());

				float dB = FVector::Dist(foundBuilders[i]->GetActorLocation(), GetActorLocation());

				if (dT > dB) {
					target = builder;
				}
			}
		}

		if (target != nullptr) {
			target->Constructing = this;

			for (int32 i = 0; i < AtWork.Num(); i++) {
				AtWork[i]->MoveTo(target->Constructing);
			}
		}
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

void ABuilding::OnBuilt()
{
	if (ActualMesh != nullptr) {
		BuildingMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		BuildingMesh->SetStaticMesh(ActualMesh);
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
		else if(Citizen->Employment->IsA<ABuilder>()) {
			ABuilder* e = Cast<ABuilder>(Citizen->Employment);
			ABuilding* constructing = e->Constructing;

			int32 amount = 0;
			for (int32 i = 0; i < constructing->CostList.Num(); i++) {
				if (constructing->CostList[i].Type == Camera->ResourceManagerComponent->GetResource(this)) {
					amount = FMath::Clamp(constructing->CostList[i].Cost - constructing->CostList[i].Stored, 0, 10);

					break;
				}
			}

			AResource* r = Cast<AResource>(Camera->ResourceManagerComponent->GetResource(this)->GetDefaultObject());
			Citizen->Carry(r, amount);

			Camera->ResourceManagerComponent->TakeLocalResource(this, Citizen->Carrying.Amount);

			Citizen->MoveTo(constructing);
		}
	}
	else if (Citizen->Employment->IsA<ABuilder>()) {
		Citizen->SetActorHiddenInGame(true);

		AtWork.Add(Citizen);

		if (Citizen->Carrying.Amount > 0) {
			for (int32 i = 0; i < CostList.Num(); i++) {
				FCostStruct stock = CostList[i];

				if (stock.Type != Citizen->Carrying.Type)
					continue;

				stock.Stored += Citizen->Carrying.Amount;

				Citizen->Carry(nullptr, 0);

				break;
			}
		}

		bool construct = true;
		for (int32 i = 0; i < CostList.Num(); i++) {
			FCostStruct stock = CostList[i];

			if (stock.Stored == stock.Cost)
				continue;

			construct = false;

			CheckGatherSites(Citizen);
		}

		if (construct) {
			GetWorldTimerManager().SetTimer(ConstructTimer, this, &ABuilding::AddBuildPercentage, 0.1f, true);
		}
	}
}

void ABuilding::CheckGatherSites(class ACitizen* Citizen)
{
	for (int32 i = 0; i < CostList.Num(); i++) {
		FCostStruct stock = CostList[i];

		if (stock.Stored == stock.Cost)
			continue;

		TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManagerComponent->GetBuildings(stock.Type);

		ABuilding* target = nullptr;

		for (int32 j = 0; j < buildings.Num(); j++) {
			TArray<AActor*> foundBuildings;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

			for (int32 k = 0; k < foundBuildings.Num(); k++) {
				if (target->BuildStatus != EBuildStatus::Construction)
					continue;

				if (target == nullptr) {
					target = Cast<ABuilding>(foundBuildings[k]);
				}
				else {
					float dT = FVector::Dist(target->GetActorLocation(), GetActorLocation());

					float dB = FVector::Dist(foundBuildings[k]->GetActorLocation(), GetActorLocation());

					ABuilding* building = Cast<ABuilding>(foundBuildings[k]);

					if (dT > dB && building->Storage > target->Storage) {
						target = building;
					}
				}
			}
		}

		if (target != nullptr) {
			Citizen->MoveTo(target);

			return;
		}
		else {
			FTimerHandle checkSitesTimer;
			GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ABuilding::CheckGatherSites, Citizen), 30.0f, false);
		}
	}
}

void ABuilding::AddBuildPercentage()
{
	BuildPercentage += 1;

	if (BuildPercentage == 100) {
		OnBuilt();

		GetWorldTimerManager().ClearTimer(ConstructTimer);
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