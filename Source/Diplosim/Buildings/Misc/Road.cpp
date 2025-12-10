#include "Buildings/Misc/Road.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/AI.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Map/Grid.h"
#include "Festival.h"

ARoad::ARoad()
{
	HealthComponent->MaxHealth = 20;
	HealthComponent->Health = HealthComponent->MaxHealth;

	HISMRoad = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRoad"));
	HISMRoad->SetupAttachment(RootComponent);
	HISMRoad->SetGenerateOverlapEvents(false);
	HISMRoad->NumCustomDataFloats = 1;
	HISMRoad->bAutoRebuildTreeOnInstanceChanges = false;
	HISMRoad->PrimaryComponentTick.bCanEverTick = false;

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
	BoxAreaAffect->SetCanEverAffectNavigation(true);
	BoxAreaAffect->SetupAttachment(RootComponent);
	BoxAreaAffect->bDynamicObstacle = true;
}

void ARoad::BeginPlay()
{
	Super::BeginPlay();

	HISMRoad->BuildTreeIfOutdated(true, false);

	BoxAreaAffect->OnComponentBeginOverlap.AddDynamic(this, &ARoad::OnCitizenOverlapBegin);
	BoxAreaAffect->OnComponentEndOverlap.AddDynamic(this, &ARoad::OnCitizenOverlapEnd);
}

void ARoad::OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->IsA<AAI>())
		return;

	Cast<AAI>(OtherActor)->MovementComponent->SpeedMultiplier += (0.15f * Tier);
}

void ARoad::OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->IsA<AAI>())
		return;

	Cast<AAI>(OtherActor)->MovementComponent->SpeedMultiplier -= (0.15f * Tier);
}

void ARoad::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	Super::Build(bRebuild, bUpgrade, Grade);

	HISMRoad->SetOverlayMaterial(nullptr);
}

void ARoad::SetBuildingColour(float R, float G, float B)
{
	Super::SetBuildingColour(R, G, B);

	HISMRoad->SetCustomPrimitiveDataFloat(1, R);
	HISMRoad->SetCustomPrimitiveDataFloat(2, G);
	HISMRoad->SetCustomPrimitiveDataFloat(3, B);
}

void ARoad::DestroyBuilding(bool bCheckAbove, bool bMove)
{
	Super::DestroyBuilding(bCheckAbove);

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 6.0f);

	for (FHitResult hit : hits) {
		if (!IsValid(hit.GetActor()) || hit.GetActor()->IsPendingKillPending() || !hit.GetActor()->IsA<ARoad>())
			continue;

		Cast<ARoad>(hit.GetActor())->RegenerateMesh();
	}
}

void ARoad::RegenerateMesh()
{
	if (BuildingMesh->GetStaticMesh() != RoadMeshes[0])
		return;

	for (int32 i = 0; i < HISMRoad->GetInstanceCount(); i++)
		HISMRoad->SetCustomDataValue(i, 0, 0.0f);

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 3.0f);

	for (FHitResult hit : hits) {
		AActor* actor = hit.GetActor();

		if ((!actor->IsA<ARoad>() && !actor->IsA<AFestival>()) || actor->IsHidden() || actor->GetActorLocation().Z != GetActorLocation().Z)
			continue;

		FRotator rotation = (GetActorLocation() - actor->GetActorLocation()).Rotation();

		if (rotation.Yaw < 0.0f)
			rotation.Yaw += 360;

		int32 instance = rotation.Yaw / 45.0f;

		bool bShow = true;
		bool bUpdate = false;

		FTransform transform;
		HISMRoad->GetInstanceTransform(instance, transform);
		transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

		if ((actor->IsA<ARoad>() && Cast<ARoad>(actor)->BuildingMesh->GetStaticMesh() != RoadMeshes[0])) {
			if ((int32)rotation.Yaw % 90 != 0)
				bShow = false;
			else
				transform.SetScale3D(FVector(1.0f, 0.4f, 1.0f));

			bUpdate = true;
		}

		if (bShow) {
			if (bUpdate)
				HISMRoad->UpdateInstanceTransform(instance, transform, false);

			HISMRoad->SetCustomDataValue(instance, 0, 1.0f);
		}
	}
}

void ARoad::SetTier(int32 Value)
{
	Super::SetTier(Value);

	FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorLocation());

	if (tile->bRiver) {
		FTileStruct* left = *tile->AdjacentTiles.Find("Left");
		FTileStruct* right = *tile->AdjacentTiles.Find("Right");

		FTileStruct* below = *tile->AdjacentTiles.Find("Below");
		FTileStruct* above = *tile->AdjacentTiles.Find("Above");

		FTileStruct* chosen = nullptr;

		if (left->bEdge && right->bEdge && left->Level == right->Level)
			chosen = left;
		else if (below->bEdge && above->bEdge && below->Level == above->Level)
			chosen = below;

		if (chosen != nullptr) {
			FTransform transform = Camera->Grid->GetTransform(chosen);

			FRotator rotation = FRotator(0.0f, 90.0f + (GetActorLocation() - transform.GetLocation()).Rotation().Yaw, 0.0f);

			BuildingMesh->SetRelativeRotation(rotation);

			if (tile->Level != chosen->Level)
				SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 75.0f));

			if (GetTier() == 1)
				BuildingMesh->SetStaticMesh(RoadMeshes[1]);
			else
				BuildingMesh->SetStaticMesh(RoadMeshes[2]);

			for (int32 i = 0; i < HISMRoad->GetInstanceCount(); i++)
				HISMRoad->SetCustomDataValue(i, 0, 0.0f);

			return;
		}
	}

	BuildingMesh->SetRelativeRotation(FRotator(0.0f));
	BuildingMesh->SetStaticMesh(RoadMeshes[0]);

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 6.0f);

	RegenerateMesh();

	for (FHitResult hit : hits) {
		if (!hit.GetActor()->IsA<ARoad>())
			continue;

		Cast<ARoad>(hit.GetActor())->RegenerateMesh();
	}
}