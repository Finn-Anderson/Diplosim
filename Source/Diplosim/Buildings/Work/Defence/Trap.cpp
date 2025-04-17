#include "Buildings/Work/Defence/Trap.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"

#include "AI/Enemy.h"
#include "Universal/HealthComponent.h"

ATrap::ATrap()
{
	PrimaryActorTick.bCanEverTick = false;

	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	RangeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetupAttachment(BuildingMesh);
	RangeComponent->SetSphereRadius(150.0f);

	Damage = 300;

	FuseTime = 1.0f;

	bExplode = false;
}

void ATrap::BeginPlay()
{
	Super::BeginPlay();

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &ATrap::OnOverlapBegin);
	RangeComponent->OnComponentEndOverlap.AddDynamic(this, &ATrap::OnOverlapEnd);
}

void ATrap::OnTrapOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<AEnemy>())
		OverlappingActors.Add(OtherActor);

	FTimerHandle FFuseTimer;
	GetWorld()->GetTimerManager().SetTimer(FFuseTimer, this, &ATrap::ActivateTrap, FuseTime);
}

void ATrap::OnTrapOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappingActors.Contains(OtherActor))
		OverlappingActors.Remove(OtherActor);
}

void ATrap::ActivateTrap()
{
	for (AActor* actor : OverlappingActors) {
		UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();

		int32 dmg = Damage;

		if (bExplode) {
			float distance = FVector::Dist(GetActorLocation(), actor->GetActorLocation());

			 dmg /= FMath::Pow(FMath::LogX(50.0f, distance), 5.0f);
		}

		healthComp->TakeHealth(dmg, this);
	}

	UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, GetActorLocation(), 0.0f, 1000.0f, 1.0f);

	ParticleComponent->Activate();

	float height = BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 1.0f;

	SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, height));
	DestructionComponent->SetRelativeLocation(DestructionComponent->GetRelativeLocation() + FVector(0.0f, 0.0f, height));
	GroundDecalComponent->SetRelativeLocation(GroundDecalComponent->GetRelativeLocation() + FVector(0.0f, 0.0f, height));

	HealthComponent->Health = 0;
	HealthComponent->Clear(this);
}