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
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(400.0f);

	RangeComponent->bDynamicObstacle = true;
	RangeComponent->SetCanEverAffectNavigation(false);

	Damage = 10;

	CanAttack = ECanAttack::Valid;

	CurrentTarget = nullptr;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AAI>(GetOwner());

	if (Owner->IsA<ACitizen>()) {
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);

		if (Cast<ACitizen>(Owner)->BioStruct.Age < 18)
			CanAttack = ECanAttack::Invalid;
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ((!*ProjectileClass && !Owner->AIController->CanMoveTo(OtherActor->GetActorLocation())))
		return;

	if (Owner->IsA<AEnemy>() && (OtherActor->IsA<ACitizen>() || OtherActor->IsA<ABuilding>()))
			OverlappingEnemies.Add(OtherActor);
	else if (Owner->IsA<ACitizen>() && OtherActor->IsA<AEnemy>())
		OverlappingEnemies.Add(OtherActor);

	if (!OverlappingEnemies.Contains(OtherActor))
		return;

	if (CurrentTarget == nullptr || !CurrentTarget->IsValidLowLevelFast())
		Async(EAsyncExecution::Thread, [this]() { PickTarget(); });
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
	if (CanAttack == ECanAttack::Invalid)
		return;

	CurrentTarget = nullptr;

	AActor* favoured = nullptr;

	for (AActor* target : OverlappingEnemies) {
		UHealthComponent* healthComp = target->GetComponentByClass<UHealthComponent>();
		UAttackComponent* attackComp = target->GetComponentByClass<UAttackComponent>();

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

		if (favoured == nullptr) {
			favoured = target;

			continue;
		}

		double outLengthNew;
		double outLengthFavoured;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

		NavData->CalcPathLength(Owner->GetActorLocation(), target->GetActorLocation(), outLengthNew);
		NavData->CalcPathLength(Owner->GetActorLocation(), favoured->GetActorLocation(), outLengthFavoured);

		if (outLengthFavoured > outLengthNew)
			favoured = target;
	}

	AsyncTask(ENamedThreads::GameThread, [this, favoured]() {
		if (favoured == nullptr) {
			Owner->AIController->DefaultAction();

			GetWorld()->GetTimerManager().ClearTimer(AttackTimer);

			return;
		}

		if (*ProjectileClass || MeleeableEnemies.Contains(favoured))
			Attack(favoured);
		else
			Owner->AIController->AIMoveTo(favoured);
	});
}

void UAttackComponent::CanHit(AActor* Target)
{
	if (CanAttack == ECanAttack::Invalid)
		return;

	MeleeableEnemies.Add(Target);

	if (CurrentTarget == nullptr)
		Attack(Target);
}

void UAttackComponent::Attack(AActor* Target)
{
	CurrentTarget = Target;

	if (CurrentTarget == nullptr || !CurrentTarget->IsValidLowLevelFast()) {
		Async(EAsyncExecution::Thread, [this]() { PickTarget(); });

		return;
	}

	UHealthComponent* hp = CurrentTarget->GetComponentByClass<UHealthComponent>();

	if (hp->Health == 0) {
		Async(EAsyncExecution::Thread, [this]() { PickTarget(); });

		return;
	}

	Cast<AAI>(Owner)->AIController->StopMovement();

	float time = 1.0f;

	if (Owner->IsA<ACitizen>())
		time /= FMath::LogX(100.0f, FMath::Clamp(Cast<ACitizen>(Owner)->Energy, 2, 100));

	FTimerHandle AnimTimer;

	if (*ProjectileClass) {
		if (RangeAnim->IsValidLowLevelFast()) {
			RangeAnim->RateScale = 0.5f / time;

			Cast<ACitizen>(Owner)->GetMesh()->PlayAnimation(RangeAnim, false);
		}

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, FTimerDelegate::CreateUObject(this, &UAttackComponent::Throw, CurrentTarget), time, false);
	}
	else {
		if (MeleeAnim->IsValidLowLevelFast()) {
			MeleeAnim->RateScale = 0.5f / time;

			Cast<ACitizen>(Owner)->GetMesh()->PlayAnimation(MeleeAnim, false);
		}

		GetWorld()->GetTimerManager().SetTimer(AnimTimer, FTimerDelegate::CreateUObject(this, &UAttackComponent::Melee, CurrentTarget), time / 2, false);
	}

	CanAttack = ECanAttack::Timer;

	GetWorld()->GetTimerManager().SetTimer(AttackTimer, FTimerDelegate::CreateUObject(this, &UAttackComponent::Attack, CurrentTarget), time, false);
}

void UAttackComponent::Throw(AActor* Target)
{
	if (CanAttack == ECanAttack::Invalid)
		return;

	Async(EAsyncExecution::Thread, [this, Target]() {
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
	});
}

void UAttackComponent::Melee(AActor* Target)
{
	if (CanAttack == ECanAttack::Invalid)
		return;

	UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

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

		Async(EAsyncExecution::Thread, [this, ai]() { ai->AttackComponent->PickTarget(); });
	}

	OverlappingEnemies.Empty();
	MeleeableEnemies.Empty();

	CanAttack = ECanAttack::Invalid;

	AsyncTask(ENamedThreads::GameThread, [this]() { GetWorld()->GetTimerManager().ClearTimer(AttackTimer); });
}