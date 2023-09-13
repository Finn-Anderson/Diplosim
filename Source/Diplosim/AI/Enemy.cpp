#include "AI/Enemy.h"

#include "Citizen.h"

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

		SetActorTickEnabled(true);
	}
}