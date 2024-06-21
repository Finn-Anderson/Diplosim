#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"

#include "Camera.h"
#include "ConstructionManager.h"
#include "Buildings/Building.h"
#include "Buildings/Builder.h"
#include "Buildings/Wall.h"
#include "Map/Grid.h"
#include "Map/Vegetation.h"
#include "Map/Mineral.h"
#include "DiplosimGameModeBase.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Broch.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bCanRotate = true;
	Building = nullptr;

	Rotation.Yaw = 90.0f;

	BuildingToMove = nullptr;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Building)
		return;

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		UHierarchicalInstancedStaticMeshComponent* comp = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.GetComponent());
		int32 instance = hit.Item;

		FTransform transform;
		comp->GetInstanceTransform(instance, transform);

		FVector location = transform.GetLocation();

		if (comp == Camera->Grid->HISMFlatGround)
			location.Z += Camera->Grid->HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 2.0f;
		else
			location.Z += comp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 2.0f;

		Building->SetActorLocation(location);

		Camera->WidgetComponent->SetWorldLocation(location + FVector(0.0f, 0.0f, Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z));
	}

	UDecalComponent* decalComp = Building->FindComponentByClass<UDecalComponent>();

	if (!IsValidLocation() || !Building->CheckBuildCost()) {
		Building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);

		if (decalComp != nullptr)
			decalComp->SetHiddenInGame(true);
	}
	else {
		Building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

		if (decalComp != nullptr)
			decalComp->SetHiddenInGame(false);
	}
}

bool UBuildComponent::IsValidLocation()
{
	FVector size = Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

	FVector location = Building->BuildingMesh->GetRelativeLocation();

	int32 numCollisionsX = (FMath::Floor(size.X / 151.0f) * 3);
	int32 numCollisionsY = (FMath::Floor(size.Y / 151.0f) * 3);

	if (numCollisionsX == 0)
		numCollisionsX = 1;

	if (numCollisionsY == 0)
		numCollisionsY = 1;

	int32 numCollisions = numCollisionsX * numCollisionsY;

	if (Building->bOffset)
		numCollisions -= 1;

	if (numCollisions > Building->Collisions.Num())
		return false;

	for (FCollisionStruct collision : Building->Collisions) {
		if (Building->GetClass() == FoundationClass->GetDefaultObject()->GetClass() && collision.Actor->IsA<AGrid>())
			continue;

		FTransform transform;

		if (collision.HISM->IsValidLowLevelFast())
			collision.HISM->GetInstanceTransform(collision.Instance, transform);
		else
			transform = collision.Actor->GetTransform();

		if (Building->IsA<AWall>() && collision.Actor->IsA<AWall>()) {
			if (Building->GetActorLocation() == collision.Actor->GetActorLocation())
				return false;
		}	
		else if (transform.GetLocation().Z != FMath::Floor(Building->GetActorLocation().Z) - 100.0f)
			return false;
	}

	return true;
}

void UBuildComponent::SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass)
{
	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0, 0, 50.0f), Rotation);

	Camera->DisplayInteract(Building);
}

void UBuildComponent::DetachBuilding()
{
	if (Building == nullptr)
		return;

	Camera->WidgetComponent->SetHiddenInGame(true);
	Camera->WidgetComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

	SetComponentTickEnabled(false);

	Building = nullptr;
}

void UBuildComponent::RemoveBuilding()
{
	if (Building == nullptr)
		return;

	for (FTreeStruct treeStruct : Building->TreeList)
		treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 1.0f);

	Building->DestroyBuilding();

	DetachBuilding();
}

void UBuildComponent::RotateBuilding(bool Rotate)
{
	if (Building != nullptr && Rotate && bCanRotate) {
		int32 yaw = Rotation.Yaw;

		if (yaw % 90 == 0)
			yaw += 90;

		if (yaw == 360)
			yaw = 0;

		bCanRotate = false;

		Rotation.Yaw = yaw;

		Building->SetActorRotation(Rotation);
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

void UBuildComponent::Place()
{
	if (Building == nullptr || !IsValidLocation() || !Building->CheckBuildCost())
		return;

	for (FTreeStruct treeStruct : Building->TreeList)
		treeStruct.Resource->ResourceHISM->RemoveInstance(treeStruct.Instance);

	if (BuildingToMove->IsValidLowLevelFast()) {
		BuildingToMove->SetActorLocation(Building->GetActorLocation());
		BuildingToMove->SetActorRotation(Building->GetActorRotation());

		TArray<ACitizen*> citizens = BuildingToMove->GetCitizensAtBuilding();

		for (ACitizen* citizen : citizens)
			citizen->SetActorLocation(BuildingToMove->GetActorLocation());

		UConstructionManager* cm = Camera->ConstructionManagerComponent;

		if (cm->IsBeingConstructed(BuildingToMove, nullptr)) {
			ABuilder* builder = cm->GetBuilder(BuildingToMove);

			if (builder != nullptr && !builder->GetOccupied().IsEmpty())
				builder->GetOccupied()[0]->AIController->RecalculateMovement(BuildingToMove);
		}

		for (ACitizen* citizen : BuildingToMove->GetOccupied())
			citizen->AIController->RecalculateMovement(BuildingToMove);

		BuildingToMove = nullptr;

		DetachBuilding();

		return;
	}

	if (Building->IsA<AWall>())
		Cast<AWall>(Building)->StoreSocketLocations();
	
	UDecalComponent* decalComp = Building->FindComponentByClass<UDecalComponent>();

	if (decalComp != nullptr)
		decalComp->SetHiddenInGame(true);

	Building->BuildingMesh->SetOverlayMaterial(nullptr);

	Building->Build();

	if (Camera->Start) {
		Camera->Start = false;
		Camera->BuildUIInstance->AddToViewport();
		Camera->Grid->MapUIInstance->RemoveFromParent();

		Building->BuildingMesh->bReceivesDecals = true;

		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		gamemode->Broch = Building;
		gamemode->Grid = Camera->Grid;

		gamemode->SetWaveTimer();
	}

	DetachBuilding();
}

void UBuildComponent::QuickPlace()
{
	if (Camera->Start || BuildingToMove->IsValidLowLevelFast()) {
		Place();

		return;
	}

	ABuilding* building = Building;

	Place();

	SpawnBuilding(building->GetClass());
}