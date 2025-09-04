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
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

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

	DamageMultiplier = 1.0f;

	bAttackedRecently = false;
	bShowMercy = false;
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
	AActor* favoured = nullptr;

	for (int32 i = OverlappingEnemies.Num() - 1; i > -1; i--) {
		AActor* target = OverlappingEnemies[i];
		FFavourabilityStruct targetFavourability = GetActorFavourability(target);

		bool withinRange = true;
		if (GetOwner()->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(GetOwner());

			withinRange = ai->CanReach(target, ai->Range);
		}

		if (targetFavourability.Hp == 0 || targetFavourability.Hp == 10000000 || !withinRange) {
			OverlappingEnemies.RemoveAt(i);

			continue;
		}

		FThreatsStruct threatStruct;
		threatStruct.Actor = target;

		TArray<FThreatsStruct> threats;

		if (GetOwner()->IsA<AEnemy>())
			threats = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats;

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
	}

	if (favoured == nullptr) {
		SetComponentTickEnabled(false);

		AttackTimer = 0.0f;
	}
	else if (!*ProjectileClass && ai->CanReach(favoured, reach))
		ai->AIController->StopMovement();

	AsyncTask(ENamedThreads::GameThread, [this, favoured, ai, reach]() {
		if (favoured == nullptr) {
			if (IsValid(ai))
				ai->AIController->DefaultAction();

			bShowMercy = false;

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

	if (attackComp) {
		if (*attackComp->ProjectileClass)
			Favourability.Dmg = attackComp->ProjectileClass->GetDefaultObject<AProjectile>()->Damage * attackComp->DamageMultiplier;
		else
			Favourability.Dmg = attackComp->Damage * attackComp->DamageMultiplier;
	}
	else if (Actor->IsA<AWall>()) {
		float num = 1.0f;

		if (!Actor->IsA<ATower>()) {
			num = 0.0f;

			for (ACitizen* citizen : Cast<AWall>(Actor)->GetCitizensAtBuilding())
				num += 1.0f * citizen->AttackComponent->DamageMultiplier;
		}

		Favourability.Dmg = Cast<AWall>(Actor)->BuildingProjectileClass->GetDefaultObject<AProjectile>()->Damage * num;
	}

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

	NavData->CalcPathLength(Camera->GetTargetActorLocation(GetOwner()), Camera->GetTargetActorLocation(Actor), Favourability.Dist);

	return Favourability;
}

void UAttackComponent::Attack()
{
	if (!IsValid(CurrentTarget))
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	float time = AttackTime;

	if (GetOwner()->IsA<ACitizen>())
		time /= Cast<ACitizen>(GetOwner())->GetProductivity();

	if (AttackTimer > 0.0f) {
		AttackTimer -= 0.1f;

		if (AttackTimer <= (time / 2)) {
			if (*ProjectileClass)
				Throw();
			else
				Melee();

			bAttackedRecently = true;
		}

		return;
	}

	AttackTimer = time;
	bAttackedRecently = false;

	FFactionStruct* faction1 = Camera->ConquestManager->GetFaction("", CurrentTarget);
	FFactionStruct* faction2 = Camera->ConquestManager->GetFaction("", GetOwner());

	if (faction1 == nullptr || faction2 == nullptr)
		return;

	if (bShowMercy && healthComp->Health < 25)
		Camera->CitizenManager->StopFighting(Cast<ACitizen>(GetOwner()));
	else if (healthComp->Health == 0)
		Camera->CitizenManager->ChangeReportToMurder(Cast<ACitizen>(GetOwner()));
}

void UAttackComponent::Throw()
{
	if (CurrentTarget == nullptr)
		return;

	if (GetOwner()->IsA<AAI>())
		Cast<AAI>(GetOwner())->MovementComponent->SetAnimation(EAnim::Throw);

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	float z = 0.0f;

	if (GetOwner()->IsA<ABuilding>())
		z = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()))->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	else
		z = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(GetOwner())).Key->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	FVector startLoc = Camera->GetTargetActorLocation(GetOwner()) + FVector(0.0f, 0.0f, z);

	if (GetOwner()->IsA<AAI>())
		startLoc += Cast<AAI>(GetOwner())->MovementComponent->Transform.GetRotation().Vector();

	FVector targetLoc = Camera->GetTargetActorLocation(CurrentTarget);
	targetLoc += CurrentTarget->GetVelocity() * (FVector::Dist(startLoc, targetLoc) / v);

	FRotator lookAt = (targetLoc - startLoc).Rotation();

	double angle = 0.0f;
	double d = 0.0f;

	FHitResult hit;

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(GetOwner());

	FVector endLoc = Camera->GetTargetActorLocation(CurrentTarget);

	if (GetWorld()->LineTraceSingleByChannel(hit, startLoc, endLoc, ECollisionChannel::ECC_PhysicsBody, queryParams)) {
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
		Cast<AAI>(GetOwner())->MovementComponent->Transform.SetRotation(ang.Quaternion());

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->SpawnNiagaraSystems(GetOwner());
}

void UAttackComponent::Melee()
{
	if (CurrentTarget == nullptr)
		return;

	if (GetOwner()->IsA<AEnemy>())
		Cast<AEnemy>(GetOwner())->Zap(Camera->GetTargetActorLocation(CurrentTarget));
	else
		Cast<AAI>(GetOwner())->MovementComponent->SetAnimation(EAnim::Melee);

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();
	
	int32 dmg = Damage;
	
	if (GetOwner()->IsA<ACitizen>())
		dmg *= 1 / (18 / FMath::Clamp(Cast<ACitizen>(GetOwner())->BioStruct.Age, 0, 18));

	healthComp->TakeHealth(dmg * DamageMultiplier, GetOwner());
}

void UAttackComponent::ClearAttacks()
{
	OverlappingEnemies.Empty();

	AttackTimer = 0.0f;
}