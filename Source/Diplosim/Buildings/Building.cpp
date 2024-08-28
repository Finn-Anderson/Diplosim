#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NavigationSystem.h"
#include "Components/Widget.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Components/BuildComponent.h"
#include "Map/Resources/Vegetation.h"
#include "Map/Grid.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Trader.h"
#include "Buildings/House.h"
#include "Buildings/Work/Production/Farm.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Universal/EggBasket.h"

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
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BuildingMesh->SetCanEverAffectNavigation(false);
	BuildingMesh->bFillCollisionUnderneathForNavmesh = true;

	RootComponent = BuildingMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
	ParticleComponent->SetupAttachment(RootComponent, "ParticleSocket");
	ParticleComponent->SetUseAutoManageAttachment(true);
	ParticleComponent->SetCastShadow(true);
	ParticleComponent->AutoAttachSocketName = "ParticleSocket";
	ParticleComponent->bAutoActivate = false;

	Emissiveness = 0.0f;

	Capacity = 2;
	MaxCapacity = 2;

	Storage = 0;
	StorageCap = 1000;

	UpkeepTimer = 0;

	bHideCitizen = true;

	bInstantConstruction = false;

	ActualMesh = nullptr;

	bConstant = true;

	bOffset = false;
	bIgnoreCollisions = false;
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

	BuildingMesh->SetCanEverAffectNavigation(true);

	BuildingMesh->bReceivesDecals = true;

	UResourceManager* rm = Camera->ResourceManager;
	UConstructionManager* cm = Camera->ConstructionManager;

	if (CheckInstant()) {
		OnBuilt();

		for (FItemStruct items : CostList)
			rm->TakeUniversalResource(items.Resource, items.Amount, 0);
	} else {
		for (FItemStruct items : CostList)
			rm->AddCommittedResource(items.Resource, items.Amount);

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

void ABuilding::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == this || OtherComp->IsA<USphereComponent>())
		return;

	if (OtherActor->IsA<AVegetation>()) {
		FTreeStruct treeStruct;
		treeStruct.Resource = Cast<AVegetation>(OtherActor);
		treeStruct.Instance = OtherBodyIndex;

		treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 0.0f);

		treeStruct.Resource->ResourceHISM->MarkRenderStateDirty();

		TreeList.Add(treeStruct);
	}
	else if (OtherActor->IsA<AResource>() || OtherActor->IsA<ABuilding>() || OtherActor->IsA<AGrid>() || OtherActor->IsA<AEggBasket>()) {
		FCollisionStruct collision;
		collision.Actor = OtherActor;

		if (OtherComp->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
			collision.HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(OtherComp);
			collision.Instance = OtherBodyIndex;
		}
		
		Collisions.Add(collision);
	}
}

void ABuilding::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
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
		FCollisionStruct collision;
		collision.Actor = OtherActor;

		if (OtherComp->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
			collision.HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(OtherComp);
			collision.Instance = OtherBodyIndex;
		}

		Collisions.Remove(collision);
	}
}

void ABuilding::DestroyBuilding()
{
	UResourceManager* rm = Camera->ResourceManager;
	UConstructionManager* cm = Camera->ConstructionManager;

	if (cm->IsBeingConstructed(this, nullptr)) {
		for (FItemStruct items : CostList) {
			rm->AddUniversalResource(items.Resource, items.Stored);

			rm->TakeCommittedResource(items.Resource, items.Amount - items.Stored);
		}
	}

	rm->TakeLocalResource(this, Capacity);

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

	UConstructionManager* cm = Camera->ConstructionManager;
	cm->RemoveBuilding(this);

	Camera->CitizenManager->Buildings.Add(this);

	if (bConstant)
		ParticleComponent->Activate();
}

