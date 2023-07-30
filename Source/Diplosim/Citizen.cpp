#include "Citizen.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

#include "Resource.h"
#include "Work.h"
#include "House.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	House = nullptr;
	Employment = nullptr;

	Balance = 0;

	Carrying = 0;

	Energy = 100;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();
	
	aiController = Cast<AAIController>(GetController());

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);
}

void ACitizen::MoveTo(AActor* Location)
{
	aiController->MoveToActor(Location, 10.0f, true);
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal) {
		if (OtherActor == Cast<AActor>(House) || OtherActor == Cast<AActor>(Employment)) {
			SetActorHiddenInGame(true);

			if (OtherActor == Cast<AActor>(House)) {
				GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::GainEnergy, 0.6f, true);
			}
		}
		else {
			AResource* r = Cast<AResource>(OtherActor);

			FTimerHandle harvestTimer;
			GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::Carry, r), 2.0f, false);
		}
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == Cast<AActor>(House) || OtherActor == Cast<AActor>(Employment)) {
		SetActorHiddenInGame(false);
	}

	if (OtherActor == Cast<AActor>(House)) {
		GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);
	}
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, -100, 100);

	if (Energy <= 0 && House != nullptr) {
		MoveTo(House);

		Goal = House;
	}

	if (Energy == -100) {
		if (Employment != nullptr) {
			Employment->RemoveCitizen(this);
		}

		Destroy();
	}
}

void ACitizen::GainEnergy()
{
	int32 maxE = 20;

	if (Balance >= 7) {
		Balance -= 7;
		maxE = 100;
	}
	else if (Balance >= 3) {
		Balance -= 3;
		maxE = 75;
	}
	else if (Balance >= 1) {
		Balance -= 1;
		maxE = 40;
	}

	Energy = FMath::Clamp(Energy + 1, -100, 100);

	if (Energy == 100) {
		MoveTo(Employment);

		Goal = Employment;
	}
}

void ACitizen::Carry(AResource* Resource)
{
	Carrying = Resource->GetYield();

	MoveTo(Employment);

	Goal = Employment;
}