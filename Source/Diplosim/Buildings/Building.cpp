#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "HealthComponent.h"
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
	BuildingMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCanEverAffectNavigation(false);
	BuildingMesh->bFillCollisionUnderneathForNavmesh = true;
	BuildingMesh->bCastDynamicShadow = true;
	BuildingMesh->CastShadow = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 200;
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

	RootComponent = BuildingMesh;
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

	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	BuildingMesh->SetCanEverAffectNavigation(true);

	UResourceManager* rm = Camera->ResourceManagerComponent;

	if (CheckInstant()) {
		OnBuilt();

		for (FCostStruct costStruct : GetCosts()) {
			rm->TakeUniversalResource(costStruct.Type, costStruct.Cost, 0);
		}
	} else {
		for (FCostStruct costStruct : GetCosts()) {
			rm->AddCommittedResource(costStruct.Type, costStruct.Cost);
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
		double magnitude = 1.0f;

		for (int32 i = 0; i < foundBuilders.Num(); i++) {
			ABuilder* builder = Cast<ABuilder>(foundBuilders[i]);

			if (builder->Constructing != nullptr || builder->BuildStatus != EBuildStatus::Complete || builder->GetOccupied().IsEmpty() || builder->GetOccupied()[0]->Building.BuildingAt != builder)
				continue;

			FClosestStruct closestStruct = builder->GetOccupied()[0]->AIController->GetClosestActor(builder, this);

			if (magnitude <= closestStruct.Magnitude)
				continue;

			magnitude = closestStruct.Magnitude;
			target = builder;
		}

		if (target != nullptr) {
			target->Constructing = this;

			target->GetOccupied()[0]->AIController->AIMoveTo(target->Constructing);
		}
	}
}

