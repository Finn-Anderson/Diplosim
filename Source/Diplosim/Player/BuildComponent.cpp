#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Camera.h"
#include "Buildings/Building.h"
#include "Buildings/Wall.h"
#include "Map/Grid.h"
#include "Map/Vegetation.h"
#include "DiplosimGameModeBase.h"

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

	if (Building) {
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

				location.Z += comp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
			}
			else {
				location = hit.Location;
			}

			Building->SetActorLocation(location);
		}

		if (!IsValidLocation() || !Building->CheckBuildCost()) {
			Building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);
		}
		else {
			Building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);
		}
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

bool UBuildComponent::IsValidLocation()
{
	if (Building->Collisions.Num() == 0)
		return false;

	FVector building = Building->GetActorLocation();
	building.X = FMath::RoundHalfFromZero(building.X);
	building.Y = FMath::RoundHalfFromZero(building.Y);
	building.Z = FMath::RoundHalfFromZero(building.Z);

	for (const FBuildStruct buildStruct : Building->Collisions) {
		FVector location = buildStruct.Location;
		location.X = FMath::RoundHalfFromZero(location.X);
		location.Y = FMath::RoundHalfFromZero(location.Y);
		location.Z = FMath::RoundHalfFromZero(location.Z);

		bool sameLoc = false;

		if (buildStruct.Object->IsA<ABuilding>() && buildStruct.Location.X == Building->GetActorLocation().X && buildStruct.Location.Y == Building->GetActorLocation().Y) {
			sameLoc = true;
		}

		TArray<FName> allowedList = { "BP_Ramp_C", "BP_Wall_C" };

		if (location.Z < 100.0f || location.Z < building.Z || (location.Z > building.Z && (sameLoc || buildStruct.Object->GetClass() != Building->GetClass() || !allowedList.Contains(Building->GetClass()->GetName())))) {
			return false;
		}
	}

	return true;
}

bool UBuildComponent::SetBuildingClass(TSubclassOf<class ABuilding> BuildingClass)
{
	if (Building != nullptr) {
		if (Building->BuildStatus == EBuildStatus::Blueprint) {
			for (int i = 0; i < Building->TreeList.Num(); i++) {
				Building->TreeList[i]->SetActorHiddenInGame(false);
			}

			Building->Destroy();
		}

		if (BuildingClass == Building->GetClass()) {
			SetComponentTickEnabled(false);

			Building = nullptr;

			return false;
		} 
	}

	if (!IsComponentTickEnabled()) {
		SetComponentTickEnabled(true);
	}

	Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0, 0, 50.0f), Rotation);
	
	return true;
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
	if (Building == nullptr || !IsValidLocation() || !Building->CheckBuildCost())
		return;

	for (int i = 0; i < Building->TreeList.Num(); i++) {
		Building->TreeList[i]->Destroy(true);
	}

	if (Building->IsA<AWall>()) {
		Cast<AWall>(Building)->StoreSocketLocations();
	}

	Building->BuildingMesh->SetOverlayMaterial(nullptr);

	Building->Build();

	if (Camera->start) {
		Camera->start = false;
		Camera->BuildUIInstance->AddToViewport();
		Camera->MapUIInstance->RemoveFromParent();

		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		gamemode->SetWaveTimer();
		gamemode->Broch = Building;
	}

	SetBuildingClass(Building->GetClass());
}

void UBuildComponent::QuickPlace()
{
	if (Camera->start) {
		Place();

		return;
	}

	ABuilding* building = Building;

	Place();

	if (!IsComponentTickEnabled()) {
		SetComponentTickEnabled(!IsComponentTickEnabled());

		SetBuildingClass(building->GetClass());
	}
}