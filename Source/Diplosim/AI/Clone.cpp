#include "AI/Clone.h"

#include "Components/CapsuleComponent.h"
#include "Player/Camera.h"
#include "Map/Grid.h"

AClone::AClone()
{
	Capsule->SetCapsuleSize(9.0f, 11.5f);
	Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2);

	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.5f));

	Range = 100000.0f;
}

void AClone::BeginPlay()
{
	Super::BeginPlay();

	float r = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);
	float g = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);
	float b = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	material->SetScalarParameterValue("Emissive", 2.0f);
	Mesh->SetMaterial(0, material);
}