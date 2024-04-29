#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include "AI.h"
#include "HealthComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Citizen.h"
#include "Projectile.h"
#include "Buildings/Wall.h"
#include "DiplosimGameModeBase.h"
#include "Map/Grid.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(400.0f);

	RangeComponent->bDynamicObstacle = true;
	RangeComponent->SetCanEverAffectNavigation(false);

	Damage = 10;
	TimeToAttack = 2.0f;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AAI>(GetOwner());

	if (GetOwner()->IsA<ACitizen>()) {
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

	if (healthComp->GetHealth() == 0 || !CanAttack() || (!*ProjectileClass && !Owner->AIController->CanMoveTo(OtherActor->GetActorLocation())))
		return;

	for (TSubclassOf<AActor> enemyClass : EnemyClasses) {
		if (!OtherActor->IsA(enemyClass))
			continue;

		OverlappingEnemies.Add(OtherActor);

		break;
	}

	if (GetWorld()->GetTimerManager().IsTimerActive(AttackTimer) || OverlappingEnemies.IsEmpty())
		return;

	PickTarget();
}

void UAttackComponent::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor))
		OverlappingEnemies.Remove(OtherActor);
}

void UAttackComponent::SetProjectileClass(TSubclassOf<AProjectile> OtherClass)
{
	if (OtherClass == ProjectileClass)
		return;

	ProjectileClass = OtherClass;
}

void UAttackComponent::PickTarget()
{
	FAttackStruct favoured;

	for (TWeakObjectPtr<AActor> target : OverlappingEnemies) {
		if (!target->IsValidLowLevelFast()) {
			OverlappingEnemies.Remove(target);

			continue;
		}

		UHealthComponent* healthComp = target->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = target->GetComponentByClass<UAttackComponent>();

		FThreatsStruct threatStruct;

		if (target->IsA<ACitizen>())
			threatStruct.Citizen = Cast<ACitizen>(target);

		TArray<FThreatsStruct> threats = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats;

		if (!threats.IsEmpty() && threats.Contains(threatStruct)) {
			if (threatStruct.Citizen->AttackComponent->RangeComponent->CanEverAffectNavigation() == true)
				continue;

			favoured.Actor = threatStruct.Citizen->Building.Employment;

			break;
		}

		float hp = 0.0f;
		float dmg = 0.0f;
		double outLength;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

		hp = healthComp->Health;

		if (attackComp) {
			if (*attackComp->ProjectileClass) {
				dmg = Cast<AProjectile>(attackComp->ProjectileClass->GetDefaultObject())->Damage;
			}
			else {
				dmg = attackComp->Damage;
			}
		}

		if (target->IsA<ACitizen>() && Cast<ACitizen>(target)->BioStruct.Age < 18)
			dmg = 0.0f;

		if (target->IsA<AWall>())
			dmg = Cast<AProjectile>(Cast<AWall>(target)->BuildingProjectileClass->GetDefaultObject())->Damage;

		NavData->CalcPathLength(Owner->GetActorLocation(), target->GetActorLocation(), outLength);

		double favourability = outLength * hp / dmg;

		if (favoured.Actor != nullptr)
			NavData->CalcPathLength(Owner->GetActorLocation(), favoured.Actor->GetActorLocation(), outLength);
		else
			outLength = 1000000.0f;

		double currentFavoured = outLength * favoured.Hp / favoured.Dmg;

		if (favourability < currentFavoured) {
			favoured.Actor = Cast<AActor>(target);
			favoured.Dmg = dmg;
			favoured.Hp = hp;
		}
	}

	if (!CanAttack())
		return;

	if (favoured.Actor == nullptr)
		Owner->MoveToBroch();

	if (*ProjectileClass || MeleeableEnemies.Contains(favoured.Actor))
		Attack(favoured.Actor);
	else
		Owner->AIController->AIMoveTo(favoured.Actor);
}

void UAttackComponent::CanHit(AActor* Target)
{
	MeleeableEnemies.Add(Target);

	PickTarget();
}

bool UAttackComponent::CanAttack()
{
	UHealthComponent* healthComp = Owner->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0 || (Owner->IsA<ACitizen>() && Cast<ACitizen>(Owner)->BioStruct.Age < 18))
		return false;

	return true;
}

void UAttackComponent::Attack(AActor* Target)
{
	Cast<AAI>(Owner)->AIController->StopMovement();

	if (*ProjectileClass) {
		Throw(Target);
	}
	else {
		UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

		healthComp->TakeHealth(Damage, Owner);
	}

	float time = TimeToAttack;

	if (Owner->IsA<ACitizen>())
		time *= FMath::LogX(100.0f, FMath::Clamp(Cast<ACitizen>(Owner)->Energy, 2, 100));

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::PickTarget, time, false);
}

void UAttackComponent::Throw(AActor* Target)
{
	USkeletalMeshComponent* ownerComp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	USkeletalMeshComponent* targetComp = Cast<USkeletalMeshComponent>(Target->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	FVector startLoc = Owner->GetActorLocation() + FVector(0.0f, 0.0f, ownerComp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z) + GetOwner()->GetActorForwardVector(); 

	FVector targetLoc = Target->GetActorLocation();
	targetLoc += Target->GetVelocity() * (FVector::Dist(startLoc, targetLoc) / v);

	FRotator lookAt = (targetLoc - startLoc).Rotation();

	double angle = 0.0f;
	double d = 0.0f;

	FHitResult hit;

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(Owner);

	if (GetWorld()->LineTraceSingleByChannel(hit, startLoc, Target->GetActorLocation(), ECollisionChannel::ECC_PhysicsBody, queryParams)) {
		if (hit.GetActor()->IsA<AEnemy>()) {
			d = FVector::Dist(startLoc, targetLoc);

			angle = 0.5f * FMath::Asin((g * FMath::Cos((45.0f + lookAt.Pitch) * (PI / 180.0f)) * d) / FMath::Square(v)) * (180.0f / PI) + lookAt.Pitch;
		}
		else {
			FVector groundedLocation = FVector(startLoc.X, startLoc.Y, targetLoc.Z);
			d = FVector::Dist(groundedLocation, targetLoc);

			double h = startLoc.Z - targetLoc.Z;

			double phi = FMath::Atan(d / h);

			angle = (FMath::Acos(((g * FMath::Square(d)) / FMath::Square(v) - h) / FMath::Sqrt(FMath::Square(h) + FMath::Square(d))) + phi) / 2 * (180.0f / PI);
		}
	}

	FRotator ang = FRotator(angle, lookAt.Yaw, lookAt.Roll);

	Owner->SetActorRotation(ang);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->Owner = Owner;
}