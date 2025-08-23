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
#include "Map/Grid.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));

	MovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

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
	if (!IsValidLowLevelFast())
		return;

	TArray<AActor*> brochs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

	if (brochs.IsEmpty() || HealthComponent->Health == 0)
		return;

	AActor* target = brochs[0];

	UHealthComponent* healthComp = target->GetComponentByClass<UHealthComponent>();

	if (!healthComp || healthComp->GetHealth() <= 0)
		return;

	if (!AIController->CanMoveTo(brochs[0]->GetActorLocation()) && IsA<AEnemy>()) {
		TArray<AActor*> buildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), buildings);

		for (AActor* actor : buildings) {
			if (!AIController->CanMoveTo(actor->GetActorLocation()))
				continue;

			if (target == brochs[0]) {
				target = actor;

				continue;
			}

			double magnitude = AIController->GetClosestActor(400.0f, MovementComponent->Transform.GetLocation(), target->GetActorLocation(), actor->GetActorLocation());

			double targetDistToBroch = FVector::Dist(target->GetActorLocation(), brochs[0]->GetActorLocation()) + magnitude;

			double actorDistToBroch = FVector::Dist(actor->GetActorLocation(), brochs[0]->GetActorLocation()) - magnitude;

			if (targetDistToBroch > actorDistToBroch)
				target = actor;
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