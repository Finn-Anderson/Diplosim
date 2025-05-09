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

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCanEverAffectNavigation(false);
	Capsule->bDynamicObstacle = false;
	Capsule->PrimaryComponentTick.bCanEverTick = false;

	RootComponent = Capsule;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));

	MovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(RootComponent);

	bUseControllerRotationYaw = false;

	AIControllerClass = ADiplosimAIController::StaticClass();

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	InitialRange = 400.0f;
	Range = InitialRange;
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

			double magnitude = AIController->GetClosestActor(400.0f, GetActorLocation(), target->GetActorLocation(), actor->GetActorLocation());

			double targetDistToBroch = FVector::Dist(target->GetActorLocation(), brochs[0]->GetActorLocation()) + magnitude;

			double actorDistToBroch = FVector::Dist(actor->GetActorLocation(), brochs[0]->GetActorLocation()) - magnitude;

			if (targetDistToBroch > actorDistToBroch)
				target = actor;
		}
	}

	AIController->AIMoveTo(target);
}

bool AAI::CanReach(AActor* Actor, float Reach)
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();
	
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	params.AddIgnoredActor(camera->Grid);
	
	TArray<FHitResult> hits;

	FVector extent = Mesh->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize() / 2.0f;
	extent.X += Reach;
	extent.Y += Reach;
	extent.Z += Reach;

	FVector startRange = FVector(0.0f, 0.0f, extent.Z);

	if (GetWorld()->SweepMultiByChannel(hits, GetActorLocation() + startRange, GetActorLocation() - startRange, GetActorQuat(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeBox(extent), params)) {
		for (FHitResult hit : hits) {
			if (hit.GetActor() != Actor)
				continue;

			return true;
		}
	}

	return false;
}

void AAI::EnableCollisions(bool bEnable)
{
	ECollisionResponse response = ECollisionResponse::ECR_Ignore;

	if (bEnable)
		response = ECollisionResponse::ECR_Block;

	if (Capsule->GetCollisionResponseToChannel(ECC_Pawn) == response)
		return;

	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, response);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, response);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, response);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, response);
	Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, response);
}