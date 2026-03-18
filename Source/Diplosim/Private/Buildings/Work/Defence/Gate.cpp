#include "Buildings/Work/Defence/Gate.h"

#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"

AGate::AGate()
{
	BuildingMesh->bFillCollisionUnderneathForNavmesh = false;

	TMap<USkeletalMeshComponent**, FName> gates;
	gates.Add(&RightGate, TEXT("RightGate"));
	gates.Add(&LeftGate, TEXT("LeftGate"));

	for (auto& element : gates) {
		auto hism = CreateDefaultSubobject<USkeletalMeshComponent>(element.Value);
		*element.Key = hism;
		hism->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		hism->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
		hism->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
		hism->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
		hism->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		hism->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
		hism->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		hism->SetupAttachment(BuildingMesh, *(element.Value.ToString() + "Socket"));
		hism->SetCanEverAffectNavigation(true);
		hism->SetRelativeLocation(FVector(0.0f, 0.0f, -0.01f));
	}

	bOpen = false;
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

	RightGate->PlayAnimation(OpenAnim, false);
	LeftGate->PlayAnimation(OpenAnim, false);

	bOpen = true;

	SetTimer();
}

void AGate::CloseGate()
{
	if (!bOpen || GetCitizensAtBuilding().IsEmpty())
		return;

	RightGate->PlayAnimation(CloseAnim, false);
	LeftGate->PlayAnimation(CloseAnim, false);

	bOpen = false;

	SetTimer();
}

void AGate::UpdateNavigation()
{
	RightGate->SetCanEverAffectNavigation(!RightGate->CanEverAffectNavigation());
	LeftGate->SetCanEverAffectNavigation(!LeftGate->CanEverAffectNavigation());
}

void AGate::SetTimer()
{
	if (Camera->TimerManager->DoesTimerExist("Gate", this))
		Camera->TimerManager->UpdateTimerLength("Gate", this, 3.0f);
	else
		Camera->TimerManager->CreateTimer("Gate", this, 3.0f, "UpdateNavigation", {}, false);
}