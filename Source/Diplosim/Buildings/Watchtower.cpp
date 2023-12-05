#include "Buildings/Watchtower.h"

#include "Components/SphereComponent.h"

#include "AttackComponent.h"
#include "AI/Citizen.h"

AWatchtower::AWatchtower()
{
	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	AttackComponent->RangeComponent->SetupAttachment(RootComponent);

	BaseRange = AttackComponent->RangeComponent->GetUnscaledSphereRadius();
}

void AWatchtower::BeginPlay()
{
	Super::BeginPlay();

	if (BuildStatus == EBuildStatus::Blueprint) {
		AttackComponent->RangeComponent->SetHiddenInGame(false);
	}
}

void AWatchtower::HideRangeComponent()
{
	AttackComponent->RangeComponent->SetHiddenInGame(true);
}

void AWatchtower::SetRange(float Z)
{
	int32 elevationLevel = FMath::Floor(Z) * 2.0f;

	AttackComponent->RangeComponent->SetSphereRadius(BaseRange + elevationLevel);
}