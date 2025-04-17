#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

#include "Universal/HealthComponent.h"
#include "AIMovementComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Buildings/Misc/Broch.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->bDynamicObstacle = false;

	RootComponent = Capsule;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);

	MovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

	bUseControllerRotationYaw = false;

	AIControllerClass = ADiplosimAIController::StaticClass();

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultController();

	AIController = GetController<ADiplosimAIController>();
	AIController->Owner = this;
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

			double magnitude = AIController->GetClosestActor(400.0f, GetActorLocation(), target->GetActorLocation(), actor->GetActorLocation());

			double targetDistToBroch = FVector::Dist(target->GetActorLocation(), brochs[0]->GetActorLocation()) + magnitude;

			double actorDistToBroch = FVector::Dist(actor->GetActorLocation(), brochs[0]->GetActorLocation()) - magnitude;

			if (targetDistToBroch > actorDistToBroch)
				target = actor;
		}
	}

	AIController->AIMoveTo(target);
}