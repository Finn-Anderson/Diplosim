#include "Buildings/Work/Defence/Wall.h"

#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"

AWall::AWall()
{
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	DecalComponent->SetVisibility(true);

	bHideCitizen = false;

	DecalComponent->DecalSize = FVector(400.0f, 400.0f, 400.0f);
}

void AWall::StoreSocketLocations()
{
	TArray<FName> sockets = BuildingMesh->GetAllSocketNames();

	FSocketStruct socketStruct;

	for (FName socket : sockets) {
		if (socket == "LeftGateSocket" || socket == "RightGateSocket" || socket == "Entrance")
			continue;

		socketStruct.Name = socket;
		socketStruct.SocketLocation = BuildingMesh->GetSocketLocation(socket);
		socketStruct.SocketRotation = BuildingMesh->GetSocketRotation(socket);

		SocketList.Add(socketStruct);
	}
}

void AWall::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	Citizen->Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);

	Citizen->AttackComponent->SetProjectileClass(BuildingProjectileClass);

	for (int32 i = 0; i < SocketList.Num(); i++) {
		if (SocketList[i].Citizen != nullptr)
			continue;

		SocketList[i].Citizen = Citizen;

		SocketList[i].EntranceLocation = Citizen->GetActorLocation();

		Citizen->SetActorLocation(SocketList[i].SocketLocation);
		Citizen->SetActorRotation(SocketList[i].SocketRotation);

		break;
	}
}

void AWall::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!Occupied.Contains(Citizen))
		return;

	Citizen->Capsule->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

	Citizen->AttackComponent->SetProjectileClass(nullptr);

	for (int32 i = 0; i < SocketList.Num(); i++) {
		if (SocketList[i].Citizen != Citizen)
			continue;

		SocketList[i].Citizen = nullptr;

		Citizen->SetActorLocation(SocketList[i].EntranceLocation);

		break;
	}
}