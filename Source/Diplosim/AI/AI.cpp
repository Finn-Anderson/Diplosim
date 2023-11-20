#include "AI/AI.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "AIController.h"
#include "NavigationSystem.h"

#include "HealthComponent.h"
#include "Buildings/Building.h"
#include "Projectile.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->SetCapsuleRadius(11.5f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(8.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->bDynamicObstacle = false;

	AIMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AIMesh"));
	AIMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
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

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("OverlapOnlyPawn", true);
	RangeComponent->SetSphereRadius(1000.0f);

	Range = 100.0f;
	Damage = 20;
	TimeToAttack = 5.0f;
	bCanAttack = false;

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &AAI::OnEnemyOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &AAI::OnEnemyOverlapEnd);

	SpawnDefaultController();

	AIController = Cast<AAIController>(GetController());
}

void AAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!OverlappingEnemies.IsEmpty()) {
		if (!GetCanAttack()) {
			FTimerHandle attackTimer;
			GetWorld()->GetTimerManager().SetTimer(attackTimer, FTimerDelegate::CreateUObject(this, &AAI::SetCanAttack, true), TimeToAttack, false);
		}
		else {
			TArray<AActor*> targets;
			for (int32 i = 0; i < OverlappingEnemies.Num(); i++) {
				float distance = FVector::Dist(GetActorLocation(), OverlappingEnemies[i]->GetActorLocation());

				if (distance <= Range) {
					if (ProjectileClass && !CanThrow(OverlappingEnemies[i]))
						continue;

					targets.Add(OverlappingEnemies[i]);
				}
			}

			if (targets.IsEmpty()) {
				AActor* target = PickTarget(OverlappingEnemies);

				MoveTo(target);
			}
			else {
				AActor* target = PickTarget(targets);

				Attack(target);
			}
		}
	}
}

void AAI::OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}

void AAI::OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (OverlappingEnemies.IsEmpty()) {
			SetActorTickEnabled(false);

			SetCanAttack(false);

			GetWorld()->GetTimerManager().ClearTimer(DamageTimer);
		}
	}
}

AActor* AAI::GetClosestActor(AActor* Actor, AActor* CurrentLocation, AActor* NewLocation, int32 CurrentValue, int32 NewValue)
{
	if (CurrentLocation == nullptr)
		return NewLocation;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetNavDataForProps(GetNavAgentPropertiesRef());

	float outLengthCurrent;
	NavData->CalcPathLength(Actor->GetActorLocation(), CurrentLocation->GetActorLocation(), outLengthCurrent);

	outLengthCurrent /= CurrentValue;

	float outLengthNew;
	NavData->CalcPathLength(Actor->GetActorLocation(), NewLocation->GetActorLocation(), outLengthNew);

	outLengthNew /= NewValue;

	if (outLengthCurrent > outLengthNew) {
		return NewLocation;
	}
	else {
		return CurrentLocation;
	}
}

FVector AAI::CanMoveTo(AActor* Location) 
{
	if (Location->IsValidLowLevelFast()) {
		UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Location->GetComponentByClass(UStaticMeshComponent::StaticClass()));

		FVector loc = Location->GetActorLocation();

		if (comp->DoesSocketExist("Entrance")) {
			loc = comp->GetSocketLocation("Entrance");
		}

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetNavDataForProps(GetNavAgentPropertiesRef());

		FPathFindingQuery query(this, *NavData, GetActorLocation(), loc);

		bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

		if (path) {
			return loc;
		}
	}

	return FVector(0.0f, 0.0f, 0.0f);
}

void AAI::MoveTo(AActor* Location)
{
	if (Location->IsValidLowLevelFast()) {
		FVector loc = CanMoveTo(Location);

		if (!loc.IsZero()) {
			AIController->MoveToLocation(loc);

			Goal = Location;
		}
	}
}

AActor* AAI::PickTarget(TArray<AActor*> Targets)
{
	FAttackStruct favoured;

	for (int32 i = 0; i < Targets.Num(); i++) {
		int32 hp = 0;
		int32 dmg = 0;

		if (Targets[i]->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(Targets[i]);

			hp = ai->HealthComponent->Health;
			dmg = ai->Damage;
		}
		else {
			ABuilding* building = Cast<ABuilding>(Targets[i]);

			hp = building->HealthComponent->Health;
			dmg = 0;
		}

		float favourability = (Damage / hp) * dmg;
		float currentFavoured = 0;

		if (favoured.Hp > 0) {
			currentFavoured = (Damage / favoured.Hp) * favoured.Dmg;
		}

		if (favourability > currentFavoured) {
			favoured.Actor = Targets[i];
			favoured.Hp = hp;
			favoured.Dmg = dmg;
		}
	}

	return favoured.Actor;
}

void AAI::Attack(AActor* Target)
{
	AIController->StopMovement();

	if (ProjectileClass) {
		Throw(Target);
	}
	else {
		if (Target->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(Target);

			ai->HealthComponent->TakeHealth(Damage, GetActorLocation());
		}
		else {
			ABuilding* building = Cast<ABuilding>(Target);

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

	if (GetWorld()->LineTraceMultiByChannel(hits, startLoc, Target->GetActorLocation(), ECC_Visibility, queryParams)) {
		FHitResult target = hits[hits.Num() - 1];
		FVector targetLoc = (target.GetActor()->GetActorForwardVector() * target.GetActor()->GetVelocity()) + target.Location;

		float distance = FVector::Dist(startLoc, targetLoc);
		float initialHeight = startLoc.Z;
		float initialV = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent->InitialSpeed;

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
	FVector lookLoc = GetActorLocation() - Target->GetActorLocation();
	FVector dir;
	float len;
	lookLoc.ToDirectionAndLength(dir, len);

	float yaw = UKismetMathLibrary::DegAtan2(dir.X, dir.Y);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, AIMesh->GetSocketLocation("Throw"), FRotator(Theta, yaw, 0));
}

void AAI::SetCanAttack(bool Value)
{
	bCanAttack = Value;
}

bool AAI::GetCanAttack()
{
	return bCanAttack;
}