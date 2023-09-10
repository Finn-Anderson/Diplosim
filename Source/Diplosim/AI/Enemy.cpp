#include "AI/Enemy.h"

#include "Citizen.h"
#include "HealthComponent.h"

AEnemy::AEnemy()
{

}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemy::OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		OverlappingEnemies.Add(OtherActor);

		if (!GetWorld()->GetTimerManager().IsTimerActive(DamageTimer)) {
			GetWorld()->GetTimerManager().SetTimer(DamageTimer, this, &AEnemy::Attack, TimeToAttack, true);
		}
	}
}

void AEnemy::OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingEnemies.Contains(OtherActor)) {
		OverlappingEnemies.Remove(OtherActor);

		if (OverlappingEnemies.IsEmpty()) {
			GetWorld()->GetTimerManager().ClearTimer(DamageTimer);
		}
	}
}

void AEnemy::Attack()
{
	FAttackStruct favoured;
	favoured.Actor = OverlappingEnemies[0];

	if (OverlappingEnemies[0]->IsA<ACitizen>()) {
		ACitizen* c = Cast<ACitizen>(OverlappingEnemies[0]);

		favoured.Hp = c->HealthComponent->Health;
		favoured.Dmg = c->Damage;
	}

	for (int32 i = 1; i < OverlappingEnemies.Num(); i++) {
		int32 hp;
		int32 dmg;

		if (OverlappingEnemies[i]->IsA<ACitizen>()) {
			ACitizen* c = Cast<ACitizen>(OverlappingEnemies[i]);

			hp = c->HealthComponent->Health;
			dmg = c->Damage;
		}

		float favourability = (Damage / hp) * dmg;
		float currentFavoured = (Damage / favoured.Hp) * favoured.Dmg;

		if (favourability > currentFavoured) {
			favoured.Actor = OverlappingEnemies[i];
			favoured.Hp = hp;
			favoured.Dmg = dmg;
		}
	}

	if (ProjectileClass) {
		Throw(favoured.Actor);
	}
	else {
		if (favoured.Actor->IsA<ACitizen>()) {
			ACitizen* c = Cast<ACitizen>(favoured.Actor);

			c->HealthComponent->TakeHealth(Damage, GetActorLocation());
		}
	}
}