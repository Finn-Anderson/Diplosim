#include "AI/Enemy.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "Buildings/Broch.h"

AEnemy::AEnemy()
{
	AIMesh->SetSimulatePhysics(true);
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float spin = FMath::Abs(GetVelocity().Length());
	SetActorRotation(GetActorRotation() + FRotator(spin, 0.0f, 0.0f));
}

void AEnemy::OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		OverlappingEnemies.Add(OtherActor);

		SetActorTickEnabled(true);
	}
}

void AEnemy::OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnEnemyOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (OverlappingEnemies.IsEmpty()) {
		TArray<AActor*> brochs;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

		MoveTo(brochs[0]);
	}
}