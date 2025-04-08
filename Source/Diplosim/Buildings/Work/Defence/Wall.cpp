#include "Buildings/Work/Defence/Wall.h"

#include "Components/DecalComponent.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"
#include "Universal/HealthComponent.h"

AWall::AWall()
{
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	HealthComponent->MaxHealth = 200;
	HealthComponent->Health = HealthComponent->MaxHealth;

	DecalComponent->SetVisibility(true);

	bHideCitizen = false;

	DecalComponent->DecalSize = FVector(400.0f, 400.0f, 400.0f);
}

void AWall::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(BuildingProjectileClass);
}

void AWall::Leave(ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	Citizen->AttackComponent->SetProjectileClass(nullptr);
}

void AWall::SetRotationMesh(int32 yaw)
{
	if (SeedNum != 0)
		return;

	FVector scale = FVector(1.0f);

	if (yaw % 90 != 0)
		scale = FVector(1.415f, 1.0f, 1.0f);
		
	BuildingMesh->SetRelativeScale3D(scale);
}