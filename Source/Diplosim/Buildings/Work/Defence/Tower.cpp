#include "Buildings/Work/Defence/Tower.h"

#include "Components/SphereComponent.h"

#include "AI/AttackComponent.h"
#include "Universal/HealthComponent.h"

ATower::ATower()
{
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;
	
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->AttackTime = 5.0f;

	RangeComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	RangeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);
}

void ATower::BeginPlay()
{
	Super::BeginPlay();

	FHashedMaterialParameterInfo matInfo;
	matInfo.Name = FScriptName("Colour");

	BuildingMesh->GetMaterial(0)->GetVectorParameterValue(matInfo, Colour);

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &ATower::OnTowerOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &ATower::OnTowerOverlapEnd);
}

void ATower::OnTowerOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AttackComponent->OverlappingEnemies.Add(OtherActor);

	if (AttackComponent->OverlappingEnemies.Num() == 1)
		AttackComponent->SetComponentTickEnabled(true);
}

void ATower::OnTowerOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AttackComponent->OverlappingEnemies.Contains(OtherActor))
		AttackComponent->OverlappingEnemies.Remove(OtherActor);
}