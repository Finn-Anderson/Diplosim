#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Camera.h"
#include "Buildings/Building.h"
#include "Map/Grid.h"
#include "Map/Vegetation.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	GridStatus = true;
	bCanRotate = true;
	Building = nullptr;

	Rotation.Yaw = 90.0f;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		UHierarchicalInstancedStaticMeshComponent* comp = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.GetComponent());
		int32 instance = hit.Item;

		FVector location;

		if (GridStatus && comp->IsValidLowLevelFast()) {
			FTransform transform;
			comp->GetInstanceTransform(instance, transform);

			location = transform.GetLocation();

			location.Z += comp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 2;
		}
		else {
			location = hit.Location;
		}

		Building->SetActorLocation(location);
	}

	if (Building->Blocking.Num() > 0 || !Building->CheckBuildCost() || !Building->bMoved) {
		Building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);
	}
	else {
		Building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

void UBuildComponent::HideTree(class AVegetation* vegetation, bool Hide)
{
	if (Hide) {
		vegetation->SetActorHiddenInGame(Hide);

		Vegetation.Add(vegetation);
	}
	else {
		vegetation->SetActorHiddenInGame(Hide);

		Vegetation.Remove(vegetation);
	}
}

void UBuildComponent::Build()
{
	SetComponentTickEnabled(!IsComponentTickEnabled());

	if (Building != nullptr && Building->BuildStatus == EBuildStatus::Blueprint) {
		Building->Destroy();
	}

	Building = nullptr;
}

void UBuildComponent::SetBuildingClass(TSubclassOf<class ABuilding> BuildingClass)
{
	if (Building != nullptr && Building->BuildStatus == EBuildStatus::Blueprint) {
		Building->Destroy();
	}

	Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0, 0, 50.0f), Rotation);
}

void UBuildComponent::RotateBuilding(bool Rotate)
{
	if (Building != nullptr && Rotate && bCanRotate) {
		int32 yaw = Rotation.Yaw;
		if (GridStatus) {
			if (yaw % 90 == 0) {
				yaw += 90;
			}
			else {
				yaw += 90 / 2;
				yaw -= yaw % 90;
			}

			bCanRotate = false;
		}
		else {
			yaw += 1;
		}

		if (yaw == 360) {
			yaw = 0;
		}

		Rotation.Yaw = yaw;

		Building->SetActorRotation(Rotation);
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

void UBuildComponent::Place()
{
	if (Building == nullptr || Building->Blocking.Num() > 0 || !Building->CheckBuildCost() || !Building->bMoved)
		return;

	for (int i = 0; i < Vegetation.Num(); i++) {
		Vegetation[i]->Destroy(true);
	}

	Vegetation.Empty();

	Building->BuildingMesh->SetOverlayMaterial(nullptr);

	Building->Build();

	Build();

	if (Camera->start) {
		Camera->start = false;
		Camera->BuildUIInstance->AddToViewport();
		Camera->MapUIInstance->RemoveFromParent();
	}
}

void UBuildComponent::QuickPlace()
{
	if (Camera->start) {
		Place();

		return;
	}

	ABuilding* building = Building;

	Place();

	SetComponentTickEnabled(!IsComponentTickEnabled());

	SetBuildingClass(building->GetClass());
}