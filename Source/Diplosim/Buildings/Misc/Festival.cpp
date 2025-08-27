#include "Buildings/Misc/Festival.h"

#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Components/CapsuleComponent.h"

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

	float spinRate = 1.0f;

	for (FSocketStruct &socket : SocketList) {
		if (!IsValid(socket.Citizen))
			continue;

		float distance = FVector::Dist(GetActorLocation(), socket.SocketLocation);
		
		socket.SocketRotation += FRotator(0.0f, spinRate / (1.0f + ((distance - 60.0f) / 40.0f) * 0.25f), 0.0f);

		FVector location = GetActorLocation();
		location.X += distance * FMath::Cos(socket.SocketRotation.Yaw);
		location.Y += distance * FMath::Sin(socket.SocketRotation.Yaw);
		socket.SocketLocation = location;

		socket.Citizen->MovementComponent->Transform.SetLocation(socket.SocketLocation);
		socket.Citizen->MovementComponent->Transform.SetRotation(socket.SocketRotation.Quaternion());

		break;
	}
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

	for (auto& element : Camera->CitizenManager->OngoingEvents()) {
		for (FEventStruct* event : element.Value) {
			if (event->Type != EEventType::Festival)
				continue;

			StartFestival(event->bFireFestival);
		}
	}
}

void AFestival::StoreSocketLocations()
{
	int32 tier = 0;
	int32 amount = 8;
	int32 target = 8;

	for (int32 i = 1; i <= Space; i++) {
		if (i == amount) {
			target *= 2;
			amount += target;
			tier++;
		}

		float distance = 60.0f + (40.0f * tier);

		FRotator rotation = GetActorRotation();
		rotation.Yaw += 360.0f * ((i - (amount - target)) / target);

		FVector location = GetActorLocation();
		location.X += distance * FMath::Cos(rotation.Yaw);
		location.Y += distance * FMath::Sin(rotation.Yaw);

		FSocketStruct socketStruct;
		socketStruct.SocketLocation = location;
		socketStruct.SocketRotation = rotation;

		SocketList.Add(socketStruct);
	}
}

void AFestival::StartFestival(bool bFireFestival)
{
	int32 index = 0;

	if (!bFireFestival)
		index = 1;

	FestivalMesh->SetStaticMesh(FestivalStruct[index].Mesh);
	FestivalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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

	FestivalMesh->SetRelativeScale3D(FVector(z));

	ParticleComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, z));

	ParticleComponent->Activate();

	SetActorTickEnabled(true);
}

void AFestival::StopFestival()
{
	FestivalMesh->SetStaticMesh(nullptr);
	FestivalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FestivalMesh->UpdateNavigationBounds();

	ParticleComponent->Deactivate();

	for (ACitizen* visitor : GetVisitors(Occupied[0].Occupant)) {
		visitor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		RemoveVisitor(Occupied[0].Occupant, visitor);
	}

	SetActorTickEnabled(false);
}