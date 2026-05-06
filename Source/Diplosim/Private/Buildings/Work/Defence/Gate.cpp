#include "Buildings/Work/Defence/Gate.h"

#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"

AGate::AGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(1 / 90.0f);

	BuildingMesh->bFillCollisionUnderneathForNavmesh = false;

	TMap<UStaticMeshComponent**, FName> gates;
	gates.Add(&RightGate, TEXT("RightGate"));
	gates.Add(&LeftGate, TEXT("LeftGate"));

	for (auto& element : gates) {
		auto gate = CreateDefaultSubobject<UStaticMeshComponent>(element.Value);
		*element.Key = gate;
		gate->SetCollisionEnabled(BuildingMesh->GetCollisionEnabled());
		gate->SetCollisionObjectType(BuildingMesh->GetCollisionObjectType());
		gate->SetCollisionResponseToChannels(BuildingMesh->GetCollisionResponseToChannels());
		gate->SetGenerateOverlapEvents(BuildingMesh->GetGenerateOverlapEvents());
		gate->SetupAttachment(BuildingMesh, *(element.Value.ToString() + "Socket"));
		gate->SetCanEverAffectNavigation(true);
		gate->SetRelativeLocation(FVector(0.0f, 0.0f, -3.5f));

		if (gate == RightGate)
			gate->SetRelativeRotation(FRotator(0.0f, 0.0f, 180.0f));
	}

	bOpen = false;
}

void AGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator rotation = FRotator(0.0f, 1.0f, 0.0f);
	if (bOpen)
		rotation *= -1.0f;

	rotation *= (1.0f / FMath::Max(FMath::Abs(RightGate->GetRelativeRotation().Yaw - 140.0f) / 40.0f * 8.0f, 2.0f));

	RightGate->SetRelativeRotation(RightGate->GetRelativeRotation() + rotation);
	LeftGate->SetRelativeRotation(LeftGate->GetRelativeRotation() - rotation);

	if (FMath::Floor(RightGate->GetRelativeRotation().Yaw) == 180.0f || FMath::CeilToInt32(RightGate->GetRelativeRotation().Yaw) == 100.0f) {
		SetActorTickEnabled(false);

		UpdateNavigation();
	}
}

void AGate::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	OpenGate();
}

void AGate::OpenGate()
{
	if (bOpen || GetCitizensAtBuilding().IsEmpty())
		return;

	SetActorTickEnabled(true);

	bOpen = true;
}

void AGate::CloseGate()
{
	if (!bOpen || GetCitizensAtBuilding().IsEmpty())
		return;

	SetActorTickEnabled(true);

	bOpen = false;
}

void AGate::UpdateNavigation()
{
	RightGate->SetCanEverAffectNavigation(!bOpen);
	LeftGate->SetCanEverAffectNavigation(!bOpen);
}