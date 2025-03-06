#include "ConstructionManager.h"

#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Buildings/Work/Service/Builder.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Buildings/Misc/Broch.h"

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

	ACamera* camera = Cast<ACamera>(GetOwner());

	if (camera->WidgetComponent->IsAttachedTo(Building->GetRootComponent()) && !Building->IsA<ABroch>() && !camera->WidgetComponent->bHiddenInGame)
		camera->DisplayInteract(Building);
	else
		camera->WidgetComponent->SetHiddenInGame(true);
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

		bool bCanMove = builder->GetOccupied()[0]->AIController->CanMoveTo(Building->GetActorLocation());
		bool bCanMoveRoof = builder->GetOccupied()[0]->AIController->CanMoveTo(Building->GetActorLocation() + FVector(0.0f, 0.0f, 75.0f));

		if (bCanMove == bCanMoveRoof && !bCanMove)
			continue;

		if (target == nullptr) {
			target = builder;

			continue;
		}

		double magnitude = builder->GetOccupied()[0]->AIController->GetClosestActor(400.0f, Building->GetActorLocation(), target->GetActorLocation(), builder->GetActorLocation());

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
	int32 constructionIndex = -1;
	int32 repairIndex = -1;

	for (int32 i = 0; i < Construction.Num(); i++) {
		if (Construction[i].Builder != nullptr)
			continue;

		bool bCanMove = Builder->GetOccupied()[0]->AIController->CanMoveTo(Construction[i].Building->GetActorLocation());
		bool bCanMoveRoof = Builder->GetOccupied()[0]->AIController->CanMoveTo(Construction[i].Building->GetActorLocation() + FVector(0.0f, 0.0f, 75.0f));

		if (bCanMove == bCanMoveRoof && !bCanMove)
			continue;

		if (constructionIndex == -1)
			constructionIndex = i;

		if (repairIndex == -1 && IsRepairJob(Construction[i].Building, Construction[i].Builder))
			repairIndex = i;

		if (constructionIndex > -1 && repairIndex > -1)
			break;
	}

	if (repairIndex > -1) {
		Construction[repairIndex].Builder = Builder;

		Builder->GetOccupied()[0]->AIController->AIMoveTo(Construction[repairIndex].Building);
	}
	else if (constructionIndex > -1) {
		Construction[constructionIndex].Builder = Builder;

		Builder->GetOccupied()[0]->AIController->AIMoveTo(Construction[constructionIndex].Building);
	}
}

bool UConstructionManager::IsBeingConstructed(class ABuilding* Building, class ABuilder* Builder)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;
	constructionStruct.Builder = Builder;

	if (Construction.IsEmpty() || !Construction.Contains(constructionStruct))
		return false;

	return true;
}

bool UConstructionManager::IsRepairJob(class ABuilding* Building, class ABuilder* Builder)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;
	constructionStruct.Builder = Builder;

	int32 index = Construction.Find(constructionStruct);

	if (Construction[index].Status == EBuildStatus::Construction)
		return false;

	return true;
}

ABuilder* UConstructionManager::GetBuilder(class ABuilding* Building)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Building = Building;

	int32 index = Construction.Find(constructionStruct);

	if (index == INDEX_NONE)
		return nullptr;

	return Construction[index].Builder;
}

ABuilding* UConstructionManager::GetBuilding(class ABuilder* Builder)
{
	FConstructionStruct constructionStruct;
	constructionStruct.Builder = Builder;

	int32 index = Construction.Find(constructionStruct);

	return Construction[index].Building;
}