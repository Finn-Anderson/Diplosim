#include "Citizen.h"

#include "AIController.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	House = nullptr;
	Employment = nullptr;

	Carrying = nullptr;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();
	
	AAIController* aiController = Cast<AAIController>(GetController());
	//aiController->MoveToActor(Player, 10.0f, true);

}