#include "Buildings/Wall.h"

#include "AIController.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"

AWall::AWall()
{
	BuildingMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	bHideCitizen = false;
}

void AWall::StoreSocketLocations()
{
	TArray<FName> sockets = BuildingMesh->GetAllSocketNames();

	if (sockets.Contains("Entrace")) {
		sockets.Remove("Entrance");
	}

	FSocketStruct socketStruct;

	for (FName socket : sockets) {
		socketStruct.Name = socket;
		socketStruct.SocketLocation = BuildingMesh->GetSocketLocation(socket);

		SocketList.Add(socketStruct);
	}
}

void AWall::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(BuildingProjectileClass);

	for (int32 i = 0; i < SocketList.Num(); i++) {
		if (SocketList[i].Citizen != nullptr)
			continue;

		SocketList[i].Citizen = Citizen;

		SocketList[i].EntranceLocation = Citizen->GetActorLocation();

		Citizen->SetActorLocation(SocketList[i].SocketLocation);

		break;
	}
}

void AWall::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(nullptr);

	for (int32 i = 0; i < SocketList.Num(); i++) {
		if (SocketList[i].Citizen != Citizen)
			continue;

		SocketList[i].Citizen = nullptr;

		Citizen->SetActorLocation(SocketList[i].EntranceLocation);

		break;
	}
}