bool ABuilding::CheckBuildCost()
{
	UResourceManager* rm = Camera->ResourceManagerComponent;

	for (FCostStruct costStruct : GetCosts()) {
		int32 maxAmount = rm->GetResourceAmount(costStruct.Type);

		if (maxAmount < costStruct.Cost) {
			return false;
		}
	}

	return true;
}

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (BuildStatus == EBuildStatus::Blueprint) {
		if (OtherActor == this)
			return;

		bMoved = true;

		if (OtherActor->IsA<AVegetation>() && !OtherActor->IsHidden()) {
			OtherActor->SetActorHiddenInGame(true);

			TreeList.Add(OtherActor);
		}
		else {
			TArray<UObject*> objArr = { OtherComp, OtherActor };

			for (UObject* obj : objArr) {
				FVector location = CheckCollisions(obj, OtherBodyIndex);

				if (!location.IsZero()) {
					FBuildStruct buildStruct;

					buildStruct.Object = obj;

					if (obj->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
						buildStruct.Instance = OtherBodyIndex;
					}

					buildStruct.Location = location;

					Collisions.Add(buildStruct);
				}
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
		else {
			TArray<UObject*> objArr = { OtherComp, OtherActor };

			for (UObject* obj : objArr) {
				FVector location = CheckCollisions(obj, OtherBodyIndex);

				if (!location.IsZero()) {
					FBuildStruct buildStruct;

					buildStruct.Object = obj;

					if (obj->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
						buildStruct.Instance = OtherBodyIndex;
					}

					buildStruct.Location = location;

					Collisions.RemoveSingle(buildStruct);
				}
			}
		}
	}
}

FVector ABuilding::CheckCollisions(class UObject* Object, int32 Index)
{
	FVector location;

	FTransform transform;
	transform.SetLocation(FVector(0.0f, 0.0f, 0.0f));

	double z = 0.0f;

	if (Object->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
		UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(Object);

		HISM->GetInstanceTransform(Index, transform);
		z = HISM->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	}
	else if (Object->IsA<AResource>() || (Object->IsA<ABuilding>())) {
		UStaticMeshComponent* MeshComp = Cast<AActor>(Object)->GetComponentByClass<UStaticMeshComponent>();

		transform = MeshComp->GetComponentTransform();
		z = MeshComp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	}

	location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

	return location;
}

void ABuilding::DestroyBuilding()
{
	for (ACitizen* citizen : GetOccupied()) {
		if (Cast<ABuilding>(citizen->Building.House) == this) {
			citizen->Building.House = nullptr;
		}
		else {
			citizen->Building.Employment = nullptr;
		}
	}

	Destroy();
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
	Citizen->Building.BuildingAt = this;
	Citizen->Building.EnterLocation = Citizen->GetActorLocation();

	if (bHideCitizen || BuildStatus == EBuildStatus::Construction)
		Citizen->SetActorLocation(GetActorLocation());

	if (BuildStatus != EBuildStatus::Construction) {
		Citizen->SetActorHiddenInGame(bHideCitizen);

		Citizen->AIController->StopMovement();

		if (GetOccupied().Contains(Citizen) || Citizen->Building.Employment == nullptr || !Citizen->Building.Employment->IsA<ABuilder>())
			return;

		ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);
			
		if (builder->Constructing == nullptr)
			return;

		builder->CarryResources(Citizen, this);
	}
	else if (Citizen->Building.Employment->IsA<ABuilder>()) {
		Citizen->SetActorHiddenInGame(true);

		if (Citizen->Carrying.Amount > 0) {
			for (int32 i = 0; i < GetCosts().Num(); i++) {
				FCostStruct costStruct = CostList[i];

				if (costStruct.Type->GetDefaultObject<AResource>() != Citizen->Carrying.Type)
					continue;

				costStruct.Stored += Citizen->Carrying.Amount;

				CostList[i] = costStruct;

				Citizen->Carry(nullptr, 0, nullptr);

				break;
			}
		}

		bool construct = true;

		for (FCostStruct costStruct : GetCosts()) {
			if (costStruct.Stored == costStruct.Cost)
				continue;

			construct = false;

			CheckGatherSites(Citizen, costStruct);

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

			if (building->BuildStatus != EBuildStatus::Complete || building->Storage < 1)
				continue;

			int32 storage = 0;

			if (target != nullptr) {
				storage = target->Storage;
			}

			FClosestStruct closestStruct = Citizen->AIController->GetClosestActor(target, building, storage, building->Storage);

			target = Cast<ABuilding>(closestStruct.Actor);
		}
	}

	if (target != nullptr) {
		Citizen->AIController->AIMoveTo(target);

		return;
	}
	else {
		ABuilder* e = Cast<ABuilder>(Citizen->Building.Employment);

		e->CheckConstruction(Citizen);

		if (e->Constructing != this)
			return;

		FTimerHandle checkSitesTimer;
		GetWorldTimerManager().SetTimer(checkSitesTimer, FTimerDelegate::CreateUObject(this, &ABuilding::CheckGatherSites, Citizen, Stock), 30.0f, false);
	}
}

void ABuilding::AddBuildPercentage(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt != this)
		return;

	BuildPercentage += 1;

	if (BuildPercentage == 100) {
		OnBuilt();

		ABuilder* e = Cast<ABuilder>(Citizen->Building.Employment);
		e->Constructing = nullptr;

		Citizen->AIController->AIMoveTo(Citizen->Building.Employment);

		GetWorldTimerManager().ClearTimer(ConstructTimer);
	}
}

void ABuilding::Leave(ACitizen* Citizen)
{
	if (Citizen->IsHidden()) {
		FHitResult Hit;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FVector loc = Citizen->Building.EnterLocation;

		while (true) {
			if (GetWorld()->LineTraceSingleByChannel(Hit, loc, Citizen->Building.EnterLocation + FVector(0.0f, 0.0f, 100.0f), ECollisionChannel::ECC_Visibility, QueryParams)) {
				loc += GetActorForwardVector() * 20.0f;
			}
			else {
				break;
			}
		}

		Citizen->SetActorLocation(loc);

		Citizen->SetActorHiddenInGame(false);
	}

	Citizen->Building.BuildingAt = nullptr;
}

bool ABuilding::CheckInstant()
{
	return bInstantConstruction;
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