#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Universal/HealthComponent.h"
#include "AIMovementComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Buildings/Misc/Broch.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));

	MovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("MovementComponent"));

	AIController = CreateDefaultSubobject<ADiplosimAIController>(TEXT("AIController"));
	AIController->SetOwner(this);

	InitialRange = 400.0f;
	Range = InitialRange;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MovementComponent->AI = this;
	MovementComponent->AIVisualiser = Camera->Grid->AIVisualiser;

	AIController->Camera = Camera;
	AIController->AI = this;

	AttackComponent->Camera = Camera;
}

void AAI::MoveToBroch()
{
	ABuilding* target = nullptr;

	for (FFactionStruct faction : Camera->ConquestManager->Factions) {
		for (ABuilding* building : faction.Buildings) {
			if (!IsValid(building) || building->HealthComponent == 0 || !AIController->CanMoveTo(building->GetActorLocation()))
				continue;

			if (!IsValid(target)) {
				target = building;

				if (building->IsA<ABroch>())
					break;
				else
					continue;
			}

			int32 targetValue = 1.0f;
			int32 buildingValue = 1.0f;

			if (target->IsA<ABroch>())
				targetValue = 5.0f;

			if (building->IsA<ABroch>())
				buildingValue = 5.0f;

			double magnitude = AIController->GetClosestActor(400.0f, MovementComponent->Transform.GetLocation(), target->GetActorLocation(), building->GetActorLocation(), true, targetValue, buildingValue);

			if (magnitude > 0.0f)
				target = building;
		}
	}

	AIController->AIMoveTo(target);
}

bool AAI::CanReach(AActor* Actor, float Reach, int32 Instance)
{
	FVector location = Camera->GetTargetActorLocation(Actor);

	if (Actor->IsA<AResource>()) {
		FTransform transform;
		Cast<AResource>(Actor)->ResourceHISM->GetInstanceTransform(Instance, transform);

		location = transform.GetLocation();
	}
	else if (Actor->IsA<ABuilding>())
		Cast<ABuilding>(Actor)->BuildingMesh->GetClosestPointOnCollision(MovementComponent->Transform.GetLocation(), location);

	return Reach >= FVector::Dist(MovementComponent->Transform.GetLocation(), location);
}