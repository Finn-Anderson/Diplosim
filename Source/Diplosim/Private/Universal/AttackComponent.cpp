#include "Universal/AttackComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"

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

	AttackTime = 1.0f;
	AttackTimer = 0.0f;

	CurrentTarget = nullptr;

	DamageMultiplier = 1.0f;

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

void UAttackComponent::PickTarget(float DeltaTime)
{
	AActor* favoured = nullptr;
	AAI* ai = nullptr;
	if (GetOwner()->IsA<AAI>())
		ai = Cast<AAI>(GetOwner());

	if (bClearAttacks) {
		OverlappingEnemies.Empty();

		bClearAttacks = false;
	}

	for (int32 i = OverlappingEnemies.Num() - 1; i > -1; i--) {
		if (i >= OverlappingEnemies.Num())
			continue;

		AActor* target = OverlappingEnemies[i];
		UAttackComponent* targetAttackComp = target->FindComponentByClass<UAttackComponent>();
		UHealthComponent* targetHealthComp = target->FindComponentByClass<UHealthComponent>();
		FFavourabilityStruct targetFavourability = GetActorFavourability(target);

		bool withinRange = true;

		if (GetOwner()->IsA<AWall>())
			withinRange = FVector::Dist(Camera->GetTargetActorLocation(GetOwner()), Camera->GetTargetActorLocation(target)) <= Cast<AWall>(GetOwner())->RangeComponent->GetScaledSphereRadius();
		else if (GetOwner()->IsA<AAI>() && *ProjectileClass)
			withinRange = ai->CanReach(target, ai->Range);

		if (targetFavourability.Hp <= (bShowMercy ? targetHealthComp->MaxHealth / 4 : 0) || targetFavourability.Hp == 10000000 || !withinRange || (targetAttackComp && !targetAttackComp->OverlappingEnemies.Contains(ai))) {
			OverlappingEnemies.RemoveAt(i);

			continue;
		}

		TArray<TWeakObjectPtr<AActor>> threats;
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

		if (GetOwner()->IsA<AEnemy>() && !gamemode->IsSnakeFaction(GetOwner()))
			threats = gamemode->WavesData.Last().Threats;

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

	//if (!IsMoraleHigh())
		//return; REDO BUT BETTER

	int32 reach = 0.0f;

	if (ai != nullptr)
		reach = ai->GetReach();

	if (favoured == nullptr) {
		AttackTimer = 0.0f;

		if (IsValid(ai))
			ai->AIController->DefaultAction();

		bShowMercy = false;

		return;
	}

	if ((*ProjectileClass || ai->CanReach(favoured, reach)))
		Attack(favoured, DeltaTime);
	else if (CurrentTarget != favoured) {
		CurrentTarget = favoured;

		ai->AIController->AIMoveTo(favoured);
	}
}

FFavourabilityStruct UAttackComponent::GetActorFavourability(AActor* Actor)
{
	FFavourabilityStruct Favourability;

	if (!IsValid(Actor))
		return Favourability;

	UHealthComponent* healthComp = Actor->GetComponentByClass<UHealthComponent>();
	UAttackComponent* attackComp = Actor->GetComponentByClass<UAttackComponent>();

	if (!healthComp)
		return Favourability;

	Favourability.Hp = healthComp->GetHealth();

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

void UAttackComponent::Attack(AActor* Target, float DeltaTime)
{
	if (!IsValid(Target))
		return;

	if (CurrentTarget != Target)
		CurrentTarget = Target;

	UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	if (AttackTimer > 0.0f) {
		AttackTimer -= DeltaTime;

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

void UAttackComponent::Throw()
{
	if (!IsValid(CurrentTarget))
		return;

	UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

	float z = 0.0f;
	if (GetOwner()->IsA<ABuilding>())
		z = Cast<UStaticMeshComponent>(GetOwner()->GetRootComponent())->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	else
		z = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(GetOwner())).Key->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	double g = FMath::Abs(GetWorld()->GetGravityZ());
	double v = projectileMovement->InitialSpeed;

	FVector startLoc = Camera->GetTargetActorLocation(GetOwner(), false) + FVector(0.0f, 0.0f, z / 2.0f);
	FVector targetLoc = Camera->GetTargetActorLocation(CurrentTarget, false);
	if (CurrentTarget->IsA<AAI>())
		targetLoc += Cast<AAI>(CurrentTarget)->MovementComponent->Velocity * (FVector::Dist(startLoc, targetLoc) / v) * 2.0f;
	FRotator lookAt = (targetLoc - startLoc).Rotation();

	FVector groundedLocation = FVector(startLoc.X, startLoc.Y, targetLoc.Z);
	double d = FVector::Dist(groundedLocation, targetLoc);
	double h = targetLoc.Z - startLoc.Z;

	double angle = FMath::Atan2(FMath::Square(v) + FMath::Sqrt(FMath::Pow(v, 4) - g * (g * FMath::Square(d) + 2 * h * FMath::Square(v))), g * d) * (180.0f / PI);

	FRotator ang = FRotator(angle, lookAt.Yaw, lookAt.Roll);

	Async(EAsyncExecution::TaskGraphMainTick, [this, startLoc, ang]() {
		AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
		projectile->SpawnNiagaraSystems(GetOwner());
		projectile->FactionName = Camera->ConquestManager->GetFaction("", GetOwner())->Name;
	});
}

void UAttackComponent::Melee()
{
	if (!IsValid(CurrentTarget))
		return;

	USoundBase* sound = OnHitSound;

	if (GetOwner()->IsA<AEnemy>()) {
		Cast<AEnemy>(GetOwner())->Zap(Camera->GetTargetActorLocation(CurrentTarget));

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
}