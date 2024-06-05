#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "HealthComponent.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Player/ConstructionManager.h"
#include "Player/BuildComponent.h"
#include "Map/Vegetation.h"
#include "Map/Grid.h"
#include "Buildings/Builder.h"
#include "Buildings/Farm.h"

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

	ParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
	ParticleComponent->SetupAttachment(RootComponent);
	ParticleComponent->SetCastShadow(true);
	ParticleComponent->bAutoActivate = false;

	Emissiveness = 0.0f;

	Capacity = 2;

	Upkeep = 0;

	Storage = 0;
	StorageCap = 1000;

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
	material->SetScalarParameterValue("Emissiveness", Emissiveness);
	BuildingMesh->SetMaterial(0, material);
}

void ABuilding::Build()
{
	BuildingMesh->SetOverlayMaterial(DamagedMaterialOverlay);

	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	BuildingMesh->SetCanEverAffectNavigation(true);

	UResourceManager* rm = Camera->ResourceManagerComponent;
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

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

		cm->AddBuilding(this, EBuildStatus::Construction);
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
	if (Camera->BuildComponent->Building != this)
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
	if (Camera->BuildComponent->Building != this)
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

		if (HISM == Camera->Grid->HISMFlatGround)
			z = Camera->Grid->HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
		else
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
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

	cm->RemoveBuilding(this);

	for (ACitizen* citizen : GetOccupied()) {
		RemoveCitizen(citizen);

		citizen->AIController->Idle();
	}

	Destroy();
}

void ABuilding::OnBuilt()
{
	if (ActualMesh != nullptr) {
		BuildingMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		BuildingMesh->SetStaticMesh(ActualMesh);
	}

	UConstructionManager* cm = Camera->ConstructionManagerComponent;
	cm->RemoveBuilding(this);

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

	return true;
}

bool ABuilding::RemoveCitizen(ACitizen* Citizen)
{
	if (!Occupied.Contains(Citizen))
		return false;

	if (Citizen->Building.BuildingAt == this)
		Leave(Citizen);

	Occupied.Remove(Citizen);

	Citizen->AIController->DefaultAction();

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

	if (!IsA<AFarm>())
		Citizen->AIController->StopMovement(); 
	
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

	if (bHideCitizen) {
		Citizen->SetActorHiddenInGame(true);

		Citizen->SetActorLocation(GetActorLocation());
	}

	if (Citizen->Building.Employment->IsA<ABuilder>() && cm->IsBeingConstructed(this, nullptr)) {
		ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

		if (cm->IsRepairJob(this, builder))
			builder->Repair(Citizen, this);
		else {
			Citizen->SetActorHiddenInGame(true);

			Citizen->SetActorLocation(GetActorLocation());

			if (Citizen->Carrying.Amount > 0)
				builder->StoreMaterials(Citizen, this);

			builder->CheckCosts(Citizen, this);
		}
	}
	else {
		if (Citizen->Building.House == this || IsA<ABuilder>() || Citizen->Building.Employment == nullptr || !Citizen->Building.Employment->IsA<ABuilder>())
			return;

		ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

		builder->CarryResources(Citizen, this);
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

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* navData = nav->GetDefaultNavDataInstance();

	FNavLocation loc;
	nav->ProjectPointToNavigation(GetActorLocation(), loc, FVector(200.0f, 200.0f, 10.0f));

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
			double currentDist = FVector::Dist(loc.Location, BuildingMesh->GetSocketLocation("Entrance"));

			double newDist = FVector::Dist(location, BuildingMesh->GetSocketLocation("Entrance"));

			if (newDist >= currentDist)
				continue;

			loc.Location = location;
		}
	}

	Citizen->SetActorLocation(loc.Location);

	Citizen->SetActorHiddenInGame(false);
}

bool ABuilding::CheckInstant()
{
	return bInstantConstruction;
}

void ABuilding::AddCapacity()
{
	if (Capacity == MaxCapacity)
		return;

	Capacity++;

	FindCitizens();
}

void ABuilding::RemoveCapacity()
{
	if (Capacity == 0)
		return;

	Capacity--;

	if (GetOccupied().Num() <= GetCapacity())
		return;

	RemoveCitizen(GetOccupied().Last());
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