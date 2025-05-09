#include "Buildings/Work/Service/Research.h"

#include "Player/Camera.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"

AResearch::AResearch()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorTickInterval(0.01f);
	
	TimeLength = 5.0f;

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretMesh->SetupAttachment(BuildingMesh, "TurretSocket");

	TelescopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelescopeMesh"));
	TelescopeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelescopeMesh->SetupAttachment(TurretMesh, "TelescopeSocket");
}

void AResearch::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime < 0.009f || DeltaTime > 1.0f)
		return;

	TurretMesh->SetRelativeRotation(FMath::RInterpTo(TurretMesh->GetRelativeRotation(), TurretTargetRotation, DeltaTime, 1.0f));
	TelescopeMesh->SetRelativeRotation(FMath::RInterpTo(TelescopeMesh->GetRelativeRotation(), TelescopeTargetRotation, DeltaTime, 1.0f));

	if (FVector::Dist(TurretMesh->GetRelativeRotation().Vector(), TurretTargetRotation.Vector()) < 0.1f && FVector::Dist(TelescopeMesh->GetRelativeRotation().Vector(), TelescopeTargetRotation.Vector()) < 0.1f)
		SetActorTickEnabled(false);
}

void AResearch::BeginRotation()
{
	TurretTargetRotation = FRotator(0.0f, FMath::RandRange(0, 359), 0.0f);
	TelescopeTargetRotation = FRotator(0.0f, 0.0f, FMath::RandRange(-15, 15));

	SetActorTickEnabled(true);

	FTimerStruct timer;

	timer.CreateTimer("Rotate", this, GetTime(FMath::RandRange(60, 120)), FTimerDelegate::CreateUObject(this, &AResearch::BeginRotation), false, true);
	Camera->CitizenManager->Timers.Add(timer);
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
	FTimerStruct timer;
	timer.CreateTimer("Research", this, GetTime(TimeLength), FTimerDelegate::CreateUObject(this, &AResearch::Production, GetCitizensAtBuilding()[0]), true);

	Camera->CitizenManager->Timers.Add(timer);
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

	Camera->ResearchManager->Research(1.0f);
}