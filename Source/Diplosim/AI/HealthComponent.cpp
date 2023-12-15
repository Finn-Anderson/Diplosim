#include "AI/HealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "AttackComponent.h"
#include "Buildings/Work.h"
#include "Buildings/House.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, FVector DeathInstigatorLocation)
{
	Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

	if (Health == 0) {
		Death(DeathInstigatorLocation);
	}
}

void UHealthComponent::Death(FVector DeathInstigatorLocation)
{
	AActor* actor = GetOwner();

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

	if (actor->IsA<AAI>()) {
		USkeletalMeshComponent* mesh = actor->GetComponentByClass<USkeletalMeshComponent>();

		mesh->SetSimulatePhysics(true);

		const FVector direction = (actor->GetActorLocation() - DeathInstigatorLocation) * 10;
		mesh->SetPhysicsLinearVelocity(direction, false);
	}

	if (actor->IsA<ABuilding>()) {
		TArray<AActor*> foundCitizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), foundCitizens);

		for (AActor* c : foundCitizens) {
			ACitizen* citizen = Cast<ACitizen>(c);

			if (citizen->Building.BuildingAt != actor)
				continue;

			citizen->HealthComponent->TakeHealth(100, DeathInstigatorLocation);
		}
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