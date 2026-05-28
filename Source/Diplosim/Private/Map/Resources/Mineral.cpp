#include "Map/Resources/Mineral.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AMineral::AMineral()
{
	ResourceHISM->SetCanEverAffectNavigation(true);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Block);
	ResourceHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);
	ResourceHISM->bFillCollisionUnderneathForNavmesh = true;
	ResourceHISM->NumCustomDataFloats = 1;
}