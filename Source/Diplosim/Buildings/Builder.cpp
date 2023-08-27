#include "Buildings/Builder.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"

ABuilder::ABuilder()
{
	Constructing = nullptr;
}

bool ABuilder::CheckInstant() 
{
	TArray<AActor*> foundBuilders;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->GetClass(), foundBuilders);

	if (foundBuilders.Num() == 0) {
		return true;
	}

	return false;
}

void ABuilder::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (Occupied.Contains(Citizen)) {
		if (Constructing != nullptr) {
			Citizen->MoveTo(Constructing);
		}
		else {
			CheckConstruction(Citizen);
		}
	}
}

void ABuilder::CheckConstruction(ACitizen* Citizen)
{
	if (AtWork.Contains(Citizen)) {
		TArray<AActor*> foundBuildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundBuildings);

		for (int32 i = 0; i < foundBuildings.Num(); i++) {
			ABuilding* building = Cast<ABuilding>(foundBuildings[i]);

			if (building->BuildStatus != EBuildStatus::Construction)
				continue;

			TArray<AActor*> foundBuilders;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->GetClass(), foundBuilders);

			bool passed = true;
			for (int32 j = 0; j < foundBuilders.Num(); j++) {
				ABuilder* builder = Cast<ABuilder>(foundBuildings[i]);

				if (builder->Constructing == building) {
					passed = false;
				}
			}

			if (passed) {
				Constructing = building;

				Citizen->MoveTo(Constructing);

				return;
			}
		}
	}
}