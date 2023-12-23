#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include "AI/AI.h"
#include "HealthComponent.h"
#include "AI/Enemy.h"
#include "AI/Citizen.h"
#include "AI/Projectile.h"
#include "Buildings/Broch.h"
#include "Buildings/Wall.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(300.0f);

	Damage = 20;
	TimeToAttack = 2.0f;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = GetOwner();

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &UAttackComponent::OnOverlapEnd);
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	for (TSubclassOf<AActor> enemyClass : EnemyClasses) {
		if (OtherActor->GetClass() != enemyClass)
			continue;

		UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

		if (healthComp->Health == 0)
			return;

		OverlappingEnemies.Add(OtherActor);

		GetTargets();

		break;
	}

	if (!Owner->IsA<ACitizen>() || !OtherActor->IsA<ACitizen>())
		return;

	UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	OverlappingAllies.Add(Cast<ACitizen>(OtherActor));
}

void UAttackComponent::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (!OverlappingEnemies.IsEmpty())
			return;

		GetWorld()->GetTimerManager().ClearTimer(AttackTimer);

		if (Owner->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			if (citizen->Building.Employment == nullptr) {
				TArray<AActor*> brochs;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

				citizen->MoveTo(brochs[0]);
			}
			else {
				citizen->MoveTo(citizen->Building.Employment);
			}
		}
		else if (Owner->IsA<AEnemy>()) {
			AEnemy* enemy = Cast<AEnemy>(Owner);

			enemy->MoveToBroch();
		}
	} 
	else if (OverlappingAllies.Contains(Cast<ACitizen>(OtherActor))) {
		OverlappingAllies.Remove(Cast<ACitizen>(OtherActor));
	}
}

void UAttackComponent::SetProjectileClass(TSubclassOf<AProjectile> OtherClass)
{
	if (OtherClass == ProjectileClass)
		return;

	ProjectileClass = OtherClass;
}

int32 UAttackComponent::GetMorale()
{
	float allyHP = 0.0f;
	float allyDmg = 0.0f;

	for (ACitizen* citizen : OverlappingAllies) {
		allyHP += citizen->HealthComponent->Health;

		if (*citizen->AttackComponent->ProjectileClass) {
			allyDmg += Cast<AProjectile>(citizen->AttackComponent->ProjectileClass->GetDefaultObject())->Damage;
		}
		else {
			allyDmg += citizen->AttackComponent->Damage;
		}
	}

	float enemyHP = 0.0f;
	float enemyDmg = 0.0f;

	for (AActor* actor : OverlappingEnemies) {
		AEnemy* enemy = Cast<AEnemy>(actor);

		allyHP += enemy->HealthComponent->Health;

		if (*enemy->AttackComponent->ProjectileClass) {
			allyDmg += Cast<AProjectile>(enemy->AttackComponent->ProjectileClass->GetDefaultObject())->Damage;
		}
		else {
			allyDmg += enemy->AttackComponent->Damage;
		}
	}

	float allyTotal = allyHP * allyDmg;
	float enemyTotal = enemyHP * enemyDmg;

	if (enemyTotal == 0.0f)
		return 1.0f;

	float morale = allyTotal / enemyTotal;

	return morale;
}

