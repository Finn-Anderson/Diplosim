#include "AI/AI.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "Buildings/Building.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"

#include "Buildings/Work/Production/ExternalProduction.h"

AAI::AAI()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->HitAudioComponent->SetupAttachment(RootComponent);

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));

	MovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("MovementComponent"));

	AIController = CreateDefaultSubobject<UDiplosimAIController>(TEXT("AIController"));

	InitialRange = 400.0f;
	Range = InitialRange;
	ReachPercentageOfRange = 6.75f;
}

void AAI::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	MovementComponent->AI = this;
	MovementComponent->AIVisualiser = Camera->Grid->AIVisualiser;

	AIController->Camera = Camera;
	AIController->AI = this;

	AttackComponent->Camera = Camera;

	Colour = FLinearColor(Camera->Stream.FRandRange(0.0f, 1.0f), Camera->Stream.FRandRange(0.0f, 1.0f), Camera->Stream.FRandRange(0.0f, 1.0f));
}

void AAI::MoveToBroch()
{
	ABuilding* target = nullptr;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	if (faction != nullptr)
		Camera->ArmyManager->MoveToTarget(faction, { Cast<ACitizen>(this) });
	else
		for (FFactionStruct& f : Camera->ConquestManager->Factions)
			target = Camera->ArmyManager->MoveArmyMember(&f, this, true, target);

	AIController->AIMoveTo(target);
}

bool AAI::CanReach(AActor* Actor, float Reach, FVector Location)
{
	FVector movementLocation = MovementComponent->Transform.GetLocation();

	if (Location == FVector::Zero()) {
		if (Actor->IsA<ABuilding>())
			Cast<ABuilding>(Actor)->BuildingMesh->GetClosestPointOnCollision(movementLocation, Location);
		else if (Actor->IsA<AAISpawner>())
			Cast<AAISpawner>(Actor)->SpawnerMesh->GetClosestPointOnCollision(movementLocation, Location);
		else
			Location = AIController->GetActualLocation(Actor);
	}

	return Reach >= FVector::Dist(movementLocation, Location);
}

float AAI::GetReach()
{
	return Range * (ReachPercentageOfRange / 100.0f);
}
