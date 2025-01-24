#include "Buildings/Misc/Road.h"

#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/AIMovementComponent.h"
#include "AI/AI.h"
#include "Universal/HealthComponent.h"

ARoad::ARoad()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTickableWhenPaused(true);
	SetActorTickInterval(0.1f);

	HealthComponent->MaxHealth = 20;
	HealthComponent->Health = HealthComponent->MaxHealth;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));
	BoxCollision->SetBoxExtent(FVector(55.0f, 55.0f, 1.0f));
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
}

void ARoad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RegenerateMesh();
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

void ARoad::RegenerateMesh()
{
	TArray<AActor*> actors;
	BoxCollision->GetOverlappingActors(actors);

	TArray<ARoad*> connectedRoads;

	for (AActor* actor : actors)
		if (actor->IsA<ARoad>() && actor != this && (GetActorLocation().X == actor->GetActorLocation().X || GetActorLocation().Y == actor->GetActorLocation().Y))
			connectedRoads.Add(Cast<ARoad>(actor));

	FRoadStruct roadStruct;
	roadStruct.Connections = connectedRoads.Num();
	roadStruct.bStraight = false;

	FVector midPoint;

	if (connectedRoads.Num() == 2)
		if (connectedRoads[0]->GetActorLocation().X == connectedRoads[1]->GetActorLocation().X || connectedRoads[0]->GetActorLocation().Y == connectedRoads[1]->GetActorLocation().Y)
			roadStruct.bStraight = true;

	for (ARoad* road : connectedRoads)
		midPoint += road->GetActorLocation();

	midPoint /= connectedRoads.Num();

	int32 index = RoadMeshes.Find(roadStruct);

	if (index == INDEX_NONE)
		return;

	FVector direction = (GetActorLocation() - midPoint);
	FRotator rotate = FRotator(0.0f, 90.0f, 0.0f);

	if (roadStruct.bStraight)
		direction = GetActorLocation() - connectedRoads[0]->GetActorLocation();
	else if (roadStruct.Connections == 2)
		rotate = FRotator(0.0f, 45.0f, 0.0f);

	if (midPoint.Z == GetActorLocation().Z) {
		FRotator rotation = direction.Rotation() + rotate;
		SetActorRotation(rotation);
	}

	BuildingMesh->SetStaticMesh(RoadMeshes[index].Mesh);

	FVector size = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2;
	BoxAreaAffect->SetBoxExtent(FVector(size.X, size.Y, 20.0f));
}