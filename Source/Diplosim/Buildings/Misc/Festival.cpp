#include "Buildings/Misc/Festival.h"

#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"

AFestival::AFestival()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(0.05f);
	
	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	bCanHostFestival = true;

	FestivalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FestivalMesh"));
	FestivalMesh->SetupAttachment(BuildingMesh);
	FestivalMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	FestivalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FestivalMesh->SetCanEverAffectNavigation(true);
	FestivalMesh->bFillCollisionUnderneathForNavmesh = true;

	SpinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpinMesh"));
	SpinMesh->SetupAttachment(BuildingMesh);
	SpinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
	BoxAreaAffect->SetCanEverAffectNavigation(true);
	BoxAreaAffect->SetupAttachment(RootComponent);
	BoxAreaAffect->bDynamicObstacle = true;
}

void AFestival::BeginPlay()
{
	Super::BeginPlay();

	BoxAreaAffect->OnComponentBeginOverlap.AddDynamic(this, &AFestival::OnCitizenOverlapBegin);
	BoxAreaAffect->OnComponentEndOverlap.AddDynamic(this, &AFestival::OnCitizenOverlapEnd);
}

void AFestival::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime > 1.0f)
		return;

	SpinMesh->SetRelativeRotation(SpinMesh->GetRelativeRotation() + FRotator(0.0f, 0.1f, 0.0f));
}

void AFestival::OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->IsA<AAI>())
		return;

	Cast<AAI>(OtherActor)->MovementComponent->SpeedMultiplier += (0.15f * Tier);
}

void AFestival::OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->IsA<AAI>())
		return;

	Cast<AAI>(OtherActor)->MovementComponent->SpeedMultiplier -= (0.15f * Tier);
}

void AFestival::StartFestival(bool bFireFestival)
{
	int32 index = 0;

	if (!bFireFestival)
		index = 1;

	FestivalMesh->SetStaticMesh(FestivalStruct[index].Mesh);

	ParticleComponent->SetAsset(FestivalStruct[index].ParticleSystem);
	ParticleComponent->Activate();

	SetActorTickEnabled(true);
}

void AFestival::StopFestival()
{
	FestivalMesh->SetStaticMesh(nullptr);

	ParticleComponent->Deactivate();

	for (ACitizen* visitor : GetVisitors(Occupied[0].Occupant)) {
		visitor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		RemoveVisitor(Occupied[0].Occupant, visitor);
	}

	SetActorTickEnabled(false);
}