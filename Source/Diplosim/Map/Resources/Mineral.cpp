#include "Mineral.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"

AMineral::AMineral()
{
	ResourceHISM->SetCanEverAffectNavigation(true);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Block);
	ResourceHISM->bFillCollisionUnderneathForNavmesh = true;
	ResourceHISM->NumCustomDataFloats = 1;
}