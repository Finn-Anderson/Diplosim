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

	InitialRange = 400.0f;
	Range = InitialRange;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	AIController->Camera = Camera;
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
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	TArray<UPrimitiveComponent*> components = { Camera->Grid->HISMFlatGround, Camera->Grid->HISMGround, Camera->Grid->HISMLava, Camera->Grid->HISMRampGround, Camera->Grid->HISMRiver, Camera->Grid->HISMSea, Camera->Grid->HISMWall };
	params.AddIgnoredComponents(components);
	
	TArray<FHitResult> hits;

	if (GetWorld()->SweepMultiByChannel(hits, MovementComponent->Transform.GetLocation(), MovementComponent->Transform.GetLocation(), GetActorQuat(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeSphere(Reach), params)) {
		for (FHitResult hit : hits) {
			if (hit.GetActor() != Actor || (Instance > 0 && hit.Item != Instance) || (!hit.GetComponent()->IsA<UStaticMeshComponent>() && !hit.GetComponent()->IsA<USkeletalMeshComponent>() && !hit.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>()))
				continue;

			return true;
		}
	}

	return false;
}