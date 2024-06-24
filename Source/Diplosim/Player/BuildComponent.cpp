#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"

#include "Camera.h"
#include "ConstructionManager.h"
#include "Player/ResourceManager.h"
#include "Buildings/Building.h"
#include "Buildings/Builder.h"
#include "Buildings/Wall.h"
#include "Map/Grid.h"
#include "Map/Vegetation.h"
#include "Map/Mineral.h"
#include "DiplosimGameModeBase.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Broch.h"
#include "Buildings/Road.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bCanRotate = true;

	Rotation.Yaw = 90.0f;

	StartLocation = FVector::Zero();
	EndLocation = FVector::Zero();

	BuildingToMove = nullptr;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Buildings.IsEmpty())
		return;

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		UHierarchicalInstancedStaticMeshComponent* comp = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.GetComponent());
		int32 instance = hit.Item;

		FTransform transform;
		comp->GetInstanceTransform(instance, transform);

		FVector location = transform.GetLocation();

		if (comp == Camera->Grid->HISMFlatGround)
			location.Z += Camera->Grid->HISMGround->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 2.0f;
		else
			location.Z += comp->GetStaticMesh()->GetBounds().GetBox().GetSize().Z - 2.0f;

		location.Z = FMath::RoundHalfFromZero(location.Z);

		Buildings[0]->SetActorLocation(location);

		Camera->WidgetComponent->SetWorldLocation(location + FVector(0.0f, 0.0f, Buildings[0]->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z));

		if (StartLocation != FVector::Zero() && EndLocation != Buildings[0]->GetActorLocation())
			SetBuildingsOnPath();
	}

	for (ABuilding* building : Buildings) {
		UDecalComponent* decalComp = building->FindComponentByClass<UDecalComponent>();

		if (!IsValidLocation(building) || !CheckBuildCosts()) {
			building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);

			if (decalComp != nullptr)
				decalComp->SetHiddenInGame(true);
		}
		else {
			building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

			if (decalComp != nullptr)
				decalComp->SetHiddenInGame(false);
		}
	}
}

void UBuildComponent::SetBuildingsOnPath()
{
	EndLocation = Buildings[0]->GetActorLocation();

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 1.0f);

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 x = StartLocation.X / 100.0f + bound / 2;
	int32 y = StartLocation.Y / 100.0f + bound / 2;

	TArray<FVector> locations = CalculatePath(Camera->Grid->Storage[x][y]);

	for (FVector location : locations) {
		if (location == EndLocation)
			continue;

		bool bRoadExists = false;

		FHitResult hit(ForceInit);

		if (GetWorld()->LineTraceSingleByChannel(hit, location + FVector(0.0f, 0.0f, 10.0f), location, ECollisionChannel::ECC_Visibility))
			if (hit.GetActor()->IsA<ARoad>())
				bRoadExists = true;

		if (!bRoadExists)
			SpawnBuilding(Buildings[0]->GetClass(), location);
	}
}

TArray<FVector> UBuildComponent::CalculatePath(FTileStruct Tile)
{
	TArray<FVector> locations;

	FTransform transform;
	transform = Camera->Grid->GetTransform(&Tile);

	FVector location = transform.GetLocation() - FVector(0.0f, 0.0f, 2.0f);

	locations.Add(location);

	FTileStruct closestTile = Tile;

	for (auto& element : Tile.AdjacentTiles) {
		FVector currentClosestLoc = Camera->Grid->GetTransform(&closestTile).GetLocation();

		FVector newClosestLoc = Camera->Grid->GetTransform(element.Value).GetLocation();

		float currentDist = FVector::Dist(currentClosestLoc, EndLocation);
		float newDist = FVector::Dist(newClosestLoc, EndLocation);

		if (currentDist > newDist)
			closestTile = *element.Value;
	}

	if (closestTile.X != Tile.X || closestTile.Y != Tile.Y)
		locations.Append(CalculatePath(closestTile));

	return locations;
}

FQueueStruct UBuildComponent::GetBuildCosts()
{
	FQueueStruct queue = Buildings[0]->CostList;

	for (int32 i = 0; i < queue.Items.Num(); i++)
		queue.Items[i].Amount *= Buildings.Num();

	return queue;
}

