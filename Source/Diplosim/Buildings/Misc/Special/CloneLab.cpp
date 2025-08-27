#include "Buildings/Misc/Special/CloneLab.h"

#include "AI/Clone.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

ACloneLab::ACloneLab()
{
	TimeLength = 30.0f;
}

void ACloneLab::Production(ACitizen* Citizen)
{
	if (Camera->CitizenManager->Enemies.IsEmpty())
		return;
	
	Super::Production(Citizen);

	FActorSpawnParameters params;
	params.bNoFail = true;

	FTransform transform;
	transform.SetLocation(BuildingMesh->GetSocketLocation("Entrance"));
	transform.SetRotation(GetActorQuat());

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	AClone* clone = GetWorld()->SpawnActor<AClone>(Clone, FVector::Zero(), FRotator::ZeroRotator, params);
	faction->Clones.Add(clone);
	Camera->Grid->AIVisualiser->AddInstance(clone, Camera->Grid->AIVisualiser->HISMClone, transform);
}