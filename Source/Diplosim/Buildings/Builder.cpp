#include "Buildings/Builder.h"

#include "Kismet/GameplayStatics.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

ABuilder::ABuilder()
{
	Constructing = nullptr;
}

bool ABuilder::CheckInstant() 
{
	TArray<AActor*> foundBuilders;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->GetClass(), foundBuilders);

	if (foundBuilders.Num() == 1) {
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
	TArray<AActor*> foundBuildings;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundBuildings);

	for (int32 i = 0; i < foundBuildings.Num(); i++) {
		ABuilding* building = Cast<ABuilding>(foundBuildings[i]);

		if (building->BuildStatus != EBuildStatus::Construction)
			continue;

		TArray<AActor*> foundBuilders;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilder::StaticClass(), foundBuilders);

		bool passed = true;
		for (int32 j = 0; j < foundBuilders.Num(); j++) {
			ABuilder* builder = Cast<ABuilder>(foundBuilders[j]);

			if (builder->Constructing == building) {
				passed = false;

				break;
			}
		}

		if (passed) {
			Constructing = building;

			Citizen->MoveTo(Constructing);

			return;
		}
	}
}

void ABuilder::CarryResources(ACitizen* Citizen, ABuilding* Building)
{
	int32 amount = 0;
	int32 capacity = 10;
	if (capacity > Building->Storage) {
		capacity = Building->Storage;
	}

	for (int32 i = 0; i < Constructing->CostList.Num(); i++) {
		if (Constructing->CostList[i].Type == Camera->ResourceManagerComponent->GetResource(Building)) {
			amount = FMath::Clamp(Constructing->CostList[i].Cost - Constructing->CostList[i].Stored, 0, capacity);

			break;
		}
	}
	Camera->ResourceManagerComponent->TakeLocalResource(Building, amount);

	if (*Camera->ResourceManagerComponent->GetResource(Building)) {
		AResource* r = Cast<AResource>(Camera->ResourceManagerComponent->GetResource(Building)->GetDefaultObject());
		Citizen->Carry(r, amount, Constructing);
	}
}