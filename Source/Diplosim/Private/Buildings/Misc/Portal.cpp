#include "Buildings/Misc/Portal.h"

#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "Universal/HealthComponent.h"

APortal::APortal()
{
	
}

void APortal::Enter(ACitizen* Citizen)
{
	AActor* linkedPortal = Citizen->AIController->MoveRequest.GetLinkedPortal();

	if (!IsValid(linkedPortal)) {
		Citizen->AIController->DefaultAction();

		return;
	}

	Citizen->MovementComponent->SetPosition = linkedPortal->GetActorLocation();
	Citizen->MovementComponent->ZOffset = 0.0f;
}