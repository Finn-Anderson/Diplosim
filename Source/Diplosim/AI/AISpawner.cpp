#include "AI/AISpawner.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"

AAISpawner::AAISpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh"));
	SpawnerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SpawnerMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	SpawnerMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Block);
	SpawnerMesh->SetCanEverAffectNavigation(true);
	SpawnerMesh->SetGenerateOverlapEvents(false);
	SpawnerMesh->bFillCollisionUnderneathForNavmesh = true;
	SpawnerMesh->PrimaryComponentTick.bCanEverTick = false;

	RootComponent = SpawnerMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 125;
	HealthComponent->Health = HealthComponent->MaxHealth;

	Camera = nullptr;
	IncrementSpawned = 0;
}

void AAISpawner::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->SnakeSpawners.Add(this);

	SpawnAI();
}

void AAISpawner::SpawnAI()
{
	FActorSpawnParameters params;
	params.bNoFail = true;

	FTransform transform;
	transform.SetLocation(GetActorLocation());

	AEnemy* enemy = GetWorld()->SpawnActor<AEnemy>(AIClass->GetClass(), FVector::Zero(), FRotator::ZeroRotator, params);
	Camera->Grid->AIVisualiser->AddInstance(enemy, Camera->Grid->AIVisualiser->HISMSnake, transform);

	enemy->SpawnLocation = transform.GetLocation();

	GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Snakes.Add(enemy);

	IncrementSpawned++;

	Camera->TimerManager->CreateTimer("AISpawner", this, Camera->Stream.FRandRange(300.0f, 600.0f), "SpawnAI", {}, false, true);
}

void AAISpawner::ClearedNest(FFactionStruct* Faction)
{
	Camera->ResourceManager->AddUniversalResource(Faction, Camera->ResourceManager->Money, Camera->Stream.RandRange(IncrementSpawned * 5, IncrementSpawned * 3 * 5));
}