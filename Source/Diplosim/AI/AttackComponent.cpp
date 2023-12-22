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
		if (OtherActor->GetClass() != enemyClass || (OtherActor->IsA<ABuilding>() && Cast<ABuilding>(OtherActor)->BuildStatus != EBuildStatus::Complete))
			continue;

		UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

		if (healthComp->Health == 0)
			return;

		OverlappingEnemies.Add(OtherActor);

		GetTargets();

		break;
	}
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
}

void UAttackComponent::SetProjectileClass(TSubclassOf<AProjectile> OtherClass)
{
	if (OtherClass == ProjectileClass)
		return;

	ProjectileClass = OtherClass;
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

		if (!attackComp->CanAttack())
			continue;

		if (*ProjectileClass || (Owner->IsA<AAI>() && Cast<AAI>(Owner)->CanMoveTo(actor))) {
			targets.Add(actor);
		}
	}

	if (targets.IsEmpty()) {
		if (Owner->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			citizen->MoveTo(citizen->Building.Employment);
		}
		else {
			AEnemy* enemy = Cast<AEnemy>(Owner);

			enemy->MoveToBroch();
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

		healthComp->TakeHealth(Damage, Owner->GetActorLocation());
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::GetTargets, TimeToAttack, true);
}

void UAttackComponent::Throw(AActor* Target)
{
	USkeletalMeshComponent* comp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	FVector startLoc = Owner->GetActorLocation() + comp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z;

	double initialHeight = startLoc.Z - Target->GetActorLocation().Z;

	FVector groundedLocation = FVector(startLoc.X, startLoc.Y, Target->GetActorLocation().Z);
	double distance = FVector::Dist(groundedLocation, Target->GetActorLocation());

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	double angle = FMath::Abs(0.5f * FMath::Asin((GetWorld()->GetGravityZ() * distance) / FMath::Square(projectileMovement->InitialSpeed))) * 90.0f;

	FRotator lookAt = UKismetMathLibrary::FindLookAtRotation(startLoc, Target->GetActorLocation());
	FRotator ang = FRotator(angle + lookAt.Pitch, lookAt.Yaw, lookAt.Roll);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->Owner = Owner;
}

bool UAttackComponent::CanAttack()
{
	UHealthComponent* healthComp = Owner->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return false;

	if (Owner->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(Owner);

		if (citizen->BioStruct.Age < 18 || (citizen->Building.BuildingAt != nullptr && !citizen->Building.BuildingAt->IsA<ABroch>() && !citizen->Building.BuildingAt->IsA<AWall>())) {
			return false;
		}
	}

	return true;
}