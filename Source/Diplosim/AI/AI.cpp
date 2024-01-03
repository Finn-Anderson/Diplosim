#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
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
	GetMesh()->SetCanEverAffectNavigation(false);
	GetMesh()->SetupAttachment(RootComponent);
	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->CastShadow = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);

	AIControllerClass = ADiplosimAIController::StaticClass();

	GetCharacterMovement()->MaxWalkSpeed = 200.0f;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);

	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->bCanWalkOffLedges = false;
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

	AIController->AIMoveTo(brochs[0]);
}