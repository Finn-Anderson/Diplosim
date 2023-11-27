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

void AEnemy::OnDetectOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<ACitizen>()) {
		OverlappingEnemies.Add(OtherActor);

		SetActorTickEnabled(true);
	}
}

void AEnemy::OnDetectOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnDetectOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (OverlappingEnemies.IsEmpty() && OtherActor->IsA<ACitizen>()) {
		TArray<AActor*> brochs;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

		MoveTo(brochs[0]);
	}
}