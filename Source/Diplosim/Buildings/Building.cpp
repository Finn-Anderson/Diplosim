#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NavigationSystem.h"
#include "Components/Widget.h"

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
#include "Buildings/Trader.h"
#include "Buildings/House.h"
#include "EggBasket.h"

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

	UResourceManager* rm = Camera->ResourceManagerComponent;
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

	if (CheckInstant()) {
		OnBuilt();

		for (FItemStruct items : CostList.Items)
			rm->TakeUniversalResource(items.Resource, items.Amount, 0);
	} else {
		for (FItemStruct items : CostList.Items)
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
	UResourceManager* rm = Camera->ResourceManagerComponent;
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

	if (cm->IsBeingConstructed(this, nullptr)) {
		for (FItemStruct items : CostList.Items) {
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

	UConstructionManager* cm = Camera->ConstructionManagerComponent;
	cm->RemoveBuilding(this);

	GetWorldTimerManager().SetTimer(CostTimer, this, &ABuilding::UpkeepCost, 300.0f, true);

	FindCitizens();

	if (bConstant)
		ParticleComponent->Activate();
}

void ABuilding::UpkeepCost()
{

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
	Citizen->Building.EnterLocation = Citizen->GetActorLocation();

	Citizen->AIController->StopMovement();
	
	UConstructionManager* cm = Camera->ConstructionManagerComponent;

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
			items = deliverTo->CostList.Items;
		}
		else if (Citizen->Building.Employment->IsA<ATrader>()) {
			ATrader* trader = Cast<ATrader>(Citizen->Building.Employment);

			deliverTo = trader;

			if (!trader->Orders[0].bCancelled)
				items = trader->Orders[0].Items;
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

	TSubclassOf<AResource> resource = Camera->ResourceManagerComponent->GetResource(this);

	if (capacity > Storage)
		capacity = Storage;

	for (FItemStruct item : Items) {
		if (resource == item.Resource) {
			amount = FMath::Clamp(item.Amount - item.Stored, 0, capacity);

			break;
		}
	}

	Camera->ResourceManagerComponent->TakeLocalResource(this, amount);

	Camera->ResourceManagerComponent->TakeCommittedResource(resource, amount);

	Citizen->Carry(Cast<AResource>(resource->GetDefaultObject()), amount, DeliverTo);
}

void ABuilding::StoreResource(ACitizen* Citizen)
{
	if (Citizen->Carrying.Type == nullptr)
		return;

	TSubclassOf<AResource> resource = Camera->ResourceManagerComponent->GetResource(this);

	if (resource != nullptr && resource->GetDefaultObject() == Citizen->Carrying.Type) {
		bool canStore = Camera->ResourceManagerComponent->AddLocalResource(this, Citizen->Carrying.Amount);

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
		UConstructionManager* cm = Camera->ConstructionManagerComponent;

		TArray<FItemStruct> items;

		if (cm->IsBeingConstructed(this, nullptr))
			items = CostList.Items;
		else
			items = Orders[0].Items;

		for (int32 i = 0; i < items.Num(); i++) {
			if (items[i].Resource->GetDefaultObject<AResource>() != Citizen->Carrying.Type)
				continue;

			if (cm->IsBeingConstructed(this, nullptr))
				CostList.Items[i].Stored += Citizen->Carrying.Amount;
			else
				Orders[0].Items[i].Stored += Citizen->Carrying.Amount;

			Citizen->Carry(nullptr, 0, nullptr);

			break;
		}
	}
}

void ABuilding::ReturnResource(class ACitizen* Citizen)
{
	for (int32 i = 0; i < Orders[0].Items.Num(); i++) {
		if (Orders[0].Items[i].Stored == 0)
			continue;

		TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManagerComponent->GetBuildings(Orders[0].Items[i].Resource);

		ABuilding* target = nullptr;

		int32 amount = FMath::Clamp(Orders[0].Items[i].Stored, 0, 10);

		for (int32 j = 0; j < buildings.Num(); j++) {
			TArray<AActor*> foundBuildings;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

			for (int32 k = 0; k < foundBuildings.Num(); k++) {
				ABuilding* building = Cast<ABuilding>(foundBuildings[k]);

				if (building->Storage + amount > building->StorageCap)
					continue;

				if (target == nullptr) {
					target = building;

					continue;
				}

				double magnitude = Citizen->AIController->GetClosestActor(Citizen->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation());

				if (magnitude <= 0.0f)
					continue;

				target = building;
			}
		}

		Orders[0].Items[i].Stored -= amount;

		if (target != nullptr) {
			Citizen->Carry(Orders[0].Items[i].Resource->GetDefaultObject<AResource>(), amount, target);

			return;
		}
		else
			Orders[0].Items[i].Stored = 0;
	}

	Orders[0].OrderWidget->RemoveFromParent();

	Orders.RemoveAt(0);

	if (Orders.Num() > 0) {
		if (Orders[0].bCancelled)
			ReturnResource(Citizen);
		else
			CheckStored(Citizen, Orders[0].Items);
	}
}

void ABuilding::SetNewOrder(FQueueStruct Order)
{
	Orders.Add(Order);

	UResourceManager* rm = Camera->ResourceManagerComponent;

	for (FItemStruct items : Order.Items)
		rm->AddCommittedResource(items.Resource, items.Amount);

	if (Orders.Num() != 1)
		return;

	TArray<ACitizen*> citizens = GetCitizensAtBuilding();

	for (ACitizen* citizen : citizens)
		CheckStored(citizen, Orders[0].Items);
}

void ABuilding::SetOrderWidget(int32 index, UWidget* Widget)
{
	Orders[index].OrderWidget = Widget;
}

void ABuilding::SetOrderCancelled(int32 index, bool bCancel)
{
	Orders[index].bCancelled = bCancel;

	UResourceManager* rm = Camera->ResourceManagerComponent;

	for (FItemStruct items : Orders[index].Items)
		rm->TakeCommittedResource(items.Resource, items.Amount - items.Stored);

	for (ACitizen* Citizen : GetOccupied())
		Citizen->AIController->AIMoveTo(this);
}