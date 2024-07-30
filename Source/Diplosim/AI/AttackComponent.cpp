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

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(0.1f);
	SetTickableWhenPaused(false);

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(400.0f);

	RangeComponent->bDynamicObstacle = true;
	RangeComponent->SetCanEverAffectNavigation(false);

	Damage = 10;

	CurrentTarget = nullptr;

	bCanAttack = true;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AAI>(GetOwner());

	if (Owner->IsA<ACitizen>()) {
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);

		if (Cast<ACitizen>(Owner)->BioStruct.Age < 18)
			bCanAttack = false;
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
}

void UAttackComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Owner->IsActorTickEnabled())
		return;

	Async(EAsyncExecution::Thread, [this]() { PickTarget(); });
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ((!*ProjectileClass && !Owner->AIController->CanMoveTo(OtherActor->GetActorLocation())))
		return;

	if (Owner->IsA<AEnemy>() && (OtherActor->IsA<ACitizen>() || OtherActor->IsA<ABuilding>()))
			OverlappingEnemies.Add(OtherActor);
	else if (Owner->IsA<ACitizen>() && OtherActor->IsA<AEnemy>())
		OverlappingEnemies.Add(OtherActor);

	if (OverlappingEnemies.Num() == 1)
		SetComponentTickEnabled(true);
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
	if (!bCanAttack)
		return;

	AActor* favoured = nullptr;

	for (AActor* target : OverlappingEnemies) {
		if (!target->IsValidLowLevelFast()) {
			OverlappingEnemies.Remove(target);

			continue;
		}

		FThreatsStruct threatStruct;

		if (target->IsA<ACitizen>())
			threatStruct.Citizen = Cast<ACitizen>(target);

		TArray<FThreatsStruct> threats = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode())->WavesData.Last().Threats;

		if (!threats.IsEmpty() && threats.Contains(threatStruct)) {
			if (threatStruct.Citizen->AttackComponent->RangeComponent->CanEverAffectNavigation() == true)
				continue;

			favoured = threatStruct.Citizen->Building.Employment;

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
			Owner->AIController->DefaultAction();

			GetWorld()->GetTimerManager().ClearTimer(AttackTimer);

			return;
		}

		if ((*ProjectileClass || MeleeableEnemies.Contains(CurrentTarget)) && !GetWorld()->GetTimerManager().IsTimerActive(AttackTimer))
			Attack();
		else
			Owner->AIController->AIMoveTo(CurrentTarget);
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

	NavData->CalcPathLength(Owner->GetActorLocation(), Actor->GetActorLocation(), Favourability.Dist);

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
	if (!CurrentTarget->IsValidLowLevelFast())
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (healthComp->Health == 0)
		return;

	Owner->AIController->StopMovement();

	float time = 1.0f;

	if (Owner->IsA<ACitizen>())
		time /= FMath::LogX(100.0f, FMath::Clamp(Cast<ACitizen>(Owner)->Energy, 2, 100));

	FTimerHandle AnimTimer;

	if (*ProjectileClass) {
		if (RangeAnim->IsValidLowLevelFast()) {
			RangeAnim->RateScale = 0.5f / time;

			Owner->GetMesh()->PlayAnimation(RangeAnim, false);
		}

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UAttackComponent::Throw, time, false);
	}
	else {
		if (MeleeAnim->IsValidLowLevelFast()) {
			MeleeAnim->RateScale = 0.5f / time;

			Owner->GetMesh()->PlayAnimation(MeleeAnim, false);
		}

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, this, &UAttackComponent::Melee, time / 2, false);
	}

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::Attack, time, false);
}

void UAttackComponent::Throw()
{
	if (!bCanAttack || CurrentTarget == nullptr)
		return;

	Async(EAsyncExecution::Thread, [this]() {
		USkeletalMeshComponent* ownerComp = Cast<USkeletalMeshComponent>(Owner->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		USkeletalMeshComponent* targetComp = Cast<USkeletalMeshComponent>(CurrentTarget->GetComponentByClass(USkeletalMeshComponent::StaticClass()));

		UProjectileMovementComponent* projectileMovement = ProjectileClass->GetDefaultObject<AProjectile>()->ProjectileMovementComponent;

		double g = FMath::Abs(GetWorld()->GetGravityZ());
		double v = projectileMovement->InitialSpeed;

		FVector startLoc = Owner->GetActorLocation() + FVector(0.0f, 0.0f, ownerComp->GetSkeletalMeshAsset()->GetBounds().GetBox().GetSize().Z) + GetOwner()->GetActorForwardVector();

		FVector targetLoc = CurrentTarget->GetActorLocation();
		targetLoc += CurrentTarget->GetVelocity() * (FVector::Dist(startLoc, targetLoc) / v);

		FRotator lookAt = (targetLoc - startLoc).Rotation();

		double angle = 0.0f;
		double d = 0.0f;

		FHitResult hit;

		FCollisionQueryParams queryParams;
		queryParams.AddIgnoredActor(Owner);

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

		Owner->SetActorRotation(ang);

		AProjectile* projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, startLoc, ang);
		projectile->Owner = Owner;
	});
}

void UAttackComponent::Melee()
{
	if (!bCanAttack || CurrentTarget == nullptr)
		return;

	UHealthComponent* healthComp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	healthComp->TakeHealth(Damage, Owner);
}

void UAttackComponent::ClearAttacks()
{
	for (TWeakObjectPtr<AActor> target : OverlappingEnemies) {
		if (!target->IsValidLowLevelFast() || !target->IsA<AAI>())
			continue;

		AAI* ai = Cast<AAI>(target);

		ai->AttackComponent->OverlappingEnemies.Remove(Owner);
		ai->AttackComponent->MeleeableEnemies.Remove(Owner);
	}

	OverlappingEnemies.Empty();
	MeleeableEnemies.Empty();

	bCanAttack = false;

	AsyncTask(ENamedThreads::GameThread, [this]() { GetWorld()->GetTimerManager().ClearTimer(AttackTimer); });
}