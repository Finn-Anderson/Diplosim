#include "Buildings/Misc/Road.h"

#include "Kismet/GameplayStatics.h"

#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	BoxCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));
	BoxCollision->SetBoxExtent(FVector(75.0f, 75.0f, 1.0f));
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

	Cast<AAI>(OtherActor)->MovementComponent->SetMultiplier(1.15f);
}

void ARoad::OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->IsA<AAI>())
		return;

	Cast<AAI>(OtherActor)->MovementComponent->SetMultiplier(0.85f);
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
		roadStruct.Connections = 2;
		roadStruct.bStraight = true;
		roadStruct.bBridge = true;

		connectedLocations.Empty();

		for (auto& element : tile.AdjacentTiles) {
			FTileStruct* t = element.Value;

			if (!t->bEdge || t->bRiver)
				continue;

			FTransform transform = Camera->Grid->GetTransform(t);
			transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 100.0f));

			connectedLocations.Add(transform.GetLocation());
		}
	}

	FVector midPoint;

	for (FVector location : connectedLocations)
		midPoint += location;

	midPoint /= connectedLocations.Num();

	int32 index = RoadMeshes.Find(roadStruct);

	if (index == INDEX_NONE)
		return;

	BuildingMesh->SetStaticMesh(RoadMeshes[index].Mesh);

	FRotator rotation = (midPoint - GetActorLocation()).Rotation();
	rotation.Pitch = 0.0f;
	rotation.Roll = 0.0f;
	rotation.Yaw = FMath::Floor(rotation.Yaw / 90.0f) * 90.0f - 90.0f;

	if (roadStruct.bStraight && roadStruct.Connections == 2 && connectedLocations[0].X == connectedLocations[1].X)
		rotation.Yaw += 90.0f;

	if (rotation.Yaw == 360.0f)
		rotation.Yaw = 0.0f;

	SetActorRotation(rotation);

	FVector size = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2;
	BoxAreaAffect->SetBoxExtent(FVector(size.X, size.Y, 20.0f));
}