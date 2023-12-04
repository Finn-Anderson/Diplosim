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
#include "Buildings/Watchtower.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(300.0f);

	Damage = 20;
	TimeToAttack = 2.0f;

	bCanAttack = true;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = GetOwner();

	if (*ProjectileClass) {
		Damage = Cast<AProjectile>(ProjectileClass->GetDefaultObject())->Damage;
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &UAttackComponent::OnOverlapEnd);
}

void UAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (OverlappingEnemies.IsEmpty() || GetWorld()->GetTimerManager().IsTimerActive(AttackTimer) || !bCanAttack)
		return;

	TArray<AActor*> targets;

	for (AActor* actor : OverlappingEnemies) {
		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		if (healthComp->Health == 0) {
			OverlappingEnemies.Remove(actor);

			continue;
		}

		if (!attackComp->bCanAttack)
			continue;

		TArray<FVector> locations;

		if (Owner->IsA<AAI>()) {
			USkeletalMeshComponent* comp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

			FVector loc = Owner->GetActorLocation() + comp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z;

			locations.Add(loc);
		}
		else {
			UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Owner->GetComponentByClass(UStaticMeshComponent::StaticClass()));

			TArray<FName> sockets = comp->GetAllSocketNames();
			sockets.Remove("Entrance");

			for (FName socket : sockets) {
				locations.Add(comp->GetSocketLocation(socket));
			}
		}

		if ((*ProjectileClass && CanThrow(actor, locations)) || (Owner->IsA<AAI>() && Cast<AAI>(Owner)->CanMoveTo(actor))) {
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
	for (TSubclassOf<AActor> enemyClass : EnemyClasses) {
		if (OtherActor->GetClass() != enemyClass || (OtherActor->IsA<ABuilding>() && Cast<ABuilding>(OtherActor)->BuildStatus == EBuildStatus::Blueprint))
			continue;

		UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

		if (healthComp->Health == 0)
			return;

		OverlappingEnemies.Add(OtherActor);

		SetComponentTickEnabled(true);

		break;
	}
}

void UAttackComponent::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (!OverlappingEnemies.IsEmpty())
			return;

		SetComponentTickEnabled(false);

		if (Owner->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			citizen->MoveTo(citizen->Employment);
		}
		else if (Owner->IsA<AEnemy>()) {
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

		UHealthComponent* healthComp = Targets[i]->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = Targets[i]->GetComponentByClass<UAttackComponent>();

		hp = healthComp->Health;

		if (attackComp) {
			dmg = attackComp->Damage;
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

		GetWorld()->GetTimerManager().SetTimer(AttackTimer, FTimerDelegate::CreateUObject(this, &UAttackComponent::Attack, favoured.Actor), TimeToAttack, false);
	}
}

bool UAttackComponent::CanHit(AActor* Target, FVector::FReal Length)
{
	USkeletalMeshComponent* comp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	if (Length > comp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Length() + 20.0f) {
		Cast<AAI>(Owner)->MoveTo(Target);

		return false;
	}

	return true;
}

bool UAttackComponent::CanThrow(AActor* Target, TArray<FVector> Locations)
{
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(Owner);

	bool bThrowable = false;

	float targetDistance = 1000000000000;

	TArray<struct FHitResult> hits;

	for (FVector startLoc : Locations) {
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

void UAttackComponent::Attack(AActor* Target)
{
	if (*ProjectileClass) {
		Throw(Target);
	}
	else {
		UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

		healthComp->TakeHealth(Damage, Owner->GetActorLocation());
	}
}

void UAttackComponent::Throw(AActor* Target)
{
	FRotator lookAt = (ThrowLocation - Target->GetActorLocation()).Rotation();

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, ThrowLocation, lookAt);
	projectile->Owner = Owner;
}