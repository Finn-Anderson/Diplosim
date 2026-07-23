#include "Universal/AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

#include "AI/AI.h"
#include "AI/Enemy.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Tower.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Broch.h"
#include "Map/Grid.h"
#include "Map/AIInstancedStaticMeshComponent.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"
#include "Universal/Projectile.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Damage = 10;
	DamageMultiplier = 1.0f;

	AttackTime = 1.0f;
	AttackTimer = 0.0f;
	LastUpdateTime = 0.0f;

	Camera = nullptr;
	CurrentTarget = nullptr;
	OnHitSound = nullptr;
	ZapSound = nullptr;

	bShowMercy = false;
	bFactorMorale = false;
	bClearAttacks = false;
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
	AAI* ai = nullptr;
	if (GetOwner()->IsA<AAI>())
		ai = Cast<AAI>(GetOwner());

	if (bClearAttacks) {
		OverlappingEnemies.Empty();

		bClearAttacks = false;
	}

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	if (gamemode->Enemies.Contains(GetOwner()) && !gamemode->WavesData.Last().Threats.IsEmpty()) {
		for (const TWeakObjectPtr<AActor>& threat : gamemode->WavesData.Last().Threats) {
			if (threat == nullptr || !OverlappingEnemies.Contains(threat.Get()))
				continue;

			if (favoured == nullptr) {
				favoured = threat.Get();

				continue;
			}

			favoured = GetFavouredActor(favoured, threat.Get());
		}
	}

	if (favoured == nullptr) {
		for (int32 i = OverlappingEnemies.Num() - 1; i > -1; i--) {
			if (i >= OverlappingEnemies.Num())
				continue;

			AActor* target = OverlappingEnemies[i];
			UAttackComponent* targetAttackComp = target->FindComponentByClass<UAttackComponent>();
			UHealthComponent* targetHealthComp = target->FindComponentByClass<UHealthComponent>();

			bool withinRange = true;

			if (GetOwner()->IsA<AWall>())
				withinRange = FVector::Dist(Camera->GetTargetActorLocation(GetOwner()), Camera->GetTargetActorLocation(target)) <= Cast<AWall>(GetOwner())->RangeComponent->GetScaledSphereRadius();
			else if (GetOwner()->IsA<AAI>() && *ProjectileClass)
				withinRange = ai->CanReach(target, ai->Range);

			if (targetHealthComp->GetHealth() <= (bShowMercy ? targetHealthComp->MaxHealth / 4 : 0) || !withinRange) {
				OverlappingEnemies.RemoveAt(i);

				continue;
			}

			if (favoured == nullptr) {
				favoured = target;

				continue;
			}

			favoured = GetFavouredActor(favoured, target);
		}
	}

	//if (!IsMoraleHigh())
		//return; REDO BUT BETTER

	int32 reach = 0.0f;

	if (ai != nullptr)
		reach = ai->GetReach();

	if (favoured == nullptr) {
		if (IsValid(ai))
			ai->AIController->DefaultAction();

		bShowMercy = false;

		return;
	}

	if ((*ProjectileClass || ai->CanReach(favoured, reach)))
		Attack(favoured);
	else if (CurrentTarget != favoured) {
		CurrentTarget = favoured;

		ai->AIController->AIMoveTo(favoured);
	}
}

AActor* UAttackComponent::GetFavouredActor(AActor* CurrentFavoured, AActor* Actor)
{
	FVector location = Camera->GetTargetActorLocation(GetOwner());

	FVector favouredLocation = FVector::Zero();
	FVector newLocation = FVector::Zero();

	if (CurrentFavoured->IsA<ABuilding>())
		Cast<ABuilding>(CurrentFavoured)->BuildingMesh->GetClosestPointOnCollision(location, favouredLocation);
	else
		favouredLocation = Cast<AAI>(CurrentFavoured)->MovementComponent->Transform.GetLocation();

	if (Actor->IsA<ABuilding>())
		Cast<ABuilding>(Actor)->BuildingMesh->GetClosestPointOnCollision(location, newLocation);
	else
		newLocation = Cast<AAI>(Actor)->MovementComponent->Transform.GetLocation();

	if (FVector::Dist(location, favouredLocation) > FVector::Dist(location, newLocation))
		return Actor;

	return CurrentFavoured;
}

bool UAttackComponent::IsMoraleHigh()
{
	if (!bFactorMorale || OverlappingEnemies.IsEmpty())
		return true;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* Faction = Camera->ConquestManager->GetFaction("", citizen);

	FVector citizenLocation = Camera->GetTargetActorLocation(citizen);

	if (IsValid(Faction->EggTimer) && FVector::Dist(citizenLocation, Camera->GetTargetActorLocation(Faction->EggTimer)) < 500.0f)
		return true;

	int32 enemiesNum = 0;
	for (AActor* enemy : OverlappingEnemies)
		if (!enemy->IsA<AAISpawner>())
			enemiesNum++;

	FOverlapsStruct overlaps;
	overlaps.bCitizens = true;
	overlaps.bClones = true;
	int32 alliesNum = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range, overlaps, EFactionType::Same, Faction, citizenLocation).Num();

	int32 moraleMultiplier = 1.0f;
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
		moraleMultiplier *= personality->Morale;

	float morale = 50.0f * moraleMultiplier + (FMath::Pow(2.0f, alliesNum) - FMath::Pow(2.0f, enemiesNum));

	if (morale <= 0.0f) {
		Camera->ArmyManager->RemoveFromArmy(citizen);

		if (IsValid(Faction->EggTimer) && citizen->AIController->MoveRequest.GetGoalActor() != Faction->EggTimer)
			citizen->AIController->AIMoveTo(Faction->EggTimer);
	}

	return morale > 0.0f;
}

