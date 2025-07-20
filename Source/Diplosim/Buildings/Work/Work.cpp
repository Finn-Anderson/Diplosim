#include "Work.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Blueprint/UserWidget.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/House.h"
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Universal/HealthComponent.h"

AWork::AWork()
{
	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(1500.0f, 1500.0f, 1500.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));
	DecalComponent->SetVisibility(false);

	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetSphereRadius(1500.0f);

	WagePerHour = 0.0f;

	bCanAttendEvents = true;
	bEmergency = false;

	ForcefieldRange = 0;
}

void AWork::BeginPlay()
{
	Super::BeginPlay();

	WorkHours.Empty();

	for (int32 i = 0; i < MaxCapacity; i++) {
		FWorkHoursStruct hours;

		for (int32 j = 0; j < 24; j++) {
			EWorkType type = EWorkType::Freetime;

			if (j >= 6 && j < 18)
				type = EWorkType::Work;

			hours.WorkHours.Add(j, type);
		}

		WorkHours.Add(hours);
	}

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AWork::OnRadialOverlapBegin);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AWork::OnRadialOverlapEnd);
}

void AWork::OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void AWork::OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

	AddToWorkHours(Citizen, true);

	if (IsWorking(Citizen)) {
		if (bCanAttendEvents && Citizen->AIController->MoveRequest.Actor != nullptr && Citizen->AIController->MoveRequest.Actor->IsA<ABroadcast>() && Cast<ABroadcast>(Citizen->AIController->MoveRequest.Actor)->bHolyPlace)
			return true;

		Citizen->AIController->DefaultAction();
	}

	return true;
}

bool AWork::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = nullptr;

	AddToWorkHours(Citizen, false);

	Citizen->HatMesh->SetStaticMesh(nullptr);

	Citizen->AIController->DefaultAction();

	return true;
}

void AWork::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!IsWorking(Citizen) && GetOccupied().Contains(Citizen))
		Citizen->AIController->DefaultAction();

	if (GetOccupied().Contains(Citizen) && Citizen->HatMesh->GetStaticMesh() != WorkHat)
		Citizen->HatMesh->SetStaticMesh(WorkHat);
}

void AWork::AddToWorkHours(ACitizen* Citizen, bool bAdd)
{
	for (int32 i = 0; i < WorkHours.Num(); i++) {
		if (IsValid(WorkHours[i].Citizen))
			continue;

		if (bAdd)
			WorkHours[i].Citizen = Citizen;
		else
			WorkHours[i].Citizen = nullptr;

		if (Camera->HoursUIInstance->IsInViewport())
			Camera->UpdateWorkHours(this, i);

		break;
	}
}

void AWork::CheckWorkStatus(int32 Hour)
{
	if (IsA<ASchool>() && !GetCitizensAtBuilding().IsEmpty())
		Cast<ASchool>(this)->AddProgress();

	FEventStruct event;

	if (Camera->CitizenManager->OngoingEvents().Contains(event) || (IsA<AClinic>() && (!Camera->CitizenManager->Infected.IsEmpty() || !Camera->CitizenManager->Injuries.IsEmpty())))
		return;

	for (ACitizen* citizen : GetOccupied()) {
		bool isWorkingNow = IsWorking(citizen, Hour);

		if ((isWorkingNow && !IsAtWork(citizen)) || (!isWorkingNow && IsAtWork(citizen)))
			citizen->AIController->DefaultAction();
	}
}

bool AWork::IsWorking(ACitizen* Citizen, int32 Hour)
{
	if (Hour == -1)
		Hour = Camera->Grid->AtmosphereComponent->Calendar.Hour;

	FWorkHoursStruct hours;
	hours.Citizen = Citizen;

	int32 index = WorkHours.Find(hours);

	EWorkType type = *hours.WorkHours.Find(Hour);

	if ((type == EWorkType::Work && Camera->CitizenManager->GetRaidPolicyStatus() == ERaidPolicy::Default) || bEmergency)
		return true;

	return false;
}

bool AWork::IsAtWork(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt == this)
		return true;
	else if (IsA<AExternalProduction>()) {
		AExternalProduction* externalProduction = Cast<AExternalProduction>(this);

		for (FValidResourceStruct validResource : externalProduction->Resources) {
			for (FWorkerStruct workerStruct : validResource.Resource->WorkerStruct) {
				if (!workerStruct.Citizens.Contains(Citizen))
					continue;

				return true;
			}
		}
	}

	return false;
}

int32 AWork::GetHoursInADay(ACitizen* Citizen)
{
	int32 hours = 0;

	for (FWorkHoursStruct hoursStruct : WorkHours) {
		if (hoursStruct.Citizen != Citizen)
			continue;

		for (auto& element : hoursStruct.WorkHours)
			if (element.Value == EWorkType::Work)
				hours++;
	}

	return hours;
}

int32 AWork::GetWage(ACitizen* Citizen)
{
	return WagePerHour * GetHoursInADay(Citizen);
}

int32 AWork::GetAverageWage()
{
	int32 averageHours = 0;

	for (FWorkHoursStruct hoursStruct : WorkHours)
		for (auto& element : hoursStruct.WorkHours)
			if (element.Value == EWorkType::Work)
				averageHours++;

	averageHours /= Capacity;

	return WagePerHour * averageHours;
}

void AWork::SetNewWorkHours(int32 Index, FWorkHoursStruct NewWorkHours)
{
	WorkHours[Index].WorkHours = NewWorkHours.WorkHours;
}

void AWork::SetEmergency(bool bStatus)
{
	if (bStatus == bEmergency)
		return;

	bEmergency = bStatus;

	CheckWorkStatus(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

void AWork::Production(ACitizen* Citizen)
{
	TArray<ACitizen*> citizens = GetCitizensAtBuilding();

	for (ACitizen* citizen : citizens) {
		int32 chance = Camera->Grid->Stream.RandRange(1, 100);

		if (chance > 98)
			continue;

		citizen->HealthComponent->TakeHealth(5, this);
	}
}