#include "Player/ConstructionManager.h"

#include "Kismet/GameplayStatics.h"

#include "Buildings/Builder.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

UConstructionManager::UConstructionManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UConstructionManager::AddBuilding(class ABuilding* Building, EBuildStatus Status)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;
	constructionStruct.Status = Status;

	Construction.Add(constructionStruct);

	FindBuilder(Building);
}

void UConstructionManager::RemoveBuilding(class ABuilding* Building)
{
	if (!IsBeingConstructed(Building, nullptr))
		return;

	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;

	ABuilder* builder = GetBuilder(Building);

	if (builder != nullptr && !builder->GetOccupied().IsEmpty())
		builder->GetOccupied()[0]->AIController->AIMoveTo(builder);

	Construction.Remove(constructionStruct);
}

void UConstructionManager::FindBuilder(class ABuilding* Building)
{
	TArray<AActor*> foundBuilders;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilder::StaticClass(), foundBuilders);

	ABuilder* target = nullptr;

	for (int32 i = 0; i < foundBuilders.Num(); i++) {
		ABuilder* builder = Cast<ABuilder>(foundBuilders[i]);

		FConstructionStruct constructionStruct;
		constructionStruct.Builder = builder;
		constructionStruct.Building = builder;

		if (Construction.Contains(constructionStruct) || builder->GetOccupied().IsEmpty() || builder->GetOccupied()[0]->Building.BuildingAt != builder)
			continue;

		if (target == nullptr) {
			target = builder;

			continue;
		}

		double magnitude = builder->GetOccupied()[0]->AIController->GetClosestActor(Building->GetActorLocation(), target->GetActorLocation(), builder->GetActorLocation());

		if (magnitude <= 0.0f)
			continue;

		target = builder;
	}

	if (target == nullptr)
		return;

	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;

	int32 index = Construction.Find(constructionStruct);
	Construction[index].Builder = target;

	target->GetOccupied()[0]->AIController->AIMoveTo(Building);
}

void UConstructionManager::FindConstruction(class ABuilder* Builder)
{
	for (int32 i = 0; i < Construction.Num(); i++) {
		if (Construction[i].Builder != nullptr)
			continue;

		Construction[i].Builder = Builder;

		Builder->GetOccupied()[0]->AIController->AIMoveTo(Construction[i].Building);

		break;
	}
}

bool UConstructionManager::IsBeingConstructed(class ABuilding* Building, class ABuilder* Builder)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;
	constructionStruct.Builder = Builder;

	if (!Construction.Contains(constructionStruct))
		return false;

	return true;
}

ABuilder* UConstructionManager::GetBuilder(class ABuilding* Building)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;

	int32 index = Construction.Find(constructionStruct);

	return Construction[index].Builder;
}

ABuilding* UConstructionManager::GetBuilding(class ABuilder* Builder)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Builder = Builder;

	int32 index = Construction.Find(constructionStruct);

	return Construction[index].Building;
}