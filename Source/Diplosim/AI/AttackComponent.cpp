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

	CanAttack = ECanAttack::Valid;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AAI>(GetOwner());

	if (Owner->IsA<ACitizen>()) {
		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		RangeComponent->SetAreaClassOverride(gamemode->NavAreaThreat);

		if (!Cast<ACitizen>(Owner)->CanWork())
			CanAttack = ECanAttack::Invalid;
	}

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &UAttackComponent::OnOverlapBegin);
}

void UAttackComponent::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CanAttack == ECanAttack::Invalid || (!*ProjectileClass && !Owner->AIController->CanMoveTo(OtherActor->GetActorLocation())))
		return;

	for (TSubclassOf<AActor> enemyClass : EnemyClasses) {
		if (!OtherActor->IsA(enemyClass))
			continue;

		OverlappingEnemies.Add(OtherActor);

		break;
	}

	if (!OverlappingEnemies.Contains(OtherActor))
		return;

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
	UHealthComponent* ownerHP = Owner->GetComponentByClass<UHealthComponent>();

	if (ownerHP->Health == 0) {
		CanAttack = ECanAttack::Invalid;

		return;
	}

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

		float hp = healthComp->Health;
		float dmg = 1.0f;
		double outLength;

		if (hp == 0) {
			OverlappingEnemies.Remove(target);

			continue;
		}

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetDefaultNavDataInstance();

		if (attackComp && attackComp->CanAttack != ECanAttack::Invalid) {
			if (*attackComp->ProjectileClass) {
				dmg = Cast<AProjectile>(attackComp->ProjectileClass->GetDefaultObject())->Damage;
			}
			else {
				dmg = attackComp->Damage;
			}
		}
		else if (target->IsA<AWall>())
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

	if (favoured.Actor == nullptr) {
		AsyncTask(ENamedThreads::GameThread, [this]() {
			Owner->AIController->DefaultAction();

			GetWorld()->GetTimerManager().ClearTimer(AttackTimer); 
		});

		return;
	}

	if (CanAttack == ECanAttack::Valid && (*ProjectileClass || MeleeableEnemies.Contains(favoured.Actor)))
		Attack(favoured.Actor);
	else
		AsyncTask(ENamedThreads::GameThread, [this, favoured]() { Owner->AIController->AIMoveTo(favoured.Actor); });
}

void UAttackComponent::CanHit(AActor* Target)
{
	if (Owner->IsA<ACitizen>() && !Cast<ACitizen>(Owner)->CanWork())
		return;

	MeleeableEnemies.Add(Target);

	Async(EAsyncExecution::Thread, [this]() { PickTarget(); });
}

void UAttackComponent::Attack(AActor* Target)
{
	Cast<AAI>(Owner)->AIController->StopMovement();

	if (*ProjectileClass) {
		Throw(Target);
	}
	else {
		UHealthComponent* healthComp = Target->GetComponentByClass<UHealthComponent>();

		AsyncTask(ENamedThreads::GameThread, [this, healthComp]() { healthComp->TakeHealth(Damage, Owner); });
	}

	float time = TimeToAttack;

	if (Owner->IsA<ACitizen>())
		time *= FMath::LogX(100.0f, FMath::Clamp(Cast<ACitizen>(Owner)->Energy, 2, 100));

	CanAttack = ECanAttack::Timer;

	AsyncTask(ENamedThreads::GameThread, [this, time]() { GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &UAttackComponent::ClearTimer, time, false); });
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

void UAttackComponent::ClearTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(AttackTimer);

	CanAttack = ECanAttack::Valid;

	Async(EAsyncExecution::Thread, [this]() { PickTarget(); });
}

void UAttackComponent::ClearAttacks()
{
	for (TWeakObjectPtr<AActor> target : OverlappingEnemies) {
		if (!target->IsValidLowLevelFast() || !target->IsA<AAI>())
			continue;

		AAI* ai = Cast<AAI>(target);

		ai->AttackComponent->OverlappingEnemies.Remove(Owner);
		ai->AttackComponent->MeleeableEnemies.Remove(Owner);

		ai->AttackComponent->PickTarget();
	}

	AsyncTask(ENamedThreads::GameThread, [this]() { GetWorld()->GetTimerManager().ClearTimer(AttackTimer); });
}