void UAttackComponent::GetTargets()
{
	if (OverlappingEnemies.IsEmpty() || !CanAttack())
		return;

	TArray<AActor*> targets;

	for (int32 i = (OverlappingEnemies.Num() - 1); i > -1; i--) {
		AActor* actor = OverlappingEnemies[i];

		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		if (healthComp->Health == 0) {
			OverlappingEnemies.Remove(actor);

			continue;
		}

		if (*ProjectileClass || (Owner->IsA<AAI>() && Cast<AAI>(Owner)->CanMoveTo(actor))) {
			targets.Add(actor);
		}
	}

	float morale = 1.0f;

	if (Owner->IsA<ACitizen>()) {
		morale = GetMorale();
	}

	if (targets.IsEmpty() || morale < 1.0f) {
		if (Owner->IsA<ACitizen>() && Cast<ACitizen>(Owner)->Building.Employment != nullptr) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			citizen->MoveTo(citizen->Building.Employment);
		}
		else {
			AAI* ai = Cast<AAI>(Owner);

			ai->MoveToBroch();
		}
	}
	else {
		PickTarget(targets);
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

		UHealthComponent* healthComp = Targets[i]->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = Targets[i]->GetComponentByClass<UAttackComponent>();

		hp = healthComp->Health;

		if (attackComp) {
			if (*attackComp->ProjectileClass) {
				dmg = Cast<AProjectile>(attackComp->ProjectileClass->GetDefaultObject())->Damage;
			}
			else {
				dmg = attackComp->Damage;
			}
		}

		NavData->CalcPathLength(Owner->GetActorLocation(), Targets[i]->GetActorLocation(), outLength);

		float favourability = (((float)Damage / (float)hp) * (float)dmg) / outLength;
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

	if (*ProjectileClass || CanHit(favoured.Actor, favoured.Length)) {
		if (Owner->IsA<AAI>()) {
			Cast<AAI>(Owner)->AIController->StopMovement();
		}

		Attack(favoured.Actor);
	}
}

bool UAttackComponent::CanHit(AActor* Target, FVector::FReal Length)
{
	USkeletalMeshComponent* comp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	if (Length > comp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Length() + 20.0f) {
		Cast<AAI>(Owner)->MoveTo(Target);

		GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::GetTargets, 0.1, false);

		return false;
	}

	return true;
}

void UAttackComponent::Attack(AActor* Target)
{
	if (*ProjectileClass) {
		Throw(Target);
	}
	else {
		UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

		healthComp->TakeHealth(Damage, Owner);
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::GetTargets, TimeToAttack, true);
}

void UAttackComponent::Throw(AActor* Target)
{
	USkeletalMeshComponent* comp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	FVector startLoc = Owner->GetActorLocation() + comp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z;

	FVector targetLoc = Target->GetActorLocation();

	FRotator lookAt = UKismetMathLibrary::FindLookAtRotation(startLoc, Target->GetActorLocation());

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	double angle = 0.0f;
	double d = 0.0f;

	FHitResult hit;

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(hit, startLoc, targetLoc, ECollisionChannel::ECC_PhysicsBody, queryParams)) {
		if (hit.GetActor()->IsA<AEnemy>()) {
			d = FVector::Dist(startLoc, targetLoc);

			angle = 0.5 * FMath::Asin(g * d / FMath::Square(v)) * (180.0f / PI) + lookAt.Pitch;
		}
		else {
			FVector groundedLocation = FVector(startLoc.X, startLoc.Y, Target->GetActorLocation().Z);
			d = FVector::Dist(groundedLocation, targetLoc);

			double h = startLoc.Z - Target->GetActorLocation().Z;

			double phi = FMath::Atan(d / h);

			angle = (FMath::Acos(((g * FMath::Square(d)) / FMath::Square(v) - h) / FMath::Sqrt(FMath::Square(h) + FMath::Square(d))) + phi) / 2 * (180.0f / PI);
		}
	}

	FRotator ang = FRotator(angle, lookAt.Yaw, lookAt.Roll);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->Owner = Owner;
}

bool UAttackComponent::CanAttack()
{
	UHealthComponent* healthComp = Owner->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return false;

	if (Owner->IsA<ACitizen>() && Cast<ACitizen>(Owner)->BioStruct.Age < 18) {
		ACitizen* citizen = Cast<ACitizen>(Owner);

		if (citizen->Building.BuildingAt == nullptr || !citizen->Building.BuildingAt->IsA<ABroch>()) {
			citizen->MoveToBroch();
		}

		return false;
	}

	return true;
}