#include "Buildings/Misc/Festival.h"

#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

#include "AI/AIMovementComponent.h"
#include "AI/Citizen.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AFestival::AFestival()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(0.01f);

	FestivalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FestivalMesh"));
	FestivalMesh->SetupAttachment(BuildingMesh);
	FestivalMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	FestivalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FestivalMesh->SetCanEverAffectNavigation(true);
	FestivalMesh->bFillCollisionUnderneathForNavmesh = true;

	SpinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpinMesh"));
	SpinMesh->SetupAttachment(FestivalMesh);
	SpinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BoxAreaAffect = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxAreaAffect"));
	BoxAreaAffect->SetupAttachment(BuildingMesh);
	BoxAreaAffect->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	BoxAreaAffect->SetBoxExtent(FVector(150.0f, 150.0f, 20.0f));
	BoxAreaAffect->SetCanEverAffectNavigation(true);
	BoxAreaAffect->bDynamicObstacle = true;

	ParticleComponent->SetupAttachment(FestivalMesh);
	ParticleComponent->SetUseAutoManageAttachment(false);

	HealthComponent->MaxHealth = 50;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Capacity = 0;
	MaxCapacity = Capacity;

	bCanHostFestival = true;

	bHideCitizen = false;
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

	SpinMesh->SetRelativeRotation(SpinMesh->GetRelativeRotation() + FRotator(0.0f, 1.0f, 0.0f));
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

void AFestival::OnBuilt()
{
	Super::OnBuilt();

	for (FEventStruct event : Camera->CitizenManager->OngoingEvents()) {
		if (event.Type != EEventType::Festival)
			continue;

		FEventTimeStruct time = Camera->CitizenManager->GetOngoingEventTimes(event);

		StartFestival(time.bFireFestival);
	}
}

void AFestival::StartFestival(bool bFireFestival)
{
	int32 index = 0;

	if (!bFireFestival)
		index = 1;

	FestivalMesh->SetStaticMesh(FestivalStruct[index].Mesh);
	FestivalMesh->UpdateNavigationBounds();

	ParticleComponent->SetAsset(FestivalStruct[index].ParticleSystem);

	float z = 2.0f;

	if (!bFireFestival) {
		TArray<FVector> locations;

		TArray<FName> socketNames = FestivalMesh->GetAllSocketNames();

		for (FName socket : socketNames) {
			FTransform transform = FestivalMesh->GetSocketTransform(socket, ERelativeTransformSpace::RTS_Actor);

			locations.Add(transform.GetLocation());
		}

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(ParticleComponent, TEXT("SpawnLocations"), locations);

		z = 1.0f;
	}

	ParticleComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, z));

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