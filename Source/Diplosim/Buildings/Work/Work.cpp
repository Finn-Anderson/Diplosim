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

	bRange = false;

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

	int32 hour = Camera->Grid->AtmosphereComponent->Calendar.Hour;

	if (hour >= WorkStart && hour <= WorkEnd && !Camera->CitizenManager->IsWorkEvent(this))
		Open();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AWork::OnRadialOverlapBegin);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AWork::OnRadialOverlapEnd);
}

void AWork::OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bRange || !OtherActor->IsA<AHouse>() || GetOccupied().IsEmpty())
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	house->Religions.Add(Belief);

	Houses.Add(house);
}

void AWork::OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Houses.Contains(OtherActor))
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	house->Religions.RemoveSingle(Belief);

	Houses.Remove(house);
}

void AWork::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance += Wage;
	}

	int32 upkeep = Wage * Occupied.Num();
	Camera->ResourceManager->TakeUniversalResource(Money, upkeep, -100000);
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

	if (bOpen) {
		if (bCanAttendEvents && Citizen->AIController->MoveRequest.Actor != nullptr && Citizen->AIController->MoveRequest.Actor->IsA<AReligion>())
			return true;

		Citizen->AIController->AIMoveTo(this);
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
	bOpen = true;

	for (ACitizen* citizen : GetOccupied())
		citizen->AIController->DefaultAction();
}

void AWork::Close()
{
	bOpen = false;

	for (ACitizen* citizen : GetOccupied()) {
		Leave(citizen);

		citizen->AIController->DefaultAction();
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