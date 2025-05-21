#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"

#include "AI.h"
#include "Universal/HealthComponent.h"
#include "DiplosimAIController.h"
#include "Enemy.h"
#include "Citizen.h"
#include "Projectile.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Buildings/Building.h"
#include "Universal/DiplosimGameModeBase.h"
#include "AIMovementComponent.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickBatching = true;
	SetComponentTickInterval(0.1f);

	Damage = 10;

	AttackTime = 1.0f;
	AttackTimer = 0.0f;

	CurrentTarget = nullptr;

	bCanAttack = true;

	DamageMultiplier = 1.0f;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->IsA<ACitizen>() && Cast<ACitizen>(GetOwner())->BioStruct.Age < 18)
		bCanAttack = false;
}

void UAttackComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	Async(EAsyncExecution::Thread, [this]() { PickTarget(); });
}

void UAttackComponent::SetProjectileClass(TSubclassOf<AProjectile> OtherClass)
{
	if (OtherClass == ProjectileClass)
		return;

	ProjectileClass = OtherClass;
}

void UAttackComponent::PickTarget()
{
	if (!bCanAttack)
		return;

	AActor* favoured = nullptr;

	for (int32 i = OverlappingEnemies.Num() - 1; i > -1; i--) {
		AActor* target = OverlappingEnemies[i];
		FFavourabilityStruct targetFavourability = GetActorFavourability(target);

		if (targetFavourability.Hp == 0 || targetFavourability.Hp == 10000000) {
			OverlappingEnemies.RemoveAt(i);

			continue;
		}

		FThreatsStruct threatStruct;
		threatStruct.Actor = target;

		TArray<FThreatsStruct> threats = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats;

		if (!threats.IsEmpty() && threats.Contains(threatStruct)) {
			if (threatStruct.Actor->IsA<AWall>() && Cast<AWall>(threatStruct.Actor)->RangeComponent->CanEverAffectNavigation() == true)
				continue;

			favoured = Cast<AActor>(threatStruct.Actor);

			break;
		}

		FFavourabilityStruct favouredFavourability = GetActorFavourability(favoured);

		double favourability = (favouredFavourability.Hp * favouredFavourability.Dist / favouredFavourability.Dmg) - (targetFavourability.Hp * targetFavourability.Dist / targetFavourability.Dmg);

		if (favourability > 0.0f)
			favoured = target;
	}

	int32 reach = 0.0f;
	AAI* ai = nullptr;

	if (GetOwner()->IsA<AAI>()) {
		ai = Cast<AAI>(GetOwner());

		reach = ai->Range / 15.0f;

		if (favoured != nullptr)
			ai->EnableCollisions(true);
	}

	if (favoured == nullptr) {
		SetComponentTickEnabled(false);

		AttackTimer = 0.0f;

		if (IsValid(ai))
			ai->EnableCollisions(false);
	}
	else if (!*ProjectileClass && ai->CanReach(favoured, reach)) {
		ai->MovementComponent->CurrentAnim = nullptr;

		ai->AIController->StopMovement();
	}

	AsyncTask(ENamedThreads::GameThread, [this, favoured, ai, reach]() {
		if (favoured == nullptr) {
			if (IsValid(ai))
				ai->AIController->DefaultAction();

			return;
		}

		if ((*ProjectileClass || ai->CanReach(favoured, reach)))
			Attack();
		else if (CurrentTarget != favoured)
			ai->AIController->AIMoveTo(favoured);

		CurrentTarget = favoured;
	});
}

FFavourabilityStruct UAttackComponent::GetActorFavourability(AActor* Actor)
{
	FFavourabilityStruct Favourability;

	if (!IsValid(Actor) || Actor->IsA<ATrap>())
		return Favourability;

	UHealthComponent* healthComp = Actor->GetComponentByClass<UHealthComponent>();
	UAttackComponent* attackComp = Actor->GetComponentByClass<UAttackComponent>();

	if (!healthComp || healthComp->Health == 0)
		return Favourability;

	Favourability.Hp = healthComp->Health;

	if (attackComp && attackComp->bCanAttack) {
		if (*attackComp->ProjectileClass)
			Favourability.Dmg = attackComp->ProjectileClass->GetDefaultObject<AProjectile>()->Damage;
		else
			Favourability.Dmg = attackComp->Damage;
	}
	else if (Actor->IsA<AWall>()) {
		int32 num = 1;

		if (!Actor->IsA<ATower>())
			num = Cast<AWall>(Actor)->GetCitizensAtBuilding().Num();

		Favourability.Dmg = Cast<AWall>(Actor)->BuildingProjectileClass->GetDefaultObject<AProjectile>()->Damage * num;
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	NavData->CalcPathLength(GetOwner()->GetActorLocation(), Actor->GetActorLocation(), Favourability.Dist);

	return Favourability;
}

void UAttackComponent::Attack()
{
	if (AttackTimer > 0.0f) {
		AttackTimer -= 0.1f;

		if (AttackTimer <= 0.0f) {
			if (*ProjectileClass)
				Throw();
			else
				Melee();
		}

		return;
	}

	if (!IsValid(CurrentTarget))
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	float time = AttackTime;

	if (GetOwner()->IsA<ACitizen>())
		time /= Cast<ACitizen>(GetOwner())->GetProductivity();

	if (*ProjectileClass) {
		if (RangeAnim->IsValidLowLevelFast()) {
			RangeAnim->RateScale = 0.5f / time;
			
			if (GetOwner()->IsA<AAI>()) {
				Cast<AAI>(GetOwner())->Mesh->PlayAnimation(RangeAnim, false);

				Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = RangeAnim;
			}
		}
	}
	else {
		if (MeleeAnim->IsValidLowLevelFast()) {
			MeleeAnim->RateScale = 0.5f / time;

			Cast<AAI>(GetOwner())->Mesh->PlayAnimation(MeleeAnim, false);

			Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = MeleeAnim;
		}
		else if (GetOwner()->IsA<AEnemy>())
			Cast<AEnemy>(GetOwner())->Zap(CurrentTarget->GetActorLocation());
	}

	AttackTimer = time;
}

void UAttackComponent::Throw()
{
	if (!bCanAttack || CurrentTarget == nullptr)
		return;

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	float z = 0.0f;

	if (GetOwner()->IsA<ABuilding>())
		z = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()))->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	else
		z = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()))->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	FVector startLoc = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, z);

	if (GetOwner()->IsA<AAI>())
		startLoc += GetOwner()->GetActorForwardVector();

	FVector targetLoc = CurrentTarget->GetActorLocation();
	targetLoc += CurrentTarget->GetVelocity() * (FVector::Dist(startLoc, targetLoc) / v);

	FRotator lookAt = (targetLoc - startLoc).Rotation();

	double angle = 0.0f;
	double d = 0.0f;

	FHitResult hit;

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(hit, startLoc, CurrentTarget->GetActorLocation(), ECollisionChannel::ECC_PhysicsBody, queryParams)) {
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

	if (GetOwner()->IsA<AAI>())
		GetOwner()->SetActorRotation(ang);

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->SpawnNiagaraSystems(GetOwner());
}

void UAttackComponent::Melee()
{
	if (!bCanAttack || CurrentTarget == nullptr)
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	healthComp->TakeHealth(Damage * DamageMultiplier, GetOwner());
}

void UAttackComponent::ClearAttacks()
{
	OverlappingEnemies.Empty();

	bCanAttack = false;

	AttackTimer = 0.0f;
}