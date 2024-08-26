#include "Buildings/Work/Defence/Tower.h"

#include "Components/SphereComponent.h"

#include "AI/AttackComponent.h"

ATower::ATower()
{
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);
	AttackComponent->AttackTime = 5.0f;
}

void ATower::BeginPlay()
{
	Super::BeginPlay();

	FHashedMaterialParameterInfo matInfo;
	matInfo.Name = FScriptName("Colour");

	BuildingMesh->GetMaterial(0)->GetVectorParameterValue(matInfo, Colour);
}