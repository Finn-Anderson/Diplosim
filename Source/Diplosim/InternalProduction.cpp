#include "InternalProduction.h"

#include "Resource.h"

void AInternalProduction::Production(ACitizen* Citizen)
{
	AResource* r = GetWorld()->SpawnActor<AResource>(ActorToGetResource);

	Storage += r->GetYield();

	r->Destroy();
}