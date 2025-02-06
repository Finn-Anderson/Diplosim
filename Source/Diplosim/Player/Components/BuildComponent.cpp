#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Camera/CameraComponent.h"

#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Stockpile.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Map/Grid.h"
#include "Map/Resources/Vegetation.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Universal/DiplosimUserSettings.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bCanRotate = true;

	Rotation.Yaw = 0.0f;

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

	if (Buildings.IsEmpty() || Camera->bInMenu)
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

		if (Buildings.Last()->GetActorLocation().X == location.X && Buildings.Last()->GetActorLocation().Y == location.Y)
			return;

		if (Buildings[0]->IsA<AStockpile>() && Buildings[0]->GetActorLocation().Z != 0.0f) {
			auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

			int32 x = Buildings[0]->GetActorLocation().X / 100.0f + (bound / 2);
			int32 y = Buildings[0]->GetActorLocation().Y / 100.0f + (bound / 2);

			bool bFlat = true;

			FTileStruct tile = Camera->Grid->Storage[x][y];

			if (!tile.bRamp) {
				if (tile.AdjacentTiles.Num() < 4)
					bFlat = false;
				else
					for (auto& element : tile.AdjacentTiles)
						if (element.Value->Level < tile.Level || element.Value->Level == 7)
							bFlat = false;

				if (bFlat)
					Camera->Grid->HISMFlatGround->SetCustomDataValue(tile.Instance, 0, 1.0f);
				else
					Camera->Grid->HISMGround->SetCustomDataValue(tile.Instance, 0, 1.0f);
			}

			if (comp != Camera->Grid->HISMRampGround)
				comp->SetCustomDataValue(instance, 0, 0.0f);
		}

		if (location.Z >= 0.0f) {
			location.Z += 100.0f;

			location.Z = FMath::RoundHalfFromZero(location.Z);

			Buildings[0]->SetActorLocation(location);

			if (StartLocation != FVector::Zero() && EndLocation != Buildings[0]->GetActorLocation())
				SetBuildingsOnPath();
		}
	}

	for (ABuilding* building : Buildings) {
		UDecalComponent* decalComp = building->FindComponentByClass<UDecalComponent>();

		if (!IsValidLocation(building) || !CheckBuildCosts()) {
			building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);

			if (IsValid(decalComp) && decalComp->GetDecalMaterial() != nullptr)
				decalComp->SetVisibility(false);
		}
		else {
			building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

			if (IsValid(decalComp) && decalComp->GetDecalMaterial() != nullptr)
				decalComp->SetVisibility(true);
		}
	}
}

void UBuildComponent::SetBuildingsOnPath()
{
	EndLocation = Buildings[0]->GetActorLocation();

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 0, 1.0f);

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

	FVector location = transform.GetLocation();

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

TArray<FItemStruct> UBuildComponent::GetBuildCosts()
{
	TArray<FItemStruct> items = Buildings[0]->CostList;

	for (int32 i = 0; i < items.Num(); i++)
		items[i].Amount *= Buildings.Num();

	return items;
}

bool UBuildComponent::CheckBuildCosts()
{
	UResourceManager* rm = Camera->ResourceManager;

	TArray<FItemStruct> items = GetBuildCosts();

	for (FItemStruct item : items) {
		int32 maxAmount = rm->GetResourceAmount(item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

bool UBuildComponent::IsValidLocation(ABuilding* building)
{
	if (building->Collisions.IsEmpty())
		return false;

	FVector location = building->BuildingMesh->GetRelativeLocation();

	if (StartLocation != FVector::Zero() && location.Z != StartLocation.Z)
		return false;

	bool bCoast = false;

	for (FCollisionStruct collision : building->Collisions) {
		if (collision.HISM == Camera->Grid->HISMLava || collision.HISM == Camera->Grid->HISMRiver)
			return false;

		if ((building->IsA(FoundationClass) && collision.Actor->IsA<AGrid>()) || (building->IsA<ARoad>() && collision.Actor->IsA<ARoad>() && building->GetActorLocation() != collision.Actor->GetActorLocation()))
			continue;
		
		FTransform transform;

		if (collision.HISM->IsValidLowLevelFast())
			collision.HISM->GetInstanceTransform(collision.Instance, transform);
		else
			transform = collision.Actor->GetTransform();

		if (building->IsA<AWall>() && collision.Actor->IsA<AWall>()) {
			if (FVector::Dist(building->GetActorLocation(), collision.Actor->GetActorLocation()) < Cast<ABuilding>(collision.Actor)->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().X)
				return false;
		}
		else if (building->IsA<AInternalProduction>() && collision.Actor->IsA(Cast<AInternalProduction>(building)->ResourceToOverlap)) {
			if (transform.GetLocation().X != building->GetActorLocation().X || transform.GetLocation().Y != building->GetActorLocation().Y)
				return false;
		}
		else if (transform.GetLocation().Z != FMath::Floor(building->GetActorLocation().Z) - 100.0f) {
			FRotator rotation = (building->GetActorLocation() - transform.GetLocation()).Rotation();

			if (building->IsA(WharfClass) && transform.GetLocation().Z < 0.0f && FMath::IsNearlyEqual(FMath::Abs(rotation.Yaw), FMath::Abs(building->GetActorRotation().Yaw - 90.0f)))
				bCoast = true;
			else
				return false;
		}
	}

	if (!bCoast && building->IsA(WharfClass))
		return false;

	return true;
}

void UBuildComponent::SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FVector location)
{
	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);

	Buildings.Add(building);

	Camera->DisplayInteract(Buildings[0]);

	if (building->IsA<ARoad>()) {
		TArray<AActor*> roads;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoad::StaticClass(), roads);

		for (AActor* road : roads)
			road->SetActorTickEnabled(true);
	}

	if (!building->Seeds.IsEmpty())
		Camera->SetSeedVisibility(true);
}

