#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AttackComponent.h"
#include "AI/Projectile.h"
#include "Buildings/Work.h"
#include "Buildings/House.h"
#include "DiplosimGameModeBase.h"
#include "InteractableInterface.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);

	UInteractableComponent* interactableComp = GetOwner()->GetComponentByClass<UInteractableComponent>();
	interactableComp->SetHP();
	interactableComp->ExecuteEditEvent("HP");
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker)
{
	Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

	UInteractableComponent* interactableComp = GetOwner()->GetComponentByClass<UInteractableComponent>();
	interactableComp->SetHP();
	interactableComp->ExecuteEditEvent("HP");

	if (Health == 0) {
		Death(Attacker);
	}
}

void UHealthComponent::Death(AActor* Attacker)
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
		bool bIsProjectile = false;

		if (Attacker->IsA<AProjectile>())
			bIsProjectile = true;

		gamemode->WavesData.Last().SetDiedTo(Cast<ACitizen>(Attacker), bIsProjectile);

		gamemode->CheckEnemiesStatus();
	}

	if (actor->IsA<AAI>()) {
		USkeletalMeshComponent* mesh = actor->GetComponentByClass<USkeletalMeshComponent>();

		mesh->SetSimulatePhysics(true);

		const FVector direction = (actor->GetActorLocation() - (Attacker->GetActorLocation() + actor->GetVelocity())) * 10;
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

	if (Attacker->IsA<AEnemy>()) {
		gamemode->WavesData.Last().NumKilled++;
	}

	FTimerHandle clearTimer;
	GetWorld()->GetTimerManager().SetTimer(clearTimer, this, &UHealthComponent::ClearRagdoll, 10.0f, false);
}

void UHealthComponent::ClearRagdoll()
{
	GetOwner()->Destroy();
}

int32 UHealthComponent::GetHealth()
{
	return Health;
}