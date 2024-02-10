#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Buildings/Broch.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->bDynamicObstacle = false;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetCanEverAffectNavigation(false);
	GetMesh()->SetupAttachment(RootComponent);
	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->CastShadow = true;

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BuildCapsuleCollision"));
	CapsuleCollision->SetCapsuleSize(27.0f, 27.0f);
	CapsuleCollision->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));

	AIControllerClass = ADiplosimAIController::StaticClass();

	GetCharacterMovement()->MaxWalkSpeed = 200.0f;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);

	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->bCanWalkOffLedges = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultController();

	AIController = GetController<ADiplosimAIController>();
	AIController->Owner = this;

	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &AAI::OnOverlapBegin);
	CapsuleCollision->OnComponentEndOverlap.AddDynamic(this, &AAI::OnOverlapEnd);
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

	if (!AIController->CanMoveTo(brochs[0]) && IsA<AEnemy>()) {
		TArray<AActor*> buildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), buildings);

		for (AActor* actor : buildings) {
			if (!AIController->CanMoveTo(actor))
				continue;

			if (target == brochs[0]) {
				target = actor;

				continue;
			}

			FClosestStruct closestStruct = AIController->GetClosestActor(target, actor);

			double targetDistToBroch = FVector::Dist(target->GetActorLocation(), brochs[0]->GetActorLocation()) + closestStruct.Magnitude;

			double actorDistToBroch = FVector::Dist(actor->GetActorLocation(), brochs[0]->GetActorLocation()) - closestStruct.Magnitude;

			if (targetDistToBroch > actorDistToBroch)
				target = closestStruct.Actor;
		}
	}

	AIController->AIMoveTo(target);
}

void AAI::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<AAI>() && OtherActor->StaticClass() != StaticClass())
		Meleeable.Add(Cast<AAI>(OtherActor));
}

void AAI::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Meleeable.Contains(Cast<AAI>(OtherActor)))
		Meleeable.Remove(Cast<AAI>(OtherActor));
}