#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include "AI.h"
#include "HealthComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Citizen.h"
#include "Projectile.h"
#include "Buildings/Broch.h"
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

	Owner = GetOwner();

	if (GetOwner()->IsA<ACitizen>()) {
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UHealthComponent* healthComp = OtherActor->GetComponentByClass<UHealthComponent>();

	if (healthComp->GetHealth() == 0)
		return;

	if (OtherActor->IsA(Owner->GetClass()) && Owner->IsA<ACitizen>())
		OverlappingAllies.Add(Cast<ACitizen>(OtherActor));

	for (TSubclassOf<AActor> enemyClass : EnemyClasses) {
		if (!OtherActor->IsA(enemyClass))
			continue;

		OverlappingEnemies.Add(OtherActor);

		break;
	}

	if (GetWorld()->GetTimerManager().IsTimerActive(AttackTimer) || OverlappingEnemies.IsEmpty())
		return;

	GetTargets();
}

void UAttackComponent::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingAllies.Contains(Cast<ACitizen>(OtherActor)))
		OverlappingAllies.Remove(Cast<ACitizen>(OtherActor));

	if (OverlappingEnemies.Contains(OtherActor))
		OverlappingEnemies.Remove(OtherActor);
}

void UAttackComponent::SetProjectileClass(TSubclassOf<AProjectile> OtherClass)
{
	if (OtherClass == ProjectileClass)
		return;

	ProjectileClass = OtherClass;
}

int32 UAttackComponent::GetMorale(TArray<TWeakObjectPtr<AActor>> Targets)
{
	if (Targets.IsEmpty() || !Owner->IsA<ACitizen>() || (Cast<ACitizen>(Owner)->Building.BuildingAt != nullptr && Cast<ACitizen>(Owner)->Building.BuildingAt->IsA<AWall>()))
		return 1.0f;

	if (!CanAttack())
		return -1.0f;

	TArray<TWeakObjectPtr<AActor>> allies;

	for (TWeakObjectPtr<AActor> actor : OverlappingAllies) {
		if (actor == nullptr)
			continue;

		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

		if (healthComp->GetHealth() == 0 || actor->IsHidden())
			continue;

		allies.Add(actor);
	}

	return allies.Num() - Targets.Num();
}

void UAttackComponent::RunAway(TArray<TWeakObjectPtr<AActor>> Targets)
{
	FVector sum = FVector::Zero();

	for (TWeakObjectPtr<AActor> target : Targets)
		sum += target->GetActorLocation();

	sum /= Targets.Num();

	ACitizen* citizen = Cast<ACitizen>(Owner);

	TArray<AActor*> grids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrid::StaticClass(), grids);

	AGrid* grid = Cast<AGrid>(grids[0]);

	TArray<int32> instances = grid->HISMGround->GetInstancesOverlappingSphere(citizen->GetActorLocation(), 1500.0f);

	FVector current = FVector(100000000.0f);
	int32 currentInst;

	for (int32 inst : instances) {
		FTransform transform;
		grid->HISMGround->GetInstanceTransform(inst, transform);

		if (!citizen->AIController->CanMoveTo(transform.GetLocation()))
			continue;

		double currentDist = FVector::Dist(sum, current);
		double newDist = FVector::Dist(sum, transform.GetLocation());

		if (currentDist > newDist)
			continue;

		current = transform.GetLocation();
		currentInst = inst;

	}

	citizen->AIController->AIMoveTo(grid, current);
}

void UAttackComponent::GetTargets()
{
	TArray<TWeakObjectPtr<AActor>> targets;

	for (TWeakObjectPtr<AActor> actor : OverlappingEnemies) {
		if (actor == nullptr)
			continue;

		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

		if (healthComp->GetHealth() == 0 || actor->IsHidden())
			continue;

		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		if (*ProjectileClass || (Owner->IsA<AAI>() && Cast<AAI>(Owner)->AIController->CanMoveTo(Cast<AActor>(actor)->GetActorLocation())))
			targets.Add(actor);
	}

	float morale = GetMorale(targets);

	if (targets.IsEmpty()) {
		if (Owner->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(Owner);

			if (Cast<ACitizen>(Owner)->Building.Employment != nullptr)
				citizen->AIController->AIMoveTo(citizen->Building.Employment);
			else
				citizen->AIController->Idle();
		}
		else {
			AAI* ai = Cast<AAI>(Owner);

			ai->MoveToBroch();
		}
	}
	else if (morale < 0.0f)
		RunAway(targets);
	else
		PickTarget(targets);
}

void UAttackComponent::PickTarget(TArray<TWeakObjectPtr<AActor>> Targets)
{
	for (TWeakObjectPtr<AActor> target : Targets) {
		UHealthComponent* healthComp = target->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = target->GetComponentByClass<UAttackComponent>();

		FThreatsStruct threatStruct;

		if (target->IsA<ACitizen>())
			threatStruct.Citizen = Cast<ACitizen>(target);

		if (!Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats.IsEmpty() && Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats.Contains(threatStruct)) {
			Favoured.Actor = threatStruct.Citizen->Building.Employment;

			break;
		}

		if (Favoured.Actor == target)
			continue;

		if (Favoured.Actor == nullptr)
			Favoured.Reset();
		else {
			UHealthComponent* favouredHealth = Favoured.Actor->GetComponentByClass<UHealthComponent>();

			Favoured.Hp = favouredHealth->GetHealth();

			if (Favoured.Hp == 0)
				Favoured.Reset();
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

		if (Favoured.Actor != nullptr)
			NavData->CalcPathLength(Owner->GetActorLocation(), Favoured.Actor->GetActorLocation(), outLength);
		else
			outLength = 1000000.0f;

		double currentFavoured = outLength * Favoured.Hp / Favoured.Dmg;

		if (favourability < currentFavoured) {
			Favoured.Actor = target;
			Favoured.Dmg = dmg;
			Favoured.Hp = hp;
		}
	}

	if ((*ProjectileClass || CanHit(Cast<AActor>(Favoured.Actor))) && !GetWorld()->GetTimerManager().IsTimerActive(AttackTimer))
		Attack(Cast<AActor>(Favoured.Actor));
	else
		GetWorld()->GetTimerManager().SetTimer(MoveTimer, this, &UAttackComponent::GetTargets, 0.1, false);
}

bool UAttackComponent::CanHit(AActor* Target)
{
	if (!Target->IsValidLowLevelFast())
		return false;

	double length = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());

	if (length < 50.0f)
		return true;

	Cast<AAI>(Owner)->AIController->AIMoveTo(Target);

	return false;
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

		GetWorld()->GetTimerManager().SetTimer(MoveTimer, this, &UAttackComponent::GetTargets, 0.1, false);
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::GetTargets, TimeToAttack, false);
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

bool UAttackComponent::CanAttack()
{
	UHealthComponent* healthComp = Owner->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0 || (Owner->IsA<ACitizen>() && Cast<ACitizen>(Owner)->BioStruct.Age < 18))
		return false;

	return true;
}