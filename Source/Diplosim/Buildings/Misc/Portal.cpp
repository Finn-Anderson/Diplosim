#include "Buildings/Misc/Portal.h"

#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"

APortal::APortal()
{
	
}

void APortal::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (HealthComponent->GetHealth() == 0 || !IsValid(Citizen->AIController->MoveRequest.GetUltimateGoalActor()))
		return;

	AActor* linkedPortal = Citizen->AIController->MoveRequest.GetLinkedPortal();

	if (!IsValid(linkedPortal) || linkedPortal->FindComponentByClass<UHealthComponent>()->GetHealth() == 0) {
		Citizen->AIController->DefaultAction();

		return;
	}

	Citizen->MovementComponent->Transform.SetLocation(linkedPortal->GetActorLocation());

	Citizen->AIController->AIMoveTo(Citizen->AIController->MoveRequest.GetUltimateGoalActor());
}