#include "AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"

#include "AI/AI.h"
#include "AI/Enemy.h"
#include "AI/Citizen.h"
#include "AI/Projectile.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/BioComponent.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Broch.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/PoliceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Damage = 10;

	AttackTime = 1.0f;
	AttackTimer = 0.0f;

	CurrentTarget = nullptr;

	DamageMultiplier = 1.0f;

	bShowMercy = false;
	bFactorMorale = false;
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

		TArray<TWeakObjectPtr<AActor>> threats;

		if (GetOwner()->IsA<AEnemy>())
			threats = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats;

		if (!threats.IsEmpty() && threats.Contains(target)) {
			if (target->IsA<AWall>() && Cast<AWall>(target)->RangeComponent->CanEverAffectNavigation() == true)
				continue;

			favoured = target;

			break;
		}

		FFavourabilityStruct favouredFavourability = GetActorFavourability(favoured);

		double favourability = (favouredFavourability.Hp * favouredFavourability.Dist / favouredFavourability.Dmg) - (targetFavourability.Hp * targetFavourability.Dist / targetFavourability.Dmg);

		if (favourability > 0.0f)
			favoured = target;
	}

	if (!IsMoraleHigh())
		return;

	int32 reach = 0.0f;
	AAI* ai = nullptr;

	if (GetOwner()->IsA<AAI>()) {
		ai = Cast<AAI>(GetOwner());

		reach = ai->Range / 15.0f;
	}

	if (favoured == nullptr)
		AttackTimer = 0.0f;
	else if (!*ProjectileClass && ai->CanReach(favoured, reach))
		ai->AIController->StopMovement();

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

bool UAttackComponent::IsMoraleHigh()
{
	if (!bFactorMorale || OverlappingEnemies.IsEmpty())
		return true;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* Faction = Camera->ConquestManager->GetFaction("", citizen);

	FVector citizenLocation = Camera->GetTargetActorLocation(citizen);
	FVector eggTimerLocation = Camera->GetTargetActorLocation(Faction->EggTimer);

	if (IsValid(Faction->EggTimer) && (FVector::Dist(citizenLocation, eggTimerLocation) < 500.0f || !citizen->AIController->CanMoveTo(eggTimerLocation) || Faction->EggTimer->HealthComponent->GetHealth() <= 0))
		return true;

	int32 enemiesNum = OverlappingEnemies.Num();

	FOverlapsStruct overlaps;
	overlaps.bCitizens = true;
	overlaps.bClones = true;
	int32 alliesNum = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range, overlaps, EFactionType::Same, Faction, citizenLocation).Num();

	int32 moraleMultiplier = 1.0f;
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
		moraleMultiplier *= personality->Morale;

	float morale = 50.0f * moraleMultiplier - FMath::Pow(2.0f, FMath::Max(enemiesNum - alliesNum, 0));

	if (morale <= 0.0f) {
		Camera->ArmyManager->RemoveFromArmy(citizen);

		if (IsValid(Faction->EggTimer))
			citizen->AIController->AIMoveTo(Faction->EggTimer);
	}

	return morale > 0.0f;
}

void UAttackComponent::Attack()
{
	if (!IsValid(CurrentTarget))
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	float time = AttackTime;

	if (GetOwner()->IsA<AWall>()) {
		AWall* defensiveBuilding = Cast<AWall>(GetOwner());

		for (ACitizen* citizen : defensiveBuilding->GetCitizensAtBuilding())
			time -= (time / defensiveBuilding->GetCitizensAtBuilding().Num()) * citizen->GetProductivity() - 1.0f;
	}

	if (AttackTimer > 0.0f) {
		AttackTimer -= 0.1f;

		return;
	}
	else if (GetOwner()->IsA<AAI>() && Cast<AAI>(GetOwner())->MovementComponent->IsAttacking())
		return;

	if ((GetOwner()->IsA<ABuilding>() && *ProjectileClass) || (GetOwner()->IsA<AEnemy>() && !*ProjectileClass)) {
		AttackTimer = time;

		if (*ProjectileClass)
			Throw();
		else
			Melee();
	}
	else {
		time = 2.0f;

		if (GetOwner()->IsA<ACitizen>())
			time *= Cast<ACitizen>(GetOwner())->GetProductivity();

		UAIMovementComponent* movementComponent = Cast<AAI>(GetOwner())->MovementComponent;

		if (*ProjectileClass)
			movementComponent->SetAnimation(EAnim::Throw, false, time);
		else
			movementComponent->SetAnimation(EAnim::Melee, false, time);
	}

	if (GetOwner()->IsA<AAI>())
		Cast<AAI>(GetOwner())->MovementComponent->ActorToLookAt = CurrentTarget;

	FFactionStruct* faction1 = Camera->ConquestManager->GetFaction("", CurrentTarget);
	FFactionStruct* faction2 = Camera->ConquestManager->GetFaction("", GetOwner());

	if (faction1 == nullptr || faction2 == nullptr)
		return;

	if (bShowMercy && healthComp->Health < 25)
		Camera->PoliceManager->StopFighting(Cast<ACitizen>(GetOwner()));
	else if (healthComp->Health == 0)
		Camera->PoliceManager->ChangeReportToMurder(Cast<ACitizen>(GetOwner()));
}

void UAttackComponent::Throw()
{
	if (CurrentTarget == nullptr)
		return;

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	float z = 0.0f;

	if (GetOwner()->IsA<ABuilding>())
		z = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()))->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	else
		z = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(GetOwner())).Key->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	FVector startLoc = Camera->GetTargetActorLocation(GetOwner()) + FVector(0.0f, 0.0f, z);

	FVector targetLoc = Camera->GetTargetActorLocation(CurrentTarget);
	targetLoc += CurrentTarget->GetVelocity() * (FVector::Dist(startLoc, targetLoc) / v);

	FRotator lookAt = (targetLoc - startLoc).Rotation();

	if (GetOwner()->IsA<AAI>())
		startLoc += lookAt.Vector();

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

	AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
	projectile->SpawnNiagaraSystems(GetOwner());
}

void UAttackComponent::Melee()
{
	if (CurrentTarget == nullptr)
		return;

	USoundBase* sound = OnHitSound;

	if (GetOwner()->IsA<AEnemy>()) {
		Cast<AEnemy>(GetOwner())->Zap(Camera->GetTargetActorLocation(CurrentTarget));

		sound = ZapSound;
	}

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();
	
	int32 dmg = Damage;
	
	if (GetOwner()->IsA<ACitizen>())
		dmg *= 1 / (18 / FMath::Clamp(Cast<ACitizen>(GetOwner())->BioComponent->Age, 0, 18));

	healthComp->TakeHealth(dmg * DamageMultiplier, GetOwner(), sound);
}

void UAttackComponent::ClearAttacks()
{
	OverlappingEnemies.Empty();

	AttackTimer = 0.0f;
}