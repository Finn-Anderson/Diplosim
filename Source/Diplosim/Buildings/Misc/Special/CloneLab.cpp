#include "Buildings/Misc/Special/CloneLab.h"

#include "AI/Clone.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

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

	AClone* clone = GetWorld()->SpawnActor<AClone>(Clone, BuildingMesh->GetSocketLocation("Entrance"), GetActorRotation(), params);
	Camera->CitizenManager->Clones.Add(clone);
}