void UAttackComponent::Attack(AActor* Target)
{
	if (!IsValid(Target))
		return;

	if (CurrentTarget != Target)
		CurrentTarget = Target;

	UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	if (AttackTimer > 0.0f) {
		AttackTimer -= (GetWorld()->GetTimeSeconds() - LastUpdateTime);
		LastUpdateTime = GetWorld()->GetTimeSeconds();

		return;
	}
	else if (GetOwner()->IsA<AAI>() && Cast<AAI>(GetOwner())->MovementComponent->IsAttacking())
		return;

	if (GetOwner()->IsA<AAI>()) {
		AAI* ai = Cast<AAI>(GetOwner());

		ai->MovementComponent->SetPoints({});
		ai->MovementComponent->ActorToLookAt = Target;
	}

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if ((GetOwner()->IsA<ABuilding>() && *ProjectileClass) || (GetOwner()->IsA<AEnemy>() && !gamemode->IsSnakeFaction(GetOwner()) && !*ProjectileClass)) {
		float time = AttackTime;

		if (GetOwner()->IsA<AWall>()) {
			AWall* defensiveBuilding = Cast<AWall>(GetOwner());

			for (ACitizen* citizen : defensiveBuilding->GetCitizensAtBuilding())
				time -= (time / defensiveBuilding->GetCitizensAtBuilding().Num()) * citizen->GetProductivity() - 1.0f;
		}

		AttackTimer = time;
		LastUpdateTime = GetWorld()->GetTimeSeconds();

		if (*ProjectileClass)
			Throw();
		else
			Melee();
	}
	else {
		float time = 2.0f;

		if (GetOwner()->IsA<ACitizen>())
			time *= Cast<ACitizen>(GetOwner())->GetProductivity();

		UAIMovementComponent* movementComponent = Cast<AAI>(GetOwner())->MovementComponent;

		if (*ProjectileClass)
			movementComponent->SetAnimation(EAnim::Throw, false, time);
		else
			movementComponent->SetAnimation(EAnim::Melee, false, time);
	}
}

FVector UAttackComponent::GetThrowLocation(AActor* Actor)
{
	FVector location = Camera->GetTargetActorLocation(Actor, false);

	float z = 0.0f;
	if (Actor->IsA<ABuilding>())
		z = Cast<UStaticMeshComponent>(Actor->GetRootComponent())->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	else
		z = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(Actor)).Key->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	location += FVector(0.0f, 0.0f, z / 2.0f);

	return location;
}

void UAttackComponent::Throw()
{
	if (!IsValid(CurrentTarget))
		return;

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	const FVector startLoc = GetThrowLocation(GetOwner());
	const FVector targetLoc = GetThrowLocation(CurrentTarget);

	FVector targetVelocity = FVector::Zero();
	if (CurrentTarget->IsA<AAI>()) {
		UAIMovementComponent* movementComp = Cast<AAI>(CurrentTarget)->MovementComponent;
		targetVelocity = movementComp->Velocity;
		
		if (!movementComp->Points.IsEmpty() && FVector::Dist(movementComp->Transform.GetLocation(), movementComp->Transform.GetLocation() + targetVelocity) > FVector::Dist(movementComp->Transform.GetLocation(), movementComp->Points.Last()))
			targetVelocity = movementComp->Points.Last() - movementComp->Transform.GetLocation();
	}

	const double gravityZ = GetWorld()->GetGravityZ();
	const FVector gravityVector = FVector(0.0, 0.0, gravityZ);

	FVector velocity = (targetLoc + targetVelocity - startLoc - (0.5f * gravityVector));

	Async(EAsyncExecution::TaskGraphMainTick, [this, startLoc, velocity]() {
		AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, velocity.Rotation());
		projectile->SpawnNiagaraSystems(GetOwner());
		projectile->FactionName = Camera->ConquestManager->GetFaction("", GetOwner())->Name;
		projectile->ProjectileMovementComponent->Velocity = velocity;
	});
}

void UAttackComponent::Melee()
{
	if (!IsValid(CurrentTarget))
		return;

	USoundBase* sound = OnHitSound;

	if (GetOwner()->IsA<AEnemy>()) {
		Cast<AEnemy>(GetOwner())->Zap(Camera->GetTargetActorLocation(CurrentTarget, false));

		sound = ZapSound;
	}

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();
	
	float dmg = Damage;

	if (GetOwner()->IsA<ACitizen>())
		dmg *= 1.0f / (18.0f / FMath::Clamp(Cast<ACitizen>(GetOwner())->BioComponent->Age, 1.0f, 18.0f));

	healthComp->TakeHealth(dmg * DamageMultiplier, GetOwner(), sound);
}

void UAttackComponent::ClearAttacks()
{
	bClearAttacks = true;
	bShowMercy = false;
}