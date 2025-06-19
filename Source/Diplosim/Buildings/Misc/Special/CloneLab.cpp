#include "Buildings/Misc/Special/CloneLab.h"

#include "AI/Clone.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

ACloneLab::ACloneLab()
{
	TimeLength = 30.0f;
}

void ACloneLab::OnBuilt()
{
	Super::OnBuilt();

	SetTimer();
}

void ACloneLab::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	FActorSpawnParameters params;
	params.bNoFail = true;

	AClone* clone = GetWorld()->SpawnActor<AClone>(Clone, BuildingMesh->GetSocketLocation("Entrance"), GetActorRotation(), params);
	Camera->CitizenManager->Clones.Add(clone);
}

void ACloneLab::SetTimer()
{
	if (Camera->CitizenManager->Enemies.IsEmpty()) {
		for (int32 i = Camera->CitizenManager->Clones.Num() - 1; i > -1; i--)
			Camera->CitizenManager->Clones.RemoveAt(i);

		return;
	}

	Super::SetTimer();
}