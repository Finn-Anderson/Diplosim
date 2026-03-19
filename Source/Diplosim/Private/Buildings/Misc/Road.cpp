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
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(50.0f, 50.0f, 20.0f));
	BoxAreaAffect->SetCanEverAffectNavigation(true);
	BoxAreaAffect->SetupAttachment(RootComponent);
	BoxAreaAffect->bDynamicObstacle = true;
}

void ARoad::BeginPlay()
{
	Super::BeginPlay();

	BoxAreaAffect->OnComponentBeginOverlap.AddDynamic(this, &ARoad::OnCitizenOverlapBegin);
	BoxAreaAffect->OnComponentEndOverlap.AddDynamic(this, &ARoad::OnCitizenOverlapEnd);
}

void ARoad::OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp->IsA<UInstancedStaticMeshComponent>())
		return;

	AAI* ai = Camera->Grid->AIVisualiser->GetHISMAI(Camera, Cast<UInstancedStaticMeshComponent>(OtherComp), OtherBodyIndex);
	ai->MovementComponent->SpeedMultiplier += (0.15f * Tier);
}

void ARoad::OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherComp->IsA<UInstancedStaticMeshComponent>())
		return;

	AAI* ai = Camera->Grid->AIVisualiser->GetHISMAI(Camera, Cast<UInstancedStaticMeshComponent>(OtherComp), OtherBodyIndex);
	ai->MovementComponent->SpeedMultiplier -= (0.15f * Tier);
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
	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 3.0f);

	for (FHitResult hit : hits) {
		AActor* actor = hit.GetActor();

		if ((!actor->IsA<ARoad>() && !actor->IsA<AFestival>()) || actor->IsHidden() || actor->GetActorLocation().Z != GetActorLocation().Z)
			continue;

		FRotator rotation = (GetActorLocation() - actor->GetActorLocation()).Rotation();

		if (rotation.Yaw < 0.0f)
			rotation.Yaw += 360;

		int32 instance = rotation.Yaw / 45.0f;

		HISMRoad->SetCustomDataValue(instance, 0, 1.0f);
	}
}

void ARoad::SetTier(int32 Value)
{
	Super::SetTier(Value);

	for (int32 i = 0; i < HISMRoad->GetInstanceCount(); i++)
		if (HISMRoad->PerInstanceSMCustomData[i * HISMRoad->NumCustomDataFloats] == 1.0f)
			HISMRoad->SetCustomDataValue(i, 0, 0.0f);

	if (SeedNum != 0)
		return;

	BuildingMesh->SetRelativeRotation(FRotator(0.0f));

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(this, 6.0f);

	RegenerateMesh();

	for (FHitResult hit : hits) {
		if (!hit.GetActor()->IsA<ARoad>() || Cast<ARoad>(hit.GetActor())->SeedNum != 0)
			continue;

		Cast<ARoad>(hit.GetActor())->RegenerateMesh();
	}
}