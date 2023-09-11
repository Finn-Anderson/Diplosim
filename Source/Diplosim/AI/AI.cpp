#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

#include "HealthComponent.h"
#include "Citizen.h"
#include "Buildings/Work.h"
#include "Projectile.h"

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

	Range = 100.0f;
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
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (OverlappingEnemies.IsEmpty()) {
			GetWorld()->GetTimerManager().ClearTimer(DamageTimer);
		}
	}
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

			if (path && s != "Throw") {
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

void AAI::Attack()
{
	FAttackStruct favoured;

	for (int32 i = 0; i < OverlappingEnemies.Num(); i++) {
		int32 hp = 0;
		int32 dmg = 0;

		if (OverlappingEnemies[i]->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(OverlappingEnemies[i]);

			hp = ai->HealthComponent->Health;
			dmg = ai->Damage;
		}
		else {
			ABuilding* building = Cast<ABuilding>(OverlappingEnemies[i]);

			hp = building->HealthComponent->Health;
			dmg = 0;
		}

		float favourability = (Damage / hp) * dmg;
		float currentFavoured = 0; 
		
		if (favoured.Hp > 0) {
			currentFavoured = (Damage / favoured.Hp) * favoured.Dmg;
		}

		if (favourability > currentFavoured) {
			favoured.Actor = OverlappingEnemies[i];
			favoured.Hp = hp;
			favoured.Dmg = dmg;
		}
	}

	if (ProjectileClass) {
		Throw(favoured.Actor);
	}
	else {
		if (favoured.Actor->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(favoured.Actor);

			ai->HealthComponent->TakeHealth(Damage, GetActorLocation());
		}
		else {
			ABuilding* building = Cast<ABuilding>(favoured.Actor);

			building->HealthComponent->TakeHealth(Damage, GetActorLocation());
		}
	}
}

bool AAI::CanThrow(AActor* Target)
{
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	TArray<struct FHitResult> hits;

	FVector startLoc = AIMesh->GetSocketLocation("Throw");

	if (GetWorld()->LineTraceMultiByChannel(hits, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, queryParams)) {
		FHitResult target = hits[hits.Num() - 1];
		FVector targetLoc = (target.GetActor()->GetActorForwardVector() * target.GetActor()->GetVelocity()) + target.Location;

		float distance = FVector::Dist(startLoc, targetLoc);
		float initialHeight = startLoc.Z;
		float initialV = ProjectileClass->GetDefaultObject<AProjectile>()->Speed;

		Theta = 0.5 * FMath::Asin((GetWorld()->GetGravityZ() * distance) / initialV);

		float initialVy = initialV * FMath::Sin(Theta);
		float maxHeight = initialHeight + FMath::Square(initialVy) / (2 * GetWorld()->GetGravityZ());

		for (int32 i = 0; i < (hits.Num() - 1); i++) {
			if (hits[i].Location.Z > maxHeight) {
				MoveTo(Target);
				return false;
			}
		}
	}

	return true;
}

void AAI::Throw(AActor* Target)
{
	if (CanThrow(Target)) {
		GetWorld()->SpawnActor<ACitizen>(ProjectileClass, AIMesh->GetSocketLocation("Throw"), FRotator(0, Theta, 0));
	}
}