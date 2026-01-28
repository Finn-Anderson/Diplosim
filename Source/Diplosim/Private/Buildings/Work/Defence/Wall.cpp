#include "Buildings/Work/Defence/Wall.h"

#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

#include "AI/Citizen/Citizen.h"
#include "Buildings/Work/Defence/Fort.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/AttackComponent.h"

AWall::AWall()
{
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetupAttachment(BuildingMesh);
	RangeComponent->SetGenerateOverlapEvents(false);
	RangeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	RangeComponent->SetSphereRadius(400.0f);
	RangeComponent->SetCanEverAffectNavigation(false);
	RangeComponent->bDynamicObstacle = true;

	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	DecalComponent->SetVisibility(true);

	bHideCitizen = false;

	DecalComponent->DecalSize = FVector(400.0f, 400.0f, 400.0f);
}

void AWall::BeginPlay()
{
	Super::BeginPlay();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);
}

void AWall::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(BuildingProjectileClass);
}

void AWall::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(nullptr);
}

void AWall::SetRotationMesh(int32 yaw)
{
	if (SeedNum != 0)
		return;

	FVector scale = FVector(1.0f);

	if (yaw % 90 != 0)
		scale = FVector(1.415f, 1.0f, 1.0f);
		
	BuildingMesh->SetRelativeScale3D(scale);
}

void AWall::SetRange()
{
	if (GetOccupied().IsEmpty())
		return;

	int32 largestRange = 0.0f;
	
	for (ACitizen* citizen : GetOccupied()) {
		if (citizen->Range <= largestRange)
			continue;

		largestRange = citizen->Range;
	}

	FVector size = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

	if (IsA<AFort>()) {
		int32 z = FMath::Clamp(size.Z / 100.0f, 1, 5);

		largestRange *= z;
	}

	float buildingRange = size.Y / 2.0f;
	
	if (size.X / 2.0f > buildingRange)
		buildingRange = size.X / 2.0f;

	RangeComponent->SetSphereRadius(largestRange + buildingRange);
}