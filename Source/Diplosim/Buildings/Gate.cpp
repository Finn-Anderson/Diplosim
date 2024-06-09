#include "Buildings/Gate.h"

#include "Components/SphereComponent.h"

#include "AI/Enemy.h"

AGate::AGate()
{
	EnemyDetectionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("EnemyDetectionComponent"));
	EnemyDetectionComponent->SetCollisionProfileName("Spectator", true);
	EnemyDetectionComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	EnemyDetectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	EnemyDetectionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	EnemyDetectionComponent->SetSphereRadius(400.0f);

	RightGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightGate"));
	RightGate->SetCollisionProfileName("Spectator", true);
	RightGate->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RightGate->SetupAttachment(BuildingMesh, "RightGateSocket");

	LeftGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftGate"));
	LeftGate->SetCollisionProfileName("Spectator", true);
	LeftGate->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	LeftGate->SetupAttachment(BuildingMesh, "LeftGateSocket");

	Enemies = 0;

	bOpen = false;
}

void AGate::BeginPlay()
{
	Super::BeginPlay();

	EnemyDetectionComponent->OnComponentBeginOverlap.AddDynamic(this, &AGate::CloseGate);
}

void AGate::CloseGate(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->IsA<AEnemy>() || GetCitizensAtBuilding().IsEmpty())
		return;

	Enemies++;

	if (Enemies > 1 || !bOpen)
		return;

	RightGate->PlayAnimation(CloseAnim, false);
	LeftGate->PlayAnimation(CloseAnim, false);

	bOpen = false;
}

void AGate::OpenGate(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Enemies--;

	if (Enemies > 0 || bOpen)
		return;

	RightGate->PlayAnimation(OpenAnim, false);
	LeftGate->PlayAnimation(OpenAnim, false);

	bOpen = true;
}

void AGate::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	if (Enemies > 0 || bOpen)
		return;

	RightGate->PlayAnimation(OpenAnim, false);
	LeftGate->PlayAnimation(OpenAnim, false);

	bOpen = true;
}