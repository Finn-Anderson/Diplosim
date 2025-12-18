#include "AI/AI.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"

#include "AIMovementComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Buildings/Building.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Universal/HealthComponent.h"

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

	AIController = CreateDefaultSubobject<ADiplosimAIController>(TEXT("AIController"));
	AIController->SetOwner(this);

	InitialRange = 400.0f;
	Range = InitialRange;
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

bool AAI::CanReach(AActor* Actor, float Reach, int32 Instance)
{
	FVector location = Camera->GetTargetActorLocation(Actor);

	if (Actor->IsA<AResource>()) {
		FTransform transform;
		Cast<AResource>(Actor)->ResourceHISM->GetInstanceTransform(Instance, transform);

		location = transform.GetLocation();
	}
	else if (Actor->IsA<ABuilding>())
		Cast<ABuilding>(Actor)->BuildingMesh->GetClosestPointOnCollision(MovementComponent->Transform.GetLocation(), location);

	return Reach >= FVector::Dist(MovementComponent->Transform.GetLocation(), location);
}