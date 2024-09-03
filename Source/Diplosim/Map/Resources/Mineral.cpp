#include "Mineral.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"

AMineral::AMineral()
{
	ResourceHISM->SetCanEverAffectNavigation(true);
	ResourceHISM->bFillCollisionUnderneathForNavmesh = true;
}