void UBuildComponent::DetachBuilding()
{
	if (Buildings.IsEmpty())
		return; 
	
	Camera->SetInteractStatus(Buildings[0], false);

	SetComponentTickEnabled(false);

	StartLocation = FVector::Zero();
	EndLocation = FVector::Zero();

	if (Buildings[0]->IsA<ARoad>()) {
		TArray<AActor*> roads;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoad::StaticClass(), roads);

		for (AActor* road : roads)
			road->SetActorTickEnabled(false);
	}

	Buildings.Empty();

	Camera->SetSeedVisibility(false);
}

void UBuildComponent::RemoveBuilding()
{
	if (Buildings.IsEmpty())
		return;

	for (ABuilding* building : Buildings) {
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 0, 1.0f);

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

		if (yaw % 45 == 0)
			yaw += 45;

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
	if (Buildings.IsEmpty() || !CheckBuildCosts()) {
		if (!CheckBuildCosts())
			Camera->ShowWarning("Cannot afford building");

		return;
	}	

	for (ABuilding* building : Buildings) {
		if (!IsValidLocation(building)) {
			Camera->ShowWarning("Invalid location");

			return;
		}
	}		

	if (StartLocation == FVector::Zero() && Buildings[0]->IsA<ARoad>()) {
		StartLocation = Buildings[0]->GetActorLocation();
		EndLocation = Buildings[0]->GetActorLocation();

		return;
	}

	Camera->MovementComponent->bShake = true;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	for (ABuilding* building : Buildings)
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->RemoveInstance(treeStruct.Instance);

	Camera->SetInteractAudioSound(PlaceSound, settings->GetMasterVolume() * settings->GetSFXVolume());
	Camera->PlayInteractSound();

	if (BuildingToMove->IsValidLowLevelFast()) {
		FVector diff = BuildingToMove->GetActorLocation() - Buildings[0]->GetActorLocation();

		BuildingToMove->SetActorLocation(Buildings[0]->GetActorLocation());
		BuildingToMove->SetActorRotation(Buildings[0]->GetActorRotation());

		BuildingToMove->StoreSocketLocations();

		TArray<ACitizen*> citizens = BuildingToMove->GetCitizensAtBuilding();

		for (ACitizen* citizen : citizens) {
			citizen->Building.EnterLocation += diff;

			BuildingToMove->SetSocketLocation(citizen);
		}

		UConstructionManager* cm = Camera->ConstructionManager;

		if (cm->IsBeingConstructed(BuildingToMove, nullptr)) {
			ABuilder* builder = cm->GetBuilder(BuildingToMove);

			if (builder != nullptr && !builder->GetOccupied().IsEmpty())
				builder->GetOccupied()[0]->AIController->RecalculateMovement(BuildingToMove);
		}

		for (ACitizen* citizen : BuildingToMove->GetOccupied())
			citizen->AIController->RecalculateMovement(BuildingToMove);

		BuildingToMove = nullptr;

		RemoveBuilding();

		return;
	}

	Buildings[0]->StoreSocketLocations();
	
	for (ABuilding* building : Buildings) {
		TArray<UDecalComponent*> decalComponents;
		building->GetComponents<UDecalComponent>(decalComponents);

		if (decalComponents.Num() >= 2)
			decalComponents[1]->SetVisibility(false);

		building->Build();
	}

	if (Camera->Start)
		Camera->OnBrochPlace(Buildings[0]);

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