bool UBuildComponent::CheckBuildCosts()
{
	UResourceManager* rm = Camera->ResourceManagerComponent;

	FQueueStruct queue = GetBuildCosts();

	for (FItemStruct item : queue.Items) {
		int32 maxAmount = rm->GetResourceAmount(item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

bool UBuildComponent::IsValidLocation(ABuilding* building)
{
	FVector size = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

	FVector location = building->BuildingMesh->GetRelativeLocation();

	if (StartLocation != FVector::Zero() && location.Z != StartLocation.Z)
		return false;

	int32 numCollisionsX = (FMath::Floor(size.X / 151.0f) * 3);
	int32 numCollisionsY = (FMath::Floor(size.Y / 151.0f) * 3);

	if (numCollisionsX == 0)
		numCollisionsX = 1;

	if (numCollisionsY == 0)
		numCollisionsY = 1;

	int32 numCollisions = numCollisionsX * numCollisionsY;

	if (building->bOffset)
		numCollisions -= 1;

	if (numCollisions > building->Collisions.Num())
		return false;

	for (FCollisionStruct collision : building->Collisions) {
		if ((building->GetClass() == FoundationClass->GetDefaultObject()->GetClass() && collision.Actor->IsA<AGrid>()) || (building->IsA<ARoad>() && collision.Actor->IsA<ARoad>() && building->GetActorLocation() != collision.Actor->GetActorLocation()))
			continue;

		if (collision.HISM == Camera->Grid->HISMWater || collision.HISM == Camera->Grid->HISMLava)
			return false;
		
		FTransform transform;

		if (collision.HISM->IsValidLowLevelFast())
			collision.HISM->GetInstanceTransform(collision.Instance, transform);
		else
			transform = collision.Actor->GetTransform();

		if (building->IsA<AWall>() && collision.Actor->IsA<AWall>()) {
			if (building->GetActorLocation() == collision.Actor->GetActorLocation())
				return false;
		}	
		else if (transform.GetLocation().Z != FMath::Floor(building->GetActorLocation().Z) - 100.0f)
			return false;
	}

	return true;
}

void UBuildComponent::SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FVector location)
{
	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);

	Buildings.Add(building);

	Camera->DisplayInteract(Buildings[0]);
}

void UBuildComponent::DetachBuilding()
{
	if (Buildings.IsEmpty())
		return;

	Camera->WidgetComponent->SetHiddenInGame(true);
	Camera->WidgetComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

	SetComponentTickEnabled(false);

	StartLocation = FVector::Zero();
	EndLocation = FVector::Zero();

	Buildings.Empty();
}

void UBuildComponent::RemoveBuilding()
{
	if (Buildings.IsEmpty())
		return;

	for (ABuilding* building : Buildings) {
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 3, 1.0f);

		building->DestroyBuilding();
	}

	DetachBuilding();
}

void UBuildComponent::RotateBuilding(bool Rotate)
{
	if (Buildings.IsEmpty() || Buildings[0]->IsA<ARoad>())
		return;

	if (Rotate && bCanRotate) {
		int32 yaw = Rotation.Yaw;

		if (yaw % 90 == 0)
			yaw += 90;

		if (yaw == 360)
			yaw = 0;

		bCanRotate = false;

		Rotation.Yaw = yaw;

		Buildings[0]->SetActorRotation(Rotation);
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

void UBuildComponent::Place()
{
	if (Buildings.IsEmpty() || !CheckBuildCosts())
		return;

	for (ABuilding* building : Buildings)
		if (!IsValidLocation(building))
			return;

	if (StartLocation == FVector::Zero() && Buildings[0]->IsA<ARoad>()) {
		StartLocation = Buildings[0]->GetActorLocation();
		EndLocation = Buildings[0]->GetActorLocation();

		return;
	}

	for (ABuilding* building : Buildings)
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->RemoveInstance(treeStruct.Instance);

	if (BuildingToMove->IsValidLowLevelFast()) {
		BuildingToMove->SetActorLocation(Buildings[0]->GetActorLocation());
		BuildingToMove->SetActorRotation(Buildings[0]->GetActorRotation());

		TArray<ACitizen*> citizens = BuildingToMove->GetCitizensAtBuilding();

		for (ACitizen* citizen : citizens)
			citizen->SetActorLocation(BuildingToMove->GetActorLocation());

		UConstructionManager* cm = Camera->ConstructionManagerComponent;

		if (cm->IsBeingConstructed(BuildingToMove, nullptr)) {
			ABuilder* builder = cm->GetBuilder(BuildingToMove);

			if (builder != nullptr && !builder->GetOccupied().IsEmpty())
				builder->GetOccupied()[0]->AIController->RecalculateMovement(BuildingToMove);
		}

		for (ACitizen* citizen : BuildingToMove->GetOccupied())
			citizen->AIController->RecalculateMovement(BuildingToMove);

		BuildingToMove = nullptr;

		DetachBuilding();

		return;
	}

	if (Buildings[0]->IsA<AWall>())
		Cast<AWall>(Buildings[0])->StoreSocketLocations();
	
	for (ABuilding* building : Buildings) {
		UDecalComponent* decalComp = building->FindComponentByClass<UDecalComponent>();

		if (decalComp != nullptr)
			decalComp->SetHiddenInGame(true);

		building->BuildingMesh->SetOverlayMaterial(nullptr);

		building->Build();
	}

	if (Camera->Start) {
		Camera->Start = false;
		Camera->BuildUIInstance->AddToViewport();
		Camera->Grid->MapUIInstance->RemoveFromParent();

		Buildings[0]->BuildingMesh->bReceivesDecals = true;

		ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
		gamemode->Broch = Buildings[0];
		gamemode->Grid = Camera->Grid;

		gamemode->SetWaveTimer();
	}

	DetachBuilding();
}

void UBuildComponent::QuickPlace()
{
	if (Camera->Start || BuildingToMove->IsValidLowLevelFast()) {
		Place();

		return;
	}

	ABuilding* building = Buildings[0];

	Place();

	SpawnBuilding(building->GetClass());
}