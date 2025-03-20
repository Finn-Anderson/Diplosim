#include "Work.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Buildings/House.h"
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/School.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

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

	Wage = 0;

	WorkStart = 6;
	WorkEnd = 18;

	bCanRest = true;

	bCanAttendEvents = true;

	bOpen = false;
}

void AWork::BeginPlay()
{
	Super::BeginPlay();

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

void AWork::UpkeepCost()
{
	for (ACitizen* citizen : GetOccupied())
		citizen->Balance += Wage;

	int32 upkeep = Wage * GetOccupied().Num();
	Camera->ResourceManager->TakeUniversalResource(Money, upkeep, -100000);
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

	if (bOpen) {
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

	Citizen->HatMesh->SetStaticMesh(nullptr);

	Citizen->AIController->DefaultAction();

	return true;
}

void AWork::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!bOpen && GetOccupied().Contains(Citizen))
		Citizen->AIController->DefaultAction();

	if (GetOccupied().Contains(Citizen) && Citizen->HatMesh->GetStaticMesh() != WorkHat)
		Citizen->HatMesh->SetStaticMesh(WorkHat);
}

void AWork::Open()
{
	if (bOpen)
		return;
	
	bOpen = true;

	for (ACitizen* citizen : GetOccupied())
		citizen->AIController->DefaultAction();
}

void AWork::Close()
{
	if (!bOpen)
		return;
	
	bOpen = false;

	for (ACitizen* citizen : GetOccupied()) {
		Leave(citizen);

		citizen->AIController->DefaultAction();
	}
}

void AWork::CheckWorkStatus(int32 Hour)
{
	if (Hour == WorkEnd && bOpen)
		UpkeepCost();

	if (IsA<ASchool>() && bOpen && !GetCitizensAtBuilding().IsEmpty())
		Cast<ASchool>(this)->AddProgress();

	FEventStruct event;

	if (Camera->CitizenManager->OngoingEvents().Contains(event) || (IsA<AClinic>() && (!Camera->CitizenManager->Infected.IsEmpty() || !Camera->CitizenManager->Injuries.IsEmpty())))
		return;

	if (WorkStart >= WorkEnd) {
		if ((Hour >= WorkStart || Hour < WorkEnd))
			Open();
		else
			Close();
	}
	else {
		if (Hour >= WorkStart && Hour < WorkEnd)
			Open();
		else
			Close();
	}
}

void AWork::Production(ACitizen* Citizen)
{
	TArray<ACitizen*> citizens = GetCitizensAtBuilding();

	for (ACitizen* citizen : citizens) {
		for (ACitizen* c : citizens) {
			if (c == citizen)
				continue;

			int32 count = 0;

			for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen)) {
				for (FPersonality* p : Camera->CitizenManager->GetCitizensPersonalities(c)) {
					if (personality->Trait == p->Trait)
						count += 2;
					else if (personality->Likes.Contains(p->Trait))
						count++;
					else if (personality->Dislikes.Contains(p->Trait))
						count--;
				}
			}

			int32 chance = FMath::RandRange(0, 100) + count * 15;

			if (chance >= 0)
				continue;

			Camera->CitizenManager->Injure(citizen, FMath::RandRange(0, 100));

			Camera->CitizenManager->Injure(c, FMath::RandRange(0, 100));

			break;
		}
	}
}