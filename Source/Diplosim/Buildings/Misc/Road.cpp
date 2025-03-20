#include "Buildings/Misc/Road.h"

#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/AI.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Map/Grid.h"

ARoad::ARoad()
{
	HealthComponent->MaxHealth = 20;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BoxCollision->SetBoxExtent(FVector(99.0f, 99.0f, 20.0f));
	BoxCollision->SetupAttachment(RootComponent);

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
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
	if (!OtherActor->IsA<ARoad>() || OtherActor->IsHidden() || OtherActor == this || GetActorLocation() == OtherActor->GetActorLocation())
		return;

	RegenerateMesh();
}

void ARoad::OnRoadOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->IsA<ARoad>() || OtherActor->IsHidden() || OtherActor == this || GetActorLocation() == OtherActor->GetActorLocation())
		return;

	RegenerateMesh();
}

void ARoad::RegenerateMesh()
{
	FRoadStruct bridgeStruct;
	bridgeStruct.bBridge = true;
	bridgeStruct.Tier = Tier;

	int32 i = RoadMeshes.Find(bridgeStruct);

	if (RoadMeshes[i].Mesh == BuildingMesh->GetStaticMesh() && !Camera->BuildComponent->Buildings.Contains(this))
		return;
	
	TArray<AActor*> connectedRoads;
	BoxCollision->GetOverlappingActors(connectedRoads);

	TArray<FVector> connectedLocations;

	for (AActor* actor : connectedRoads)
		if ((GetActorLocation().X == actor->GetActorLocation().X || GetActorLocation().Y == actor->GetActorLocation().Y) && IsValid(actor) && actor->IsA<ARoad>() && !actor->IsHidden() && actor != this && GetActorLocation() != actor->GetActorLocation() && Camera->BuildComponent->Buildings[0] != actor)
			connectedLocations.Add(actor->GetActorLocation());

	FRoadStruct roadStruct;
	roadStruct.Connections = connectedLocations.Num();
	roadStruct.bStraight = false;
	roadStruct.bBridge = false;

	if (connectedLocations.Num() == 2)
		if (connectedLocations[0].X == connectedLocations[1].X || connectedLocations[0].Y == connectedLocations[1].Y)
			roadStruct.bStraight = true;

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));
	FTileStruct tile = Camera->Grid->Storage[GetActorLocation().X / 100.0f + bound / 2][GetActorLocation().Y / 100.0f + bound / 2];

	if (tile.bRiver) {
		roadStruct.bStraight = true;
		roadStruct.bBridge = true;
		roadStruct.Tier = Tier;

		connectedLocations.Empty();

		for (auto& element : tile.AdjacentTiles) {
			FTileStruct* t = element.Value;

			if (!t->bEdge || t->bRiver)
				continue;

			FTransform transform = Camera->Grid->GetTransform(t);

			connectedLocations.Add(transform.GetLocation());
		}

		roadStruct.Connections = connectedLocations.Num();
	}

	if (roadStruct.Connections == 3 && roadStruct.bBridge) {
		if (connectedLocations[0].X == connectedLocations[1].X || connectedLocations[0].Y == connectedLocations[1].Y)
			connectedLocations.RemoveAt(2);
		else if (connectedLocations[0].X == connectedLocations[2].X || connectedLocations[0].Y == connectedLocations[2].Y)
			connectedLocations.RemoveAt(1);
		else
			connectedLocations.RemoveAt(0);

		if (connectedLocations[0].Z != connectedLocations[1].Z)
			return;

		roadStruct.Connections = 2;
	}

	int32 index = RoadMeshes.Find(roadStruct);

	if (index == INDEX_NONE)
		return;

	FVector midPoint;

	for (FVector location : connectedLocations)
		midPoint += location;

	midPoint /= connectedLocations.Num();

	if (tile.bRiver && roadStruct.bBridge)
		midPoint.Z = FMath::FloorToInt(midPoint.Z / 75.0f) * 75.0f + 25.0f;

	BuildingMesh->SetStaticMesh(RoadMeshes[index].Mesh);

	FRotator rotation = (midPoint - GetActorLocation()).Rotation();
	rotation.Pitch = 0.0f;
	rotation.Roll = 0.0f;
	rotation.Yaw = FMath::FloorToInt(rotation.Yaw / 90.0f) * 90.0f - 90.0f;

	if (roadStruct.bStraight && roadStruct.Connections == 2 && connectedLocations[0].X == connectedLocations[1].X)
		rotation.Yaw += 90.0f;

	if (rotation.Yaw == 360.0f)
		rotation.Yaw = 0.0f;

	SetActorRotation(rotation);

	if (tile.bRiver && roadStruct.bBridge)
		SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, midPoint.Z - GetActorLocation().Z));

	FVector size = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2;
	BoxAreaAffect->SetBoxExtent(FVector(size.X, size.Y, 20.0f));
}