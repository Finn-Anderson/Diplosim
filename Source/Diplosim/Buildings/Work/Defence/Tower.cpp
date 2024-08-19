#include "Buildings/Work/Defence/Tower.h"

#include "Components/SphereComponent.h"

#include "AI/AttackComponent.h"

ATower::ATower()
{
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);
}

void ATower::BeginPlay()
{
	Super::BeginPlay();

	float r = 0.0f;
	float g = 1.0f;
	float b = 1.0f;

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(BuildingMesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	material->SetScalarParameterValue("Emissiveness", Emissiveness);
	BuildingMesh->SetMaterial(0, material);
}