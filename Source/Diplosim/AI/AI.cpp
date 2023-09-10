#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

#include "HealthComponent.h"
#include "Citizen.h"
#include "Buildings/Work.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->bDynamicObstacle = false;

	AIMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	AIMesh->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	AIMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	AIMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	AIMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AIMesh->SetCanEverAffectNavigation(false);
	AIMesh->SetupAttachment(RootComponent);
	AIMesh->bCastDynamicShadow = true;
	AIMesh->CastShadow = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Range = 10.0f;
	Damage = 20;
	TimeToAttack = 5.0f;

	AttackComponent = CreateDefaultSubobject<USphereComponent>(TEXT("AttackComponent"));
	AttackComponent->SetSphereRadius(Range);

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	AIMesh->OnComponentBeginOverlap.AddDynamic(this, &AAI::OnOverlapBegin);
	AIMesh->OnComponentEndOverlap.AddDynamic(this, &AAI::OnOverlapEnd);

	AttackComponent->OnComponentBeginOverlap.AddDynamic(this, &AAI::OnEnemyOverlapBegin);
	AttackComponent->OnComponentEndOverlap.AddDynamic(this, &AAI::OnEnemyOverlapEnd);

	SpawnDefaultController();

	AIController = Cast<AAIController>(GetController());
}

void AAI::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void AAI::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AAI::OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void AAI::OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AAI::MoveTo(AActor* Location)
{
	if (Location->IsValidLowLevelFast()) {
		UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Location->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		TArray<FName> sockets = comp->GetAllSocketNames();

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetNavDataForProps(GetNavAgentPropertiesRef());

		FName socket = "";
		for (int32 i = 0; i < sockets.Num(); i++) {
			FName s = sockets[i];

			FPathFindingQuery query(this, *NavData, GetActorLocation(), comp->GetSocketLocation(s));
			query.bAllowPartialPaths = false;

			bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

			if (path) {
				if (socket == "") {
					socket = s;
				}
				else {
					float curSocketLoc = FVector::Dist(comp->GetSocketLocation(socket), GetActorLocation());

					float newSocketLoc = FVector::Dist(comp->GetSocketLocation(s), GetActorLocation());

					if (curSocketLoc > newSocketLoc) {
						socket = s;
					}
				}
			}
		}

		AIController->MoveToLocation(comp->GetSocketLocation(socket));

		Goal = Location;
	}
}

void AAI::Throw(AActor* Target)
{
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	TArray<struct FHitResult> hits;

	if (GetWorld()->LineTraceMultiByChannel(hits, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, queryParams)) {
		int32 Distance = hits[hits.Num() - 1].Distance;

		for (int32 i = 0; i < hits.Num(); i++) {
			// Projectile Motion Calculation
		}
	}
}