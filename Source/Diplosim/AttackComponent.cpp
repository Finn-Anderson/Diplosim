#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include "AI/AI.h"
#include "AI/HealthComponent.h"
#include "AI/Enemy.h"
#include "AI/Citizen.h"
#include "AI/Projectile.h"
#include "Buildings/Work.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("OverlapOnlyPawn", true);
	RangeComponent->SetSphereRadius(300.0f);

	Range = 100.0f;
	Damage = 20;
	TimeToAttack = 5.0f;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = GetOwner();

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &UAttackComponent::OnOverlapEnd);
}

void UAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (OverlappingEnemies.IsEmpty() || GetWorld()->GetTimerManager().IsTimerActive(AttackTimer))
		return;

	TArray<AActor*> targets;

	for (AActor* actor : OverlappingEnemies) {
		AAI* a = Cast<AAI>(actor);

		if (a->HealthComponent->Health == 0) {
			OverlappingEnemies.Remove(actor);

			continue;
		}

		if (!ProjectileClass && a->CanMoveTo(actor)) {
			targets.Add(actor);
		}
		else if (CanThrow(actor)) {
			targets.Add(actor);
		}
	}

	if (targets.IsEmpty()) {
		if (Owner->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			citizen->MoveTo(citizen->Employment);
		}
		else {
			AEnemy* enemy = Cast<AEnemy>(Owner);

			enemy->MoveToBroch();
		}
	}
	else {
		PickTarget(targets);
	}

	if (OverlappingEnemies.IsEmpty()) {
		GetWorld()->GetTimerManager().ClearTimer(AttackTimer);
	}
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->StaticClass() == EnemyClass) {
		AAI* a = Cast<AAI>(OtherActor);

		if (a->HealthComponent->Health == 0)
			return;

		OverlappingEnemies.Add(OtherActor);
	}
	else if (Owner->IsA<AEnemy>() && OtherActor->StaticClass() == Cast<AEnemy>(Owner)->WatchtowerClass) {
		ABuilding* b = Cast<ABuilding>(OtherActor);

		if (b->HealthComponent->Health == 0)
			return;

		OverlappingEnemies.Add(OtherActor);
	}
}

void UAttackComponent::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (!OverlappingEnemies.IsEmpty())
			return;

		if (OtherActor->IsA<AEnemy>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			citizen->MoveTo(citizen->Employment);
		}
		else {
			AEnemy* enemy = Cast<AEnemy>(Owner);

			enemy->MoveToBroch();
		}
	}
}

void UAttackComponent::PickTarget(TArray<AActor*> Targets)
{
	FAttackStruct favoured;

	for (int32 i = 0; i < Targets.Num(); i++) {
		int32 hp = 0;
		int32 dmg = 0;
		FVector::FReal outLength;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

		if (Targets[i]->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(Targets[i]);

			hp = ai->HealthComponent->Health;
			dmg = ai->AttackComponent->Damage;

			NavData->CalcPathLength(Owner->GetActorLocation(), ai->GetActorLocation(), outLength);
		}
		else {
			ABuilding* building = Cast<ABuilding>(Targets[i]);

			hp = building->HealthComponent->Health;
			dmg = 0;

			NavData->CalcPathLength(Owner->GetActorLocation(), building->GetActorLocation(), outLength);
		}

		float favourability = (Damage / hp) * dmg / outLength;
		float currentFavoured = 0;

		if (favoured.Hp > 0) {
			currentFavoured = (Damage / favoured.Hp) * favoured.Dmg / favoured.Length;
		}

		if (favourability > currentFavoured) {
			favoured.Actor = Targets[i];
			favoured.Hp = hp;
			favoured.Dmg = dmg;
			favoured.Length = outLength;
		}
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, FTimerDelegate::CreateUObject(this, &UAttackComponent::Attack, favoured.Actor, favoured.Length), TimeToAttack, false);
}

bool UAttackComponent::CanThrow(AActor* Target)
{
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(Owner);

	bool bThrowable = false;

	float targetDistance = 1000000000000;

	TArray<struct FHitResult> hits;

	UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	TArray<FName> sockets = comp->GetAllSocketNames();

	for (FName name : sockets) {
		if (name == FName("Entrance"))
			continue;

		FVector startLoc = comp->GetSocketLocation(name);

		if (GetWorld()->LineTraceMultiByChannel(hits, startLoc, Target->GetActorLocation(), ECC_Visibility, queryParams)) {
			FHitResult target = hits[hits.Num() - 1];
			FVector targetLoc = (target.GetActor()->GetActorForwardVector() * target.GetActor()->GetVelocity()) + target.Location;

			UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

			float distance = FVector::Dist(startLoc, targetLoc);
			float initialHeight = startLoc.Z;
			float initialV = projectileMovement->InitialSpeed;

			Theta = 0.5 * FMath::Asin(((projectileMovement->ProjectileGravityScale * GetWorld()->GetGravityZ()) * distance) / initialV);

			float initialVy = initialV * FMath::Sin(Theta);
			float maxHeight = initialHeight + FMath::Square(initialVy) / (2 * GetWorld()->GetGravityZ());

			bool bCollision = false;

			for (int32 i = 0; i < (hits.Num() - 1); i++) {
				if (hits[i].Location.Z < maxHeight) {
					bCollision = true;
				}
			}

			if (bCollision)
				continue;

			if (target.Distance < targetDistance) {
				ThrowLocation = target.Location;

				bThrowable = true;
			}
		}
	}

	return bThrowable;
}

void UAttackComponent::Attack(AActor* Target, FVector::FReal Length)
{
	if (Owner->IsA<AAI>()) {
		Cast<AAI>(Owner)->AIController->StopMovement();
	}

	if (ProjectileClass) {
		Throw(Target);
	}
	else {
		if (Length > Range) {
			Cast<AAI>(Owner)->MoveTo(Target);

			return;
		}

		if (Target->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(Target);

			ai->HealthComponent->TakeHealth(Damage, Owner->GetActorLocation());
		}
		else {
			ABuilding* building = Cast<ABuilding>(Target);

			building->HealthComponent->TakeHealth(Damage, Owner->GetActorLocation());
		}
	}
}

void UAttackComponent::Throw(AActor* Target)
{
	FVector lookLoc = Owner->GetActorLocation() - Target->GetActorLocation();
	FVector dir;
	float len;
	lookLoc.ToDirectionAndLength(dir, len);

	float yaw = UKismetMathLibrary::DegAtan2(dir.X, dir.Y);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, ThrowLocation, FRotator(Theta, yaw, 0));
}