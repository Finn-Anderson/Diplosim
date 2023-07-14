#include "Citizen.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"

#include "AIController.h"
#include "Resource.h"
#include "Building.h"
#include "ExternalProduction.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	House = nullptr;
	Employment = nullptr;

	Carrying = nullptr;

	Energy = 100;

	Happiness = 100;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();
	
	aiController = Cast<AAIController>(GetController());
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

		for (int i = 0; i < b.Num(); i++) {
			ABuilding* building = Cast<ABuilding>(b[i]);

			if (building->GetCapacity() != building->GetOccupied().Num() && building->Category == ECategory::House) {
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

		GetWorldTimerManager().SetTimer(HouseTimer, this, &ACitizen::LookForHouse, 30.0f, false, 2.0f);
	}
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal) {
		if (OtherActor == House || OtherActor == Employment) {
			SetActorHiddenInGame(true);

			ABuilding* e = Cast<ABuilding>(Employment);

			if (Energy == 0 && Goal == House) {
				GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::GainEnergy, 0.6f, true);
			}
			else if (OtherActor == Employment && !e->InternalProd) {
				AExternalProduction* ex = Cast<AExternalProduction>(Employment);

				ex->StoreResource(Carrying);

				Carrying = nullptr;
			}
		}
		
		if (OtherActor->GetClass() == ResourceActor) {
			if (Carrying == nullptr) {
				Carrying = Cast<AResource>(OtherActor);

				MoveTo(Employment);

				Goal = Employment;
			}
		}
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == House || OtherActor == Employment) {
		SetActorHiddenInGame(false);
	}

	if (OtherActor == House) {
		GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 30.0f, true);
	}
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	if (Energy == 0) {
		MoveTo(House);

		Goal = House;
	}
}

void ACitizen::GainEnergy()
{
	Energy = FMath::Clamp(Energy + 1, 0, 100);

	if (Energy == 100) {
		MoveTo(Employment);

		Goal = Employment;
	}
}