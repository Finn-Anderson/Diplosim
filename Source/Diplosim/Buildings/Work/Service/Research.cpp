#include "Buildings/Work/Service/Research.h"

#include "Player/Camera.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

AResearch::AResearch()
{
	TimeLength = 5.0f;

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretMesh->SetupAttachment(BuildingMesh, "TurretSocket");

	TelescopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelescopeMesh"));
	TelescopeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelescopeMesh->SetupAttachment(TurretMesh, "TelescopeSocket");
}

void AResearch::BeginRotation()
{
	PrevTurretTargetYaw = TurretTargetYaw;
	PrevTelescopeTargetPitch = TelescopeTargetPitch;

	TurretTargetYaw = Camera->Grid->Stream.RandRange(0, 359) / 360.0f;
	TelescopeTargetPitch = Camera->Grid->Stream.RandRange(-15, 15) / 360.0f;

	RotationTime = GetWorld()->GetTimeSeconds();

	Camera->Grid->AIVisualiser->RotatingBuildings.Add(this);

	Camera->CitizenManager->CreateTimer("Rotate", this, GetTime(Camera->Grid->Stream.RandRange(60, 120)), FTimerDelegate::CreateUObject(this, &AResearch::BeginRotation), false, true);
}

void AResearch::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	TurretMesh->SetOverlayMaterial(nullptr);
	TelescopeMesh->SetOverlayMaterial(nullptr);

	TurretMesh->bReceivesDecals = true;
	TelescopeMesh->bReceivesDecals = true;
	
	TurretMesh->SetHiddenInGame(true, true);

	Super::Build(bRebuild, bUpgrade, Grade);

	if (!CheckInstant()) {
		FVector bSize = ActualMesh->GetBounds().GetBox().GetSize();
		bSize.Z += TurretMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

		FVector cSize = ConstructionMesh->GetBounds().GetBox().GetSize();

		FVector size = bSize / cSize;

		BuildingMesh->SetRelativeScale3D(size);
	}
}

void AResearch::OnBuilt()
{
	TurretMesh->SetHiddenInGame(false, true);
	
	Super::OnBuilt();
}

void AResearch::Enter(class ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	if (GetCitizensAtBuilding().Num() == 1)
		BeginRotation();

	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Research", this);

	if (timer == nullptr)
		SetResearchTimer();
	else
		UpdateResearchTimer();
}

void AResearch::Leave(class ACitizen* Citizen)
{
	Super::Leave(Citizen);

	if (!GetCitizensAtBuilding().IsEmpty())
		return;

	if (GetCitizensAtBuilding().IsEmpty()) {
		Camera->CitizenManager->RemoveTimer("Rotate", this);

		SetActorTickEnabled(false);
	}

	Camera->CitizenManager->RemoveTimer("Research", this);
}

float AResearch::GetTime(int32 Time)
{
	float time = Time / GetCitizensAtBuilding().Num();

	for (ACitizen* citizen : GetCitizensAtBuilding())
		time -= (time / GetCitizensAtBuilding().Num()) * (citizen->GetProductivity() - 1.0f);

	return time;
}

void AResearch::SetResearchTimer()
{
	Camera->CitizenManager->CreateTimer("Research", this, GetTime(TimeLength), FTimerDelegate::CreateUObject(this, &AResearch::Production, GetCitizensAtBuilding()[0]), true);
}

void AResearch::UpdateResearchTimer()
{
	FTimerStruct* timer = Camera->CitizenManager->FindTimer("Research", this);

	if (timer == nullptr)
		return;

	timer->Target = GetTime(TimeLength);
}

void AResearch::Production(class ACitizen* Citizen)
{
	Super::Production(Citizen);

	Camera->ResearchManager->Research(1.0f, FactionName);
}