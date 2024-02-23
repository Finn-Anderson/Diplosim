#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "HealthComponent.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Map/Vegetation.h"
#include "Map/Grid.h"
#include "Buildings/Builder.h"
#include "Buildings/Farm.h"
#include "InteractableInterface.h"

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

	RootComponent = BuildingMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));

	ParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
	ParticleComponent->SetupAttachment(RootComponent);
	ParticleComponent->SetCastShadow(true);
	ParticleComponent->bAutoActivate = false;

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

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(BuildingMesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	BuildingMesh->SetMaterial(0, material);
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

		for (int32 i = 0; i < foundBuilders.Num(); i++) {
			ABuilder* builder = Cast<ABuilder>(foundBuilders[i]);

			if (builder->Constructing != nullptr || builder->BuildStatus != EBuildStatus::Complete || builder->GetOccupied().IsEmpty() || builder->GetOccupied()[0]->Building.BuildingAt != builder)
				continue;

			if (target == nullptr) {
				target = builder;

				continue;
			}

			double magnitude = builder->GetOccupied()[0]->AIController->GetClosestActor(GetActorLocation(), target->GetActorLocation(), builder->GetActorLocation());

			if (magnitude <= 0.0f)
				continue;

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
	if (BuildStatus != EBuildStatus::Blueprint)
		return;

	bMoved = true;

	if (OtherActor->IsA<AVegetation>()) {
		FTreeStruct treeStruct;
		treeStruct.Resource = Cast<AVegetation>(OtherActor);
		treeStruct.Instance = OtherBodyIndex;

		treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 0.0f);

		treeStruct.Resource->ResourceHISM->MarkRenderStateDirty();

		TreeList.Add(treeStruct);
	}
	else {
		TArray<UObject*> objArr = { OtherComp, OtherActor };

		for (UObject* obj : objArr) {
			FVector location = CheckCollisions(obj, OtherBodyIndex);

			if (!location.IsZero()) {
				FBuildStruct buildStruct;

				buildStruct.Object = obj;

				if (obj->IsA<UHierarchicalInstancedStaticMeshComponent>())
					buildStruct.Instance = OtherBodyIndex;

				buildStruct.Location = location;

				Collisions.Add(buildStruct);
			}
		}
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (BuildStatus != EBuildStatus::Blueprint)
		return;

	FTreeStruct treeStruct;

	if (OtherActor->IsA<AVegetation>()) {
		treeStruct.Resource = Cast<AVegetation>(OtherActor);
		treeStruct.Instance = OtherBodyIndex;
	}

	if (TreeList.Contains(treeStruct)) {
		treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 1.0f);

		treeStruct.Resource->ResourceHISM->MarkRenderStateDirty();

		TreeList.Remove(treeStruct);
	}
	else {
		TArray<UObject*> objArr = { OtherComp, OtherActor };

		for (UObject* obj : objArr) {
			FVector location = CheckCollisions(obj, OtherBodyIndex);

			if (!location.IsZero()) {
				FBuildStruct buildStruct;

				buildStruct.Object = obj;

				if (obj->IsA<UHierarchicalInstancedStaticMeshComponent>())
					buildStruct.Instance = OtherBodyIndex;

				buildStruct.Location = location;

				Collisions.RemoveSingle(buildStruct);
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
	if (BuildStatus == EBuildStatus::Construction) {
		TArray<AActor*> builders;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilder::StaticClass(), builders);

		for (AActor* actor : builders) {
			ABuilder* builder = Cast<ABuilder>(actor);

			if (builder->Constructing != this)
				continue;

			builder->Constructing = nullptr;

			if (!builder->GetOccupied().IsEmpty())
				builder->GetOccupied()[0]->AIController->AIMoveTo(builder);

			break;
		}
	}
	else {
		for (ACitizen* citizen : GetOccupied()) {
			RemoveCitizen(citizen);

			citizen->AIController->Idle();
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

	ParticleComponent->Activate();
}

void ABuilding::UpkeepCost(int32 Cost)
{
	Camera->ResourceManagerComponent->TakeUniversalResource(Money, Cost, -1000000000);
}

void ABuilding::FindCitizens()
{

}

bool ABuilding::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() <= Occupied.Num())
		return false;

	Occupied.Add(Citizen);

	InteractableComponent->SetOccupied();
	InteractableComponent->ExecuteEditEvent("Occupied");

	return true;
}

bool ABuilding::RemoveCitizen(ACitizen* Citizen)
{
	if (!Occupied.Contains(Citizen))
		return false;

	Leave(Citizen);

	Occupied.Remove(Citizen);

	InteractableComponent->SetOccupied();
	InteractableComponent->ExecuteEditEvent("Occupied");

	return true;
}

TArray<ACitizen*> ABuilding::GetCitizensAtBuilding()
{
	TArray<ACitizen*> citizens;

	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Building.BuildingAt != this)
			continue;

		citizens.Add(citizen);
	}

	return citizens;
}

void ABuilding::Enter(ACitizen* Citizen)
{
	Citizen->Building.BuildingAt = this;
	Citizen->Building.EnterLocation = Citizen->GetActorLocation();

	if (!IsA<AFarm>())
		Citizen->AIController->StopMovement();

	if (bHideCitizen || BuildStatus == EBuildStatus::Construction)
		Citizen->SetActorLocation(GetActorLocation());

	if (BuildStatus != EBuildStatus::Construction) {
		Citizen->SetActorHiddenInGame(bHideCitizen);

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

			if (target == nullptr) {
				target = building;

				continue;
			}

			int32 storage = 0;

			if (target != nullptr)
				storage = target->Storage;

			double magnitude = Citizen->AIController->GetClosestActor(Citizen->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation(), storage, building->Storage);

			if (magnitude <= 0.0f)
				continue;

			target = building;
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
	Citizen->Building.BuildingAt = nullptr;

	if (!Citizen->IsHidden())
		return;

	FHitResult hit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FVector> possibleLocations;

	FVector loc = Citizen->Building.EnterLocation;

	if (BuildingMesh->DoesSocketExist("Entrance")) {
		for (int32 i = -5; i < 5; i++) {
			FVector pos = BuildingMesh->GetSocketLocation("Entrance") + GetActorForwardVector() * 20.0f * i;

			if (GetWorld()->LineTraceSingleByChannel(hit, pos, pos - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_WorldStatic, QueryParams)) {
				if (!hit.GetActor()->IsA<AGrid>() || hit.GetComponent() != Cast<AGrid>(hit.GetActor())->HISMGround)
					continue;

				possibleLocations.Add(pos);
			}
		}

		for (FVector location : possibleLocations) {
			double currentDist = FVector::Dist(loc, BuildingMesh->GetSocketLocation("Entrance"));

			double newDist = FVector::Dist(location, BuildingMesh->GetSocketLocation("Entrance"));

			if (newDist >= currentDist && loc != Citizen->Building.EnterLocation)
				continue;

			loc = location;
		}
	}

	Citizen->SetActorLocation(loc);

	Citizen->SetActorHiddenInGame(false);
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