#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/Projectile.h"
#include "AI/AttackComponent.h"
#include "Buildings/Building.h"
#include "DiplosimGameModeBase.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker)
{
	Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

	if (Health == 0)
		Death(Attacker);
}

void UHealthComponent::Death(AActor* Attacker)
{
	AActor* actor = GetOwner();

	if (actor->IsA<AAI>()) {
		USkeletalMeshComponent* mesh = actor->GetComponentByClass<USkeletalMeshComponent>();
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		for (TWeakObjectPtr<AActor> target : attackComp->OverlappingEnemies) {
			if (!target->IsValidLowLevelFast() || !target->IsA<AAI>())
				continue;

			AAI* ai = Cast<AAI>(target);

			ai->AttackComponent->OverlappingEnemies.Remove(actor);

			if (!GetWorld()->GetTimerManager().IsTimerActive(ai->AttackComponent->AttackTimer))
				ai->AttackComponent->PickTarget();
		}

		mesh->SetSimulatePhysics(true);

		const FVector direction = (actor->GetActorLocation() - Attacker->GetActorLocation()).Rotation().Vector() * 50;
		mesh->SetPhysicsLinearVelocity(direction, false);

		Cast<AAI>(actor)->DetachFromControllerPendingDestroy();
	} 
	else if (actor->IsA<ABuilding>()) {
		TArray<AActor*> foundCitizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), foundCitizens);

		for (AActor* c : foundCitizens) {
			ACitizen* citizen = Cast<ACitizen>(c);

			if (citizen->Building.BuildingAt != actor)
				continue;

			citizen->HealthComponent->TakeHealth(100, Attacker);
		}
	}

	FTimerHandle clearTimer;
	GetWorld()->GetTimerManager().SetTimer(clearTimer, FTimerDelegate::CreateUObject(this, &UHealthComponent::Clear, Attacker), 10.0f, false);
}

void UHealthComponent::Clear(AActor* Attacker)
{
	AActor* actor = GetOwner();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if (actor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Building.Employment != nullptr) {
			citizen->Building.Employment->RemoveCitizen(citizen);
		}

		if (citizen->Building.House != nullptr) {
			citizen->Building.House->RemoveCitizen(citizen);
		}

		if (citizen->BioStruct.Partner != nullptr) {
			citizen->BioStruct.Partner->BioStruct.Partner = nullptr;
			citizen->BioStruct.Partner = nullptr;
		}
	}
	else if (actor->IsA<AEnemy>()) {
		ACitizen* citizen;
		FString name;

		if (Attacker->IsA<AProjectile>()) {
			citizen = Cast<ACitizen>(Attacker->GetOwner());

			name = citizen->Building.Employment->GetName();
		}
		else {
			citizen = Cast<ACitizen>(Attacker);

			name = citizen->GetName();
		}

		gamemode->WavesData.Last().SetDiedTo(citizen, name);

		gamemode->CheckEnemiesStatus();
	}

	if (Attacker->IsA<AEnemy>())
		gamemode->WavesData.Last().NumKilled++;

	actor->Destroy();
}

int32 UHealthComponent::GetHealth()
{
	return Health;
}