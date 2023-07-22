#include "Citizen.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

#include "Resource.h"
#include "Building.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	House = nullptr;
	Employment = nullptr;

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

void ACitizen::LookForHouse()
{
	if (Employment != nullptr) {
		TArray<AActor*> b;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), b);

		ABuilding* e = Cast<ABuilding>(Employment);

		for (int i = 0; i < b.Num(); i++) {
			ABuilding* building = Cast<ABuilding>(b[i]);

			if (building->GetCapacity() != building->GetOccupied().Num() && building->Category == ECategory::House) {
				bool ecoCheck = false;

				if (e->EcoStatus == EEconomy::Modest && building->EcoStatus != EEconomy::Rich) {
					ecoCheck = true;
				}
				else if (e->EcoStatus == EEconomy::Poor && building->EcoStatus == EEconomy::Poor) {
					ecoCheck = true;
				}
				else {
					ecoCheck = true;
				}

				if (ecoCheck) {
					if (House == nullptr) {
						House = building;
					}
					else {
						float dH = (House->GetActorLocation() - Employment->GetActorLocation()).Length();

						float dB = (building->GetActorLocation() - Employment->GetActorLocation()).Length();

						if (dH > dB) {
							House = building;
						}
					}
				}
			}
		}

		GetWorldTimerManager().SetTimer(HouseTimer, this, &ACitizen::LookForHouse, 30.0f, false, 2.0f);
	}
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal) {
		if (OtherActor == House || OtherActor == Employment) {
			SetActorHiddenInGame(true);

			if (OtherActor == House) {
				GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::GainEnergy, 0.6f, true);
			}
		}
		else {
			AResource* r = Cast<AResource>(OtherActor);

			FTimerHandle harvestTimer;
			GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::Carry, r), 5.0f, false);
		}
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == House || OtherActor == Employment) {
		SetActorHiddenInGame(false);
	}

	if (OtherActor == House) {
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
			ABuilding* e = Cast<ABuilding>(Employment);

			e->RemoveCitizen(this);
		}

		Destroy();
	}
}

void ACitizen::GainEnergy()
{
	int32 maxE;

	ABuilding* h = Cast<ABuilding>(House);
	if (h->EcoStatus == EEconomy::Modest) {
		maxE = 50;
	}
	else if (h->EcoStatus == EEconomy::Poor) {
		maxE = 75;
	}
	else {
		maxE = 100;
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