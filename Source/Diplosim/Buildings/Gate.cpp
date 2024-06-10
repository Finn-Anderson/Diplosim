#include "Buildings/Gate.h"

#include "Components/SphereComponent.h"

#include "AI/Enemy.h"

AGate::AGate()
{
	BuildingMesh->bFillCollisionUnderneathForNavmesh = false;

	EnemyDetectionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("EnemyDetectionComponent"));
	EnemyDetectionComponent->SetCollisionProfileName("Spectator", true);
	EnemyDetectionComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	EnemyDetectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	EnemyDetectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	EnemyDetectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	EnemyDetectionComponent->SetSphereRadius(400.0f);
	EnemyDetectionComponent->SetupAttachment(RootComponent);

	RightGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightGate"));
	RightGate->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RightGate->SetupAttachment(BuildingMesh, "RightGateSocket");
	RightGate->SetCanEverAffectNavigation(true);

	LeftGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftGate"));
	LeftGate->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LeftGate->SetupAttachment(BuildingMesh, "LeftGateSocket");
	LeftGate->SetCanEverAffectNavigation(true);

	Enemies = 0;

	bOpen = false;
}

void AGate::BeginPlay()
{
	Super::BeginPlay();

	EnemyDetectionComponent->OnComponentBeginOverlap.AddDynamic(this, &AGate::OnGateBeginOverlap);
}

void AGate::OnGateBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->IsA<AEnemy>() || GetCitizensAtBuilding().IsEmpty())
		return;

	Enemies++;

	CloseGate();
}

void AGate::OnGateEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Enemies--;

	OpenGate();
}

void AGate::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	OpenGate();
}

void AGate::OpenGate()
{
	if (GetCitizensAtBuilding().IsEmpty())
		return;

	if (Enemies > 0 || bOpen)
		return;

	RightGate->PlayAnimation(OpenAnim, false);
	LeftGate->PlayAnimation(OpenAnim, false);

	bOpen = true;

	FTimerHandle updateTimer;
	GetWorld()->GetTimerManager().SetTimer(updateTimer, this, &AGate::UpdateNavigation, 3.0f, false);
}

void AGate::CloseGate()
{
	if (GetCitizensAtBuilding().IsEmpty())
		return;

	if (Enemies != 1 || !bOpen)
		return;

	RightGate->PlayAnimation(CloseAnim, false);
	LeftGate->PlayAnimation(CloseAnim, false);

	bOpen = false;

	FTimerHandle updateTimer;
	GetWorld()->GetTimerManager().SetTimer(updateTimer, this, &AGate::UpdateNavigation, 3.0f, false);
}

void AGate::UpdateNavigation()
{
	RightGate->SetCanEverAffectNavigation(!RightGate->CanEverAffectNavigation());
	LeftGate->SetCanEverAffectNavigation(!LeftGate->CanEverAffectNavigation());
}