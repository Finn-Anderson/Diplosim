#include "Buildings/Misc/Road.h"

#include "Kismet/GameplayStatics.h"
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
	HISMRoad->bAutoRebuildTreeOnInstanceChanges = false;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BoxCollision->SetBoxExtent(FVector(99.0f, 99.0f, 20.0f));
	BoxCollision->SetupAttachment(RootComponent);

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

	BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ARoad::OnRoadOverlapBegin);
	BoxCollision->OnComponentEndOverlap.AddDynamic(this, &ARoad::OnRoadOverlapEnd);
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

void ARoad::OnRoadOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!(OtherActor->IsA<ARoad>() || OtherActor->IsA<AFestival>()) || OtherActor->IsHidden() || OtherActor == this || GetActorLocation() == OtherActor->GetActorLocation() || OtherComp->IsA<UHierarchicalInstancedStaticMeshComponent>() || !OtherComp->IsA<UStaticMeshComponent>())
		return;

	RegenerateMesh();
}

void ARoad::OnRoadOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!(OtherActor->IsA<ARoad>() || OtherActor->IsA<AFestival>()) || OtherActor->IsHidden() || OtherActor == this || GetActorLocation() == OtherActor->GetActorLocation())
		return;

	RegenerateMesh();
}

void ARoad::RegenerateMesh()
{
	HISMRoad->ClearInstances();

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

			return;
		}
	}

	BuildingMesh->SetRelativeRotation(FRotator(0.0f));
	BuildingMesh->SetStaticMesh(RoadMeshes[0]);

	TArray<FTransform> transforms;

	TArray<AActor*> connectedRoads;
	BoxCollision->GetOverlappingActors(connectedRoads);

	for (AActor* actor : connectedRoads) {
		if ((!actor->IsA<ARoad>() && !actor->IsA<AFestival>()) || (!Camera->BuildComponent->Buildings.IsEmpty() && Camera->BuildComponent->Buildings[0] == actor))
			continue;

		FVector location = actor->GetActorLocation();

		if (actor->IsA<AFestival>()) {
			FVector dist = GetActorLocation() - location;

			float x = FMath::RoundHalfFromZero(FMath::Abs(dist.X));
			float y = FMath::RoundHalfFromZero(FMath::Abs(dist.Y));

			int32 xSign = 1;
			int32 ySign = 1;

			if (dist.X > 0)
				xSign = -1;

			if (dist.Y > 0)
				ySign = -1;

			if (x > y)
				ySign = 0;
			else if (y > x)
				xSign = 0;

			location = GetActorLocation() + FVector(100.0f * xSign, 100.0f * ySign, 0.0f);
		}

		FTransform transform;

		if (IsValid(actor) && !actor->IsHidden() && actor != this && GetActorLocation() != location && (Camera->BuildComponent->Buildings.IsEmpty() || Camera->BuildComponent->Buildings[0] != actor)) {
			transform.SetRotation(FRotator(0.0f, 90.0f + (GetActorLocation() - location).Rotation().Yaw, 0.0f).Quaternion());

			FTileStruct* t = Camera->Grid->GetTileFromLocation(location);

			if (t->Level != tile->Level)
				continue;

			if (t->bRiver) {
				bool bValid = false;

				for (auto& element : t->AdjacentTiles)
					if (element.Value->X == tile->X && element.Value->Y == tile->Y)
						bValid = true;

				if (!bValid)
					continue;

				transform.SetScale3D(FVector(1.0f, 0.4f, 1.0f));
			}

			transforms.Add(transform);
		}
	}

	HISMRoad->AddInstances(transforms, false);
	HISMRoad->BuildTreeIfOutdated(true, true);
}

void ARoad::SetTier(int32 Value)
{
	Super::SetTier(Value);

	RegenerateMesh();
}