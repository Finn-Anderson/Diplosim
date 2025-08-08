#include "Buildings/Misc/Special/CloneLab.h"

#include "AI/Clone.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
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

	AClone* clone = GetWorld()->SpawnActor<AClone>(Clone, FVector::Zero(), FRotator::ZeroRotator, params);
	Camera->CitizenManager->Clones.Add(clone);
	Camera->Grid->AIVisualiser->AddInstance(clone, Camera->Grid->AIVisualiser->HISMClone, transform);
}