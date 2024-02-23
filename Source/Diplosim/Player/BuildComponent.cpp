#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"

#include "Camera.h"
#include "Buildings/Building.h"
#include "Buildings/Builder.h"
#include "Buildings/Wall.h"
#include "Map/Grid.h"
#include "Map/Vegetation.h"
#include "DiplosimGameModeBase.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

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

		location.Z += comp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

		Building->SetActorLocation(location);
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
	if (Building->Collisions.Num() == 0)
		return false;

	FVector building = Building->GetActorLocation();
	building.X = FMath::RoundHalfFromZero(building.X);
	building.Y = FMath::RoundHalfFromZero(building.Y);
	building.Z = FMath::RoundHalfFromZero(building.Z);

	for (const FBuildStruct buildStruct : Building->Collisions) {
		if (buildStruct.Object == Camera->Grid->HISMLava || buildStruct.Object == Camera->Grid->HISMWater)
			return false;

		FVector location = buildStruct.Location;
		location.X = FMath::RoundHalfFromZero(location.X);
		location.Y = FMath::RoundHalfFromZero(location.Y);
		location.Z = FMath::RoundHalfFromZero(location.Z);

		if (building.Z == location.Z && location.Z >= 100.0f)
			continue;

		if (building.X == location.X && building.Y == location.Y)
			return false;

		FVector temp;
		float d = Building->BuildingMesh->GetClosestPointOnCollision(location, temp);

		if (d < 49.0f)
			return false;
	}

	return true;
}

bool UBuildComponent::SetBuildingClass(TSubclassOf<class ABuilding> BuildingClass)
{
	if (Building != nullptr) {
		if (Building->BuildStatus == EBuildStatus::Blueprint) {
			for (FTreeStruct treeStruct : Building->TreeList) {
				treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 1.0f);
			}

			Building->Destroy();
		}

		if (BuildingClass == Building->GetClass()) {
			SetComponentTickEnabled(false);

			Building = nullptr;

			return false;
		} 
	}

	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0, 0, 50.0f), Rotation);
	
	return true;
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

		TArray<ACitizen*> citizens = BuildingToMove->GetCitizensAtBuilding();

		for (ACitizen* citizen : citizens)
			citizen->SetActorLocation(BuildingToMove->GetActorLocation());

		if (BuildingToMove->BuildStatus == EBuildStatus::Construction) {
			TArray<AActor*> builders;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilder::StaticClass(), builders);

			for (AActor* actor : builders) {
				ABuilder* builder = Cast<ABuilder>(actor);

				if (builder->Constructing != BuildingToMove)
					continue;

				if (!builder->GetOccupied().IsEmpty())
					builder->GetOccupied()[0]->AIController->RecalculateMovement(BuildingToMove);

				break;
			}
		}
		else {
			for (ACitizen* citizen : BuildingToMove->GetOccupied())
				citizen->AIController->RecalculateMovement(BuildingToMove);
		}

		BuildingToMove = nullptr;

		SetBuildingClass(Building->GetClass());

		return;
	}

	if (Building->IsA<AWall>())
		Cast<AWall>(Building)->StoreSocketLocations();
	
	UDecalComponent* decalComp = Building->FindComponentByClass<UDecalComponent>();

	if (decalComp != nullptr)
		decalComp->SetHiddenInGame(true);

	Building->BuildingMesh->SetOverlayMaterial(nullptr);

	Building->Build();

	if (Camera->start) {
		Camera->start = false;
		Camera->BuildUIInstance->AddToViewport();
		Camera->Grid->MapUIInstance->RemoveFromParent();

		Building->BuildingMesh->bReceivesDecals = true;

		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		gamemode->Broch = Building;
		gamemode->Grid = Camera->Grid;

		gamemode->SetWaveTimer();
	}

	SetBuildingClass(Building->GetClass());
}

void UBuildComponent::QuickPlace()
{
	if (Camera->start || BuildingToMove->IsValidLowLevelFast()) {
		Place();

		return;
	}

	ABuilding* building = Building;

	Place();

	SetBuildingClass(building->GetClass());
}