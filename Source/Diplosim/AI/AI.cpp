#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

#include "HealthComponent.h"
#include "AttackComponent.h"
#include "Buildings/Building.h"
#include "Projectile.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->SetCapsuleRadius(11.5f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(11.5f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->bDynamicObstacle = false;

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -11.5f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetCanEverAffectNavigation(false);
	GetMesh()->SetupAttachment(RootComponent);
	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->CastShadow = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100;

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultController();

	AIController = Cast<AAIController>(GetController());
}

AActor* AAI::GetClosestActor(AActor* Actor, AActor* CurrentLocation, AActor* NewLocation, int32 CurrentValue, int32 NewValue)
{
	if (CurrentLocation == nullptr)
		return NewLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	FVector::FReal outLengthCurrent;
	NavData->CalcPathLength(Actor->GetActorLocation(), CurrentLocation->GetActorLocation(), outLengthCurrent);

	if (CurrentValue > 1) {
		outLengthCurrent /= CurrentValue;
	}	

	FVector::FReal outLengthNew;
	NavData->CalcPathLength(Actor->GetActorLocation(), NewLocation->GetActorLocation(), outLengthNew);

	if (NewValue > 1) {
		outLengthNew /= NewValue;
	}

	if (outLengthCurrent > outLengthNew) {
		return NewLocation;
	}
	else {
		return CurrentLocation;
	}
}

bool AAI::CanMoveTo(AActor* Location) 
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Location->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	FVector loc = Location->GetActorLocation();

	if (comp && comp->DoesSocketExist("Entrance")) {
		loc = comp->GetSocketLocation("Entrance");
	}

	FPathFindingQuery query(this, *NavData, GetActorLocation(), loc);

	bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

	if (path) {
		return true;
	}

	return false;
}

void AAI::MoveTo(AActor* Location)
{
	if (Location->IsValidLowLevelFast() && CanMoveTo(Location)) {
		UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Location->GetComponentByClass(UStaticMeshComponent::StaticClass()));

		if (comp && comp->DoesSocketExist("Entrance")) {
			AIController->MoveToLocation(comp->GetSocketLocation("Entrance"));
		}
		else {
			AIController->MoveToLocation(Location->GetActorLocation());
		}

		Goal = Location;
	}
}