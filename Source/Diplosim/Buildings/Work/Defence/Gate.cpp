#include "Buildings/Work/Defence/Gate.h"

#include "Components/SphereComponent.h"

#include "AI/Enemy.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"

AGate::AGate()
{
	BuildingMesh->bFillCollisionUnderneathForNavmesh = false;

	RightGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightGate"));
	RightGate->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	RightGate->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RightGate->SetupAttachment(BuildingMesh, "RightGateSocket");
	RightGate->SetCanEverAffectNavigation(true);

	LeftGate = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftGate"));
	LeftGate->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	LeftGate->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LeftGate->SetupAttachment(BuildingMesh, "LeftGateSocket");
	LeftGate->SetCanEverAffectNavigation(true);

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
	if (Camera->CitizenManager->DoesTimerExist("Gate", this))
		Camera->CitizenManager->UpdateTimerLength("Gate", this, 3.0f);
	else
		Camera->CitizenManager->CreateTimer("Gate", this, 3.0f, "UpdateNavigation", {}, false);
}