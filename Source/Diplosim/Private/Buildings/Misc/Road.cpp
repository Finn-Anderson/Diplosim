#include "Buildings/Misc/Road.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "NavigationSystem.h"

#include "AI/AI.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Misc/Festival.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Universal/HealthComponent.h"

ARoad::ARoad()
{
	HealthComponent->MaxHealth = 20;
	HealthComponent->Health = HealthComponent->MaxHealth;

	HISMRoad = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISMRoad"));
	HISMRoad->SetupAttachment(RootComponent);
	HISMRoad->SetGenerateOverlapEvents(false);
	HISMRoad->NumCustomDataFloats = 1;
	HISMRoad->PrimaryComponentTick.bCanEverTick = false;

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetupAttachment(RootComponent);
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
	BoxAreaAffect->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxAreaAffect->SetCanEverAffectNavigation(true);
	BoxAreaAffect->bDynamicObstacle = true;
}

void ARoad::BeginPlay()
{
	Super::BeginPlay();

	SetTier(Tier);
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

	RegenerateMesh(true);
}

void ARoad::RegenerateMesh(bool bRegenerateHits)
{
	if (SeedNum != 0)
		return;

	for (int32 i = 0; i < HISMRoad->GetInstanceCount(); i++)
		if (HISMRoad->PerInstanceSMCustomData[i * HISMRoad->NumCustomDataFloats] == 1.0f)
			HISMRoad->SetCustomDataValue(i, 0, 0.0f);

	for (int32 i = LastHitRoads.Num() - 1; i > -1; i--) {
		LastHitRoads[i]->RegenerateMesh(false);

		LastHitRoads.RemoveAt(i);
	}

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 4.0f);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	for (const FHitResult& hit : hits) {
		AActor* actor = hit.GetActor();

		if ((!actor->IsA<ARoad>() && !actor->IsA<AFestival>()) || actor->IsHidden() || actor->GetActorLocation() == GetActorLocation())
			continue;

		FRotator thisRotation = GetActorRotation();
		FRotator actorRotation = actor->GetActorRotation();

		if (actor->GetActorLocation().Z != GetActorLocation().Z) {
			if (thisRotation.Pitch == 0.0f && thisRotation.Roll == 0.0f && actorRotation.Pitch == 0.0f && actorRotation.Roll == 0.0f)
				continue;

			int32 yaw = FMath::RoundHalfFromZero(thisRotation.Yaw);
			if (thisRotation.Pitch == 0.0f && thisRotation.Roll == 0.0f)
				yaw = FMath::RoundHalfFromZero(actorRotation.Yaw);

			FRotator direction = (actor->GetActorLocation() - GetActorLocation()).Rotation();
			int32 targetYaw = FMath::RoundHalfFromZero(direction.Yaw);

			if (yaw != targetYaw && yaw != targetYaw + 180 && yaw != targetYaw - 180)
				continue;
		}
		
		FVector actorLocation = actor->GetActorLocation();
		if (actor->IsA<AFestival>())
			Cast<UStaticMeshComponent>(actor->GetRootComponent())->GetClosestPointOnCollision(GetActorLocation(), actorLocation);

		FRotator rotation = (GetActorLocation() - actorLocation).Rotation();

		if (rotation.Yaw < 0.0f)
			rotation.Yaw += 360.0f;

		if (actor->IsA<ARoad>() && Cast<ARoad>(actor)->SeedNum != 0 && FVector::Dist(Cast<ARoad>(actor)->BuildingMesh->GetSocketLocation("Point1"), GetActorLocation()) > 50.0f && FVector::Dist(Cast<ARoad>(actor)->BuildingMesh->GetSocketLocation("Point2"), GetActorLocation()) > 50.0f)
			continue;

		int32 instance = rotation.Yaw / 45.0f;

		FTransform transform;
		HISMRoad->GetInstanceTransform(instance, transform);

		if ((int32)rotation.Yaw % 90 != 0) {
			FTileStruct* tile1 = Camera->Grid->GetTileFromLocation(GetActorLocation() + FVector(0.0f, actor->GetActorLocation().Y - GetActorLocation().Y, 0.0f));
			if (tile1->bRamp)
				continue;

			FTileStruct* tile2 = Camera->Grid->GetTileFromLocation(GetActorLocation() + FVector(actor->GetActorLocation().X - GetActorLocation().X, 0.0f, 0.0f));
			if (tile2->bRamp)
				continue;

			FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorLocation());

			if (tile->Level != tile1->Level || tile->Level != tile2->Level)
				continue;

			if (actor->IsA<AFestival>())
				transform.SetScale3D(FVector(1.0f, 1.5f, 1.0f));
		}
		else {
			if (thisRotation.Roll != 0.0f || thisRotation.Pitch != 0.0f)
				transform.SetScale3D(FVector(1.0f, 0.85f, 1.0f));
			else
				transform.SetScale3D(FVector(1.0f, 0.67f, 1.0f));
		}

		HISMRoad->UpdateInstanceTransform(instance, transform);
		HISMRoad->SetCustomDataValue(instance, 0, 1.0f);

		if (bRegenerateHits && actor->IsA<ARoad>() && Cast<ARoad>(actor)->SeedNum == 0) {
			Cast<ARoad>(actor)->RegenerateMesh(false);

			LastHitRoads.Add(Cast<ARoad>(actor));
		}
	}
}

void ARoad::SetTier(int32 Value)
{
	Super::SetTier(Value);

	TArray<float> data = BuildingMesh->GetCustomPrimitiveData().Data;

	HISMRoad->SetCustomPrimitiveDataFloat(1, data[1]);
	HISMRoad->SetCustomPrimitiveDataFloat(2, data[2]);
	HISMRoad->SetCustomPrimitiveDataFloat(3, data[3]);
}