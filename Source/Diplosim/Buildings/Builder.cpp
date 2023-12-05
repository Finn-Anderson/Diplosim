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

	TSubclassOf<AResource> resource = Camera->ResourceManagerComponent->GetResource(Building);

	if (capacity > Building->Storage) {
		capacity = Building->Storage;
	}

	for (FCostStruct costStruct : Constructing->GetCosts()) {
		if (costStruct.Type == resource) {
			amount = FMath::Clamp(costStruct.Cost - costStruct.Stored, 0, capacity);

			break;
		}
	}

	Camera->ResourceManagerComponent->TakeCommittedResource(resource, amount);

	Camera->ResourceManagerComponent->TakeLocalResource(Building, amount);

	Citizen->Carry(Cast<AResource>(resource->GetDefaultObject()), amount, Constructing);
}