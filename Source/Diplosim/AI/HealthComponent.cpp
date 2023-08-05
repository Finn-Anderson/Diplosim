#include "AI/HealthComponent.h"

#include "Citizen.h"
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
		ACitizen* c = Cast<ACitizen>(actor);

		if (c->Employment != nullptr) {
			c->Employment->RemoveCitizen(c);
		}

		if (c->House != nullptr) {
			c->House->RemoveCitizen(c);
		}

		if (c->Partner != nullptr) {
			c->Partner->Partner = nullptr;
			c->Partner = nullptr;
		}
	}

	UStaticMeshComponent* mesh = actor->GetComponentByClass<UStaticMeshComponent>();

	mesh->SetSimulatePhysics(true);

	const FVector direction = actor->GetActorLocation() + DeathInstigatorLocation;
	mesh->SetPhysicsLinearVelocity(direction, false);

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