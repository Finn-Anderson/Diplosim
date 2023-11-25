#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/HealthComponent.h"
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
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCanEverAffectNavigation(true);
	BuildingMesh->bFillCollisionUnderneathForNavmesh = true;
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 500;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Capacity = 2;

	Upkeep = 0;

	Storage = 0;
	StorageCap = 1000;

	BuildStatus = EBuildStatus::Blueprint;

	bHideCitizen = true;

	bInstantConstruction = false;

	ActualMesh = nullptr;

	bMoved = false;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	FVector size = (BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2);

	BuildingMesh->OnComponentBeginOverlap.AddDynamic(this, &ABuilding::OnOverlapBegin);
	BuildingMesh->OnComponentEndOverlap.AddDynamic(this, &ABuilding::OnOverlapEnd);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

void ABuilding::Build()
{
	BuildStatus = EBuildStatus::Construction;

	if (CheckInstant()) {
		OnBuilt();
	} else {
		UResourceManager* rm = Camera->ResourceManagerComponent;

		for (int i = 0; i < CostList.Num(); i++) {
			rm->AddCommittedResource(CostList[i].Type, CostList[i].Cost);
		}

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

			if (builder->Constructing != nullptr || builder->BuildStatus != EBuildStatus::Complete)
				continue;

			FVector loc = builder->Occupied[0]->CanMoveTo(this);

			if (loc.IsZero())
				continue;

			target = Cast<ABuilder>(builder->Occupied[0]->GetClosestActor(this, target, builder));
		}

		if (target != nullptr) {
			target->Constructing = this;

			target->Occupied[0]->MoveTo(target->Constructing);
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
	if (BuildStatus == EBuildStatus::Blueprint) {
		bMoved = true;

		if (OtherActor->IsA<AVegetation>() && !OtherActor->IsHidden()) {
			OtherActor->SetActorHiddenInGame(true);

			TreeList.Add(OtherActor);
		}
		else if (OtherActor->IsA<AMineral>() || (OtherActor->IsA<ABuilding>() && OtherActor != this)) {
			Blocking.Add(OtherActor);
		}
		else if (OtherComp->GetName() == "HISMWater") {
			Blocking.Add(OtherComp);
		} else if (OtherComp->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
			FBuildStruct item;
			item.HISMComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(OtherComp);
			item.Instance = OtherBodyIndex;

			FTransform transform;
			item.HISMComponent->GetInstanceTransform(item.Instance, transform);
			int32 z = item.HISMComponent->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 50.0f;

			item.Location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

			if (!AllowedComps.Contains(item)) {
				AllowedComps.Add(item);
			}
		}
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (BuildStatus == EBuildStatus::Blueprint) {
		if (TreeList.Contains(OtherActor)) {
			OtherActor->SetActorHiddenInGame(false);

			TreeList.Remove(OtherActor);
		}
		else if (Blocking.Contains(OtherActor)) {
			Blocking.RemoveSingle(OtherActor);
		}
		else if (Blocking.Contains(OtherComp)) {
			Blocking.RemoveSingle(OtherComp);
		} else if (OtherComp->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
			FBuildStruct item;
			item.HISMComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(OtherComp);
			item.Instance = OtherBodyIndex;

			if (AllowedComps.Contains(item)) {
				AllowedComps.Remove(item);
			}
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

	GetWorldTimerManager().SetTimer(CostTimer, FTimerDelegate::CreateUObject(this, &ABuilding::UpkeepCost, 0), 300.0f, true);

	FindCitizens();
}

void ABuilding::UpkeepCost(int32 Cost)
{
	Camera->ResourceManagerComponent->TakeUniversalResource(Money, Cost, -1000000000);
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
		else if(Citizen->Employment != nullptr && Citizen->Employment->IsA<ABuilder>()) {
			ABuilder* e = Cast<ABuilder>(Citizen->Employment);
			
			e->CarryResources(Citizen, this);
		}
	}
	else if (Citizen->Employment->IsA<ABuilder>()) {
		Citizen->SetActorHiddenInGame(true);

		AtWork.Add(Citizen);

		if (Citizen->Carrying.Amount > 0) {
			for (int32 i = 0; i < CostList.Num(); i++) {
				if (CostList[i].Type->GetDefaultObject<AResource>() != Citizen->Carrying.Type)
					continue;

				CostList[i].Stored += Citizen->Carrying.Amount;

				Citizen->Carry(nullptr, 0, nullptr);

				break;
			}
		}

		bool construct = true;
		for (int32 i = 0; i < CostList.Num(); i++) {
			if (CostList[i].Stored == CostList[i].Cost)
				continue;

			construct = false;

			CheckGatherSites(Citizen, CostList[i]);

			break;
		}

		if (construct) {
			GetWorldTimerManager().SetTimer(ConstructTimer, FTimerDelegate::CreateUObject(this, &ABuilding::AddBuildPercentage, Citizen), 0.1f, true);
		}
	}
}

void ABuilding::CheckGatherSites(ACitizen* Citizen, FCostStruct Stock)
{
	TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManagerComponent->GetBuildings(Stock.Type);

	ABuilding* target = nullptr;

	for (int32 j = 0; j < buildings.Num(); j++) {
		TArray<AActor*> foundBuildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

		for (int32 k = 0; k < foundBuildings.Num(); k++) {
			ABuilding* building = Cast<ABuilding>(foundBuildings[k]); 

			FVector loc = Citizen->CanMoveTo(building);

			if (building->BuildStatus != EBuildStatus::Complete || loc.IsZero() || building->Storage < 1)
				continue;

			int32 storage = 1;

			if (target != nullptr) {
				storage = target->Storage;
			}

			target = Cast<ABuilding>(Citizen->GetClosestActor(Citizen, target, building, storage, building->Storage));
		}
	}

	if (target != nullptr) {
		Citizen->MoveTo(target);

		return;
	}
	else {
		ABuilder* e = Cast<ABuilder>(Citizen->Employment);

		e->CheckConstruction(Citizen);

		if (e->Constructing != this)
			return;

		FTimerHandle checkSitesTimer;
		GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ABuilding::CheckGatherSites, Citizen, Stock), 30.0f, false);
	}
}

void ABuilding::AddBuildPercentage(ACitizen* Citizen)
{
	if (AtWork.Contains(Citizen)) {
		BuildPercentage += 1;

		if (BuildPercentage == 100) {
			OnBuilt();

			ABuilder* e = Cast<ABuilder>(Citizen->Employment);
			e->Constructing = nullptr;

			Citizen->MoveTo(Citizen->Employment);

			GetWorldTimerManager().ClearTimer(ConstructTimer);
		}
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