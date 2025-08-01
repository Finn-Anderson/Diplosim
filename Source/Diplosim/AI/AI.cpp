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
	Mesh->PrimaryComponentTick.bCanEverTick = false;

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

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
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

bool AAI::CanReach(AActor* Actor, float Reach, int32 Instance)
{
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	params.AddIgnoredActor(Camera->Grid);
	
	TArray<FHitResult> hits;

	if (GetWorld()->SweepMultiByChannel(hits, GetActorLocation(), GetActorLocation(), GetActorQuat(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeSphere(Reach), params)) {
		for (FHitResult hit : hits) {
			if (hit.GetActor() != Actor || (Instance > 0 && hit.Item != Instance) || (!hit.GetComponent()->IsA<UStaticMeshComponent>() && !hit.GetComponent()->IsA<USkeletalMeshComponent>() && !hit.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>()))
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