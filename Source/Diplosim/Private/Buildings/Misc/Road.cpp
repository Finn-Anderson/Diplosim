#include "Buildings/Misc/Road.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

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

	HISMRoad = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HISMRoad"));
	HISMRoad->SetupAttachment(RootComponent);
	HISMRoad->SetGenerateOverlapEvents(false);
	HISMRoad->NumCustomDataFloats = 1;
	HISMRoad->PrimaryComponentTick.bCanEverTick = false;

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetupAttachment(RootComponent);
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
	BoxAreaAffect->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxAreaAffect->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
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
	for (int32 i = 0; i < HISMRoad->GetInstanceCount(); i++)
		if (HISMRoad->PerInstanceSMCustomData[i * HISMRoad->NumCustomDataFloats] == 1.0f)
			HISMRoad->SetCustomDataValue(i, 0, 0.0f);

	if (SeedNum == 0)
		BuildingMesh->SetRelativeRotation(FRotator(0.0f));

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 3.0f);

	for (FHitResult hit : hits) {
		AActor* actor = hit.GetActor();

		if ((!actor->IsA<ARoad>() && !actor->IsA<AFestival>()) || actor->IsHidden() || actor->GetActorLocation().Z != GetActorLocation().Z)
			continue;

		FRotator rotation = (GetActorLocation() - actor->GetActorLocation()).Rotation();

		if (rotation.Yaw < 0.0f)
			rotation.Yaw += 360.0f;

		int32 instance = rotation.Yaw / 45.0f;

		if (actor->IsA<ARoad>() && Cast<ARoad>(actor)->SeedNum != 0 && FVector::Dist(Cast<ARoad>(actor)->BuildingMesh->GetSocketLocation("Point1"), GetActorLocation()) > 50.0f && FVector::Dist(Cast<ARoad>(actor)->BuildingMesh->GetSocketLocation("Point2"), GetActorLocation()) > 50.0f)
			continue;

		if (SeedNum == 0)
			HISMRoad->SetCustomDataValue(instance, 0, 1.0f);

		if (bRegenerateHits && actor->IsA<ARoad>() && Cast<ARoad>(actor)->SeedNum == 0)
			Cast<ARoad>(actor)->RegenerateMesh(false);
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