void ABuilding::UpkeepCost()
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
	Citizen->Building.EnterLocation = Citizen->GetActorLocation();

	if (!IsA<AFarm>())
		Citizen->AIController->StopMovement();
	
	UConstructionManager* cm = Camera->ConstructionManager;

	if (bHideCitizen) {
		Citizen->SetActorHiddenInGame(true);

		Citizen->SetActorLocation(GetActorLocation());
	}

	if (Citizen->Carrying.Amount > 0)
		StoreResource(Citizen);

	if (Citizen->Building.Employment->IsA<ABuilder>() && cm->IsBeingConstructed(this, nullptr)) {
		ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

		if (cm->IsRepairJob(this, builder))
			builder->Repair(Citizen, this);
		else {
			Citizen->SetActorHiddenInGame(true);

			Citizen->SetActorLocation(GetActorLocation());

			builder->CheckCosts(Citizen, this);
		}
	}
	else if (!IsA<AHouse>() && Citizen->Building.Employment != this) {
		TArray<FItemStruct> items;
		ABuilding* deliverTo = nullptr;

		if (Citizen->Building.Employment->IsA<ABuilder>()) {
			ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

			deliverTo = cm->GetBuilding(builder);
			items = deliverTo->CostList;
		}
		else if (Citizen->Building.Employment->IsA<ATrader>()) {
			ATrader* trader = Cast<ATrader>(Citizen->Building.Employment);

			deliverTo = trader;

			if (!trader->Orders[0].bCancelled)
				items = trader->Orders[0].SellingItems;
		}

		if (items.IsEmpty())
			Citizen->AIController->AIMoveTo(deliverTo);
		else
			CarryResources(Citizen, deliverTo, items);
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

	FVector location = Citizen->Building.EnterLocation;

	if (BuildingMesh->DoesSocketExist("Entrance")) {
			FVector pos = BuildingMesh->GetSocketLocation("Entrance");

			if (GetWorld()->LineTraceSingleByChannel(hit, pos, pos - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_WorldStatic, QueryParams))
				if (hit.GetActor()->IsA<AGrid>() && hit.GetComponent() == Cast<AGrid>(hit.GetActor())->HISMGround)
					location = pos;
	}

	Citizen->SetActorLocation(location);

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

bool ABuilding::CheckStored(ACitizen* Citizen, TArray<FItemStruct> Items)
{
	for (FItemStruct item : Items) {
		if (item.Stored == item.Amount)
			continue;

		Citizen->AIController->GetGatherSite(Camera, item.Resource);

		return false;
	}

	return true;
}

void ABuilding::CarryResources(ACitizen* Citizen, ABuilding* DeliverTo, TArray<FItemStruct> Items)
{
	int32 amount = 0;
	int32 capacity = 10;

	TSubclassOf<AResource> resource = Camera->ResourceManager->GetResource(this);

	if (capacity > Storage)
		capacity = Storage;

	for (FItemStruct item : Items) {
		if (resource == item.Resource) {
			amount = FMath::Clamp(item.Amount - item.Stored, 0, capacity);

			break;
		}
	}

	Camera->ResourceManager->TakeLocalResource(this, amount);

	Camera->ResourceManager->TakeCommittedResource(resource, amount);

	Citizen->Carry(Cast<AResource>(resource->GetDefaultObject()), amount, DeliverTo);
}

void ABuilding::StoreResource(ACitizen* Citizen)
{
	if (Citizen->Carrying.Type == nullptr)
		return;

	TSubclassOf<AResource> resource = Camera->ResourceManager->GetResource(this);

	if (resource != nullptr && Citizen->Carrying.Type->IsA(resource)) {
		bool canStore = Camera->ResourceManager->AddLocalResource(this, Citizen->Carrying.Amount);

		if (canStore) {
			Citizen->Carry(nullptr, 0, nullptr);

			AWork* work = Cast<AWork>(this);

			work->Production(Citizen);
		}
		else {
			FTimerHandle StoreCheckTimer;
			GetWorldTimerManager().SetTimer(StoreCheckTimer, FTimerDelegate::CreateUObject(this, &ABuilding::StoreResource, Citizen), 30.0f, false);
		}
	}
	else {
		UConstructionManager* cm = Camera->ConstructionManager;

		TArray<FItemStruct> items;

		if (cm->IsBeingConstructed(this, nullptr))
			items = CostList;
		else if (IsA<ATrader>())
			items = Cast<ATrader>(this)->Orders[0].SellingItems;
		else
			items = Cast<AInternalProduction>(this)->Intake;

		for (int32 i = 0; i < items.Num(); i++) {
			if (!Citizen->Carrying.Type->IsA(items[i].Resource))
				continue;

			if (cm->IsBeingConstructed(this, nullptr))
				CostList[i].Stored += Citizen->Carrying.Amount;
			else if (IsA<ATrader>())
				Cast<ATrader>(this)->Orders[0].SellingItems[i].Stored += Citizen->Carrying.Amount;
			else
				Cast<AInternalProduction>(this)->Intake[i].Stored += Citizen->Carrying.Amount;

			Citizen->Carry(nullptr, 0, nullptr);

			break;
		}
	}
}