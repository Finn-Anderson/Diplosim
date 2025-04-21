#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
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
#include "Buildings/Building.h"
#include "Universal/DiplosimGameModeBase.h"
#include "AIMovementComponent.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(0.1f);
	SetTickableWhenPaused(false);

	Damage = 10;

	AttackTime = 1.0f;

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

	if (DeltaTime < 0.0001f)
		return;

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

	for (AActor* target : OverlappingEnemies) {
		if (!IsValid(target)) {
			OverlappingEnemies.Remove(target);

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

		FFavourabilityStruct targetFavourability = GetActorFavourability(target);
		FFavourabilityStruct favouredFavourability = GetActorFavourability(favoured);

		double favourability = (favouredFavourability.Hp * favouredFavourability.Dist / favouredFavourability.Dmg) - (targetFavourability.Hp * targetFavourability.Dist / targetFavourability.Dmg);

		if (favourability > 0.0f)
			favoured = target;
	}

	if (CurrentTarget == favoured)
		return;

	if (favoured == nullptr)
		SetComponentTickEnabled(false);

	AsyncTask(ENamedThreads::GameThread, [this, favoured]() {
		CurrentTarget = favoured;

		if (CurrentTarget == nullptr) {
			if (GetOwner()->IsA<AAI>())
				Cast<AAI>(GetOwner())->AIController->DefaultAction();

			GetWorld()->GetTimerManager().ClearTimer(AttackTimer);

			return;
		}

		if ((*ProjectileClass || MeleeableEnemies.Contains(CurrentTarget)) && !GetWorld()->GetTimerManager().IsTimerActive(AttackTimer))
			Attack();
		else if (GetOwner()->IsA<AAI>())
			Cast<AAI>(GetOwner())->AIController->AIMoveTo(CurrentTarget);
	});
}

FFavourabilityStruct UAttackComponent::GetActorFavourability(AActor* Actor)
{
	FFavourabilityStruct Favourability;

	if (Actor == nullptr || !Actor->IsValidLowLevelFast())
		return Favourability;

	UHealthComponent* healthComp = Actor->GetComponentByClass<UHealthComponent>();
	UAttackComponent* attackComp = Actor->GetComponentByClass<UAttackComponent>();

	if (healthComp->Health == 0)
		return Favourability;

	Favourability.Hp = healthComp->Health;

	if (attackComp && attackComp->bCanAttack) {
		if (*attackComp->ProjectileClass)
			Favourability.Dmg = attackComp->ProjectileClass->GetDefaultObject<AProjectile>()->Damage;
		else
			Favourability.Dmg = attackComp->Damage;
	}
	else if (Actor->IsA<AWall>())
		Favourability.Dmg = Cast<AWall>(Actor)->BuildingProjectileClass->GetDefaultObject<AProjectile>()->Damage;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	NavData->CalcPathLength(GetOwner()->GetActorLocation(), Actor->GetActorLocation(), Favourability.Dist);

	return Favourability;
}

void UAttackComponent::CanHit(AActor* Target)
{
	if (!bCanAttack)
		return;

	MeleeableEnemies.Add(Target);

	if (CurrentTarget == Target)
		Attack();
}

void UAttackComponent::Attack()
{
	Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = nullptr;

	if (!CurrentTarget->IsValidLowLevelFast())
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	if (GetOwner()->IsA<AAI>())
		Cast<AAI>(GetOwner())->AIController->StopMovement();

	float time = AttackTime;

	if (GetOwner()->IsA<ACitizen>())
		time /= FMath::LogX(100.0f, FMath::Clamp(Cast<ACitizen>(GetOwner())->Energy, 2, 100));

	FTimerHandle AnimTimer;

	if (*ProjectileClass) {
		if (RangeAnim->IsValidLowLevelFast()) {
			RangeAnim->RateScale = 0.5f / time;
			
			if (GetOwner()->IsA<AAI>()) {
				Cast<AAI>(GetOwner())->Mesh->PlayAnimation(RangeAnim, false);

				Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = RangeAnim;
			}
		}

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UAttackComponent::Throw, time, false);
	}
	else {
		if (MeleeAnim->IsValidLowLevelFast()) {
			MeleeAnim->RateScale = 0.5f / time;

			Cast<AAI>(GetOwner())->Mesh->PlayAnimation(MeleeAnim, false);

			Cast<AAI>(GetOwner())->MovementComponent->CurrentAnim = MeleeAnim;
		}
		else if (GetOwner()->IsA<AEnemy>())
			Cast<AEnemy>(GetOwner())->Zap(CurrentTarget->GetActorLocation());

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UAttackComponent::Melee, time / 2, false);
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::Attack, time, false);
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
	for (TWeakObjectPtr<AActor> target : OverlappingEnemies) {
		if (!target->IsValidLowLevelFast() || !target->IsA<AAI>())
			continue;

		AAI* ai = Cast<AAI>(target);

		ai->AttackComponent->OverlappingEnemies.Remove(GetOwner());
		ai->AttackComponent->MeleeableEnemies.Remove(GetOwner());
	}

	OverlappingEnemies.Empty();
	MeleeableEnemies.Empty();

	bCanAttack = false;

	AsyncTask(ENamedThreads::GameThread, [this]() { GetWorld()->GetTimerManager().ClearTimer(AttackTimer); });
}