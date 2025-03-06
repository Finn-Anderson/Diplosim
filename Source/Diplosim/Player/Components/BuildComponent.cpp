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

	if (Buildings.IsEmpty() || Camera->bInMenu || Camera->bMouseCapture)
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

		if (transform.GetRotation().X != 0.0f || transform.GetRotation().Y != 0.0f)
			return;
			
		FVector location = transform.GetLocation();

		if (Buildings[0]->GetActorLocation().X == location.X && Buildings[0]->GetActorLocation().Y == location.Y)
			return;

		if (Buildings[0]->IsA<AStockpile>())
			SetTileOpacity(Buildings[0], 1.0f);

		if (location.Z >= 0.0f) {
			if (comp == Camera->Grid->HISMRiver && comp->PerInstanceSMCustomData[instance * 4] == 1.0f && Buildings[0]->IsA(FoundationClass))
				location.Z += 30.0f;
			else
				location.Z += 100.0f;

			location.Z = FMath::RoundHalfFromZero(location.Z);

			Buildings[0]->SetActorLocation(location);

			if (StartLocation != FVector::Zero())
				SetBuildingsOnPath();
			else if (Buildings[0]->IsA<ARoad>())
				Cast<ARoad>(Buildings[0])->RegenerateMesh();
			else if (Buildings[0]->IsA<AStockpile>())
				SetTileOpacity(Buildings[0], 0.0f);
		}
	}

	for (ABuilding* building : Buildings) {
		if (building->IsHidden())
			continue;

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

void UBuildComponent::SetTileOpacity(ABuilding* Building, float Opacity)
{
	if (Building->GetActorLocation().Z <= 0.0f)
		return;
	
	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 x = Building->GetActorLocation().X / 100.0f + (bound / 2);
	int32 y = Building->GetActorLocation().Y / 100.0f + (bound / 2);

	FTileStruct tile = Camera->Grid->Storage[x][y];

	if (tile.bRamp || tile.bEdge)
		return;

	Camera->Grid->HISMFlatGround->SetCustomDataValue(tile.Instance, 1, Opacity);
}

void UBuildComponent::SetBuildingsOnPath()
{
	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 x = FMath::FloorToInt(StartLocation.X / 100.0f + bound / 2);
	int32 y = FMath::FloorToInt(StartLocation.Y / 100.0f + bound / 2);

	TArray<FVector> locations = CalculatePath(Camera->Grid->Storage[x][y], Camera->Grid->Storage[x][y]);

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		if (locations.Contains(Buildings[i]->GetActorLocation()))
			continue;

		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		if (Buildings[0]->IsA<AStockpile>())
			SetTileOpacity(Buildings[i], 1.0f);

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	for (FVector location : locations) {
		if (Buildings.Last()->IsA<ARoad>())
			Cast<ARoad>(Buildings.Last())->RegenerateMesh();

		if (IsValidLocation(Buildings.Last()) || Buildings.Num() == 1) {
			bool bBuildingExists = false;

			FHitResult hit(ForceInit);

			if (GetWorld()->LineTraceSingleByChannel(hit, location + FVector(0.0f, 0.0f, 50.0f), location, ECollisionChannel::ECC_Visibility))
				if (hit.GetActor()->IsA<ABuilding>() && !hit.GetActor()->IsHidden())
					bBuildingExists = true;

			if (Buildings.Last()->IsA(FoundationClass))
				location.Z -= 70.0f;

			if (!bBuildingExists)
				SpawnBuilding(Buildings[0]->GetClass(), location);

			Buildings.Last()->SetSeed(Buildings[0]->SeedNum);

			if (Buildings.Last()->IsA<AStockpile>())
				SetTileOpacity(Buildings.Last(), 0.0f);

			if (Buildings.Last()->IsA<ARoad>()) {
				ARoad* road = Cast<ARoad>(Buildings.Last());

				road->SetTier(Cast<ARoad>(Buildings[0])->Tier);

				road->RegenerateMesh();
			}
		}
		else
			Buildings.Last()->SetActorLocation(location);
	}

	if (Buildings.Num() == 1)
		return;

	if (!IsValidLocation(Buildings.Last())) {
		Buildings.Last()->DestroyBuilding();

		Buildings.RemoveAt(Buildings.Num() - 1);
	}
	else if (Buildings.Last()->IsA<AStockpile>())
		SetTileOpacity(Buildings.Last(), 0.0f);

	Camera->DisplayInteract(Buildings[0]);
}

TArray<FVector> UBuildComponent::CalculatePath(FTileStruct Tile, FTileStruct StartingTile, int32 z)
{
	TArray<FVector> locations;

	FTransform transform;
	transform = Camera->Grid->GetTransform(&Tile);

	FVector location = transform.GetLocation() + FVector(0.0f, 0.0f, z);

	locations.Add(location);

	FTileStruct closestTile = Tile;

	for (auto& element : Tile.AdjacentTiles) {
		if (element.Value->X != StartingTile.X && element.Value->Y != StartingTile.Y)
			continue;

		FVector currentClosestLoc = Camera->Grid->GetTransform(&closestTile).GetLocation();

		FVector newClosestLoc = Camera->Grid->GetTransform(element.Value).GetLocation();

		float currentDist = FVector::Dist(currentClosestLoc, Buildings[0]->GetActorLocation());
		float newDist = FVector::Dist(newClosestLoc, Buildings[0]->GetActorLocation());

		if (currentDist > newDist)
			closestTile = *element.Value;
	}

	z = 0.0f;

	if (closestTile.bRiver) {
		FTransform tf;
		tf = Camera->Grid->GetTransform(&closestTile);

		if (tf.GetLocation().Z < transform.GetLocation().Z)
			z += transform.GetLocation().Z - tf.GetLocation().Z;
	}

	if (closestTile != Tile)
		locations.Append(CalculatePath(closestTile, StartingTile));

	return locations;
}

TArray<FItemStruct> UBuildComponent::GetBuildCosts()
{
	TArray<FItemStruct> items;

	if (Buildings.IsEmpty() || Camera->bInstantBuildCheat)
		return items;
	
	items = Buildings[0]->CostList;

	int32 modifier = 0;

	if (StartLocation != FVector::Zero())
		modifier = 1;

	for (int32 i = 0; i < items.Num(); i++)
		items[i].Amount *= (Buildings.Num() - modifier);

	return items;
}

bool UBuildComponent::CheckBuildCosts()
{
	if (Camera->bInstantBuildCheat)
		return true;
	
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

	bool bCoast = false;
	bool bResource = false;

	for (FCollisionStruct collision : building->Collisions) {
		if (collision.HISM == Camera->Grid->HISMLava || (collision.HISM == Camera->Grid->HISMRampGround && !(building->IsA(FoundationClass) || building->IsA(RampClass))))
			return false;

		FTransform transform;

		if (collision.HISM->IsValidLowLevelFast())
			collision.HISM->GetInstanceTransform(collision.Instance, transform);
		else
			transform = collision.Actor->GetTransform();

		if (collision.HISM == Camera->Grid->HISMRiver && Camera->Grid->HISMRiver->PerInstanceSMCustomData[collision.Instance * 4] == 1.0f) {
			if (building->IsA<ARoad>()) {
				auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

				int32 x = building->GetActorLocation().X / 100.0f + bound / 2;
				int32 y = building->GetActorLocation().Y / 100.0f + bound / 2;

				FTileStruct tile = Camera->Grid->Storage[x][y];

				TArray<FTileStruct*> riverTiles;

				for (auto& element : tile.AdjacentTiles) {
					if (!element.Value->bRiver)
						continue;

					riverTiles.Add(element.Value);
				}

				if (riverTiles.Num() > 1) {
					if (riverTiles.Num() > 2)
						return false;

					if (riverTiles[0]->X != riverTiles[1]->X && riverTiles[0]->Y != riverTiles[1]->Y)
						return false;
				}

				return true;
			}
			else if (building->IsA(FoundationClass)) {
				return true;
			}
		}

		if (building->IsA<ARoad>() && collision.Actor->IsA<ARoad>())
			continue;

		if (building->IsA<AWall>() && collision.Actor->IsA<AWall>()) {
			if (FVector::Dist(building->GetActorLocation(), collision.Actor->GetActorLocation()) < Cast<ABuilding>(collision.Actor)->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().X)
				return false;
		}
		else if (building->IsA<AInternalProduction>() && Cast<AInternalProduction>(building)->ResourceToOverlap != nullptr && collision.Actor->IsA<AResource>()) {
			if (collision.Actor->IsA(Cast<AInternalProduction>(building)->ResourceToOverlap))
				bResource = true;

			if (transform.GetLocation().X != building->GetActorLocation().X || transform.GetLocation().Y != building->GetActorLocation().Y)
				return false;
		}
		else if (transform.GetLocation().Z != FMath::Floor(building->GetActorLocation().Z) - 100.0f) {
			FRotator rotation = (building->GetActorLocation() - transform.GetLocation()).Rotation();

			FVector size = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

			if (FMath::RoundToInt(building->GetActorRotation().Yaw / 90.0f) % 2 != 0)
				size = FVector(size.Y, size.X, size.Z);

			size = FVector(FMath::RoundHalfFromZero(size.X), FMath::RoundHalfFromZero(size.Y), FMath::RoundHalfFromZero(size.Z));

			if (building->bCoastal && transform.GetLocation().Z < 0.0f && FMath::IsNearlyEqual(FMath::Abs(rotation.Yaw), FMath::Abs(building->GetActorRotation().Yaw - 90.0f)))
				bCoast = true;
			else if (!collision.Actor->IsHidden() && FMath::Abs(building->GetActorLocation().X - transform.GetLocation().X) < size.X && FMath::Abs(building->GetActorLocation().Y - transform.GetLocation().Y) < size.Y)
				return false;
		}
	}

	if ((!bCoast && building->bCoastal) || (!bResource && building->IsA<AInternalProduction>()))
		return false;

	return true;
}

void UBuildComponent::SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FVector location)
{
	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);

	Buildings.Add(building);

	if (StartLocation == FVector::Zero())
		Camera->DisplayInteract(Buildings[0]);

	if (building->IsA<ARoad>())
		Cast<ARoad>(building)->RegenerateMesh();

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

	Buildings.Empty();

	Camera->SetSeedVisibility(false);
}

void UBuildComponent::RemoveBuilding()
{
	if (Buildings.IsEmpty())
		return;

	for (ABuilding* building : Buildings) {
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		if (building->IsA<AStockpile>())
			SetTileOpacity(building, 1.0f);

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

		if (Buildings.Num() > 1)
			SetBuildingsOnPath();
		else
			Buildings[0]->SetActorRotation(Rotation);
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

void UBuildComponent::StartPathPlace()
{
	if (StartLocation != FVector::Zero() || Buildings[0]->bUnique)
		return;
	
	StartLocation = Buildings[0]->GetActorLocation();
	Buildings[0]->BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);

	Buildings[0]->SetActorHiddenInGame(true);

	SetBuildingsOnPath();
}

void UBuildComponent::EndPathPlace()
{
	StartLocation = FVector::Zero();

	Buildings[0]->SetActorHiddenInGame(false);
	Buildings[0]->BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		if (Buildings[i]->IsA<AStockpile>())
			SetTileOpacity(Buildings[i], 1.0f);

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	Camera->DisplayInteract(Buildings[0]);
}

void UBuildComponent::Place()
{
	if (Buildings.IsEmpty() || !CheckBuildCosts()) {
		if (!CheckBuildCosts())
			Camera->ShowWarning("Cannot afford building");

		EndPathPlace();

		return;
	}

	for (ABuilding* building : Buildings) {
		if (building->IsHidden() && Buildings.Num() > 1)
			continue;

		if (!IsValidLocation(building) || (Buildings.Num() == 1 && !building->bUnique)) {
			Camera->ShowWarning("Invalid location");

			EndPathPlace();

			return;
		}
	}

	if (StartLocation != FVector::Zero()) {
		Buildings[0]->DestroyBuilding();

		Buildings.RemoveAt(0);
	}

	Camera->MovementComponent->bShake = true;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	for (ABuilding* building : Buildings)
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->RemoveInstance(treeStruct.Instance);

	Camera->SetInteractAudioSound(PlaceSound, settings->GetMasterVolume() * settings->GetSFXVolume());
	Camera->PlayInteractSound();

	if (Buildings[0]->IsA(FoundationClass))
		for (FCollisionStruct collision : Buildings[0]->Collisions)
			if (collision.HISM == Camera->Grid->HISMRiver)
				collision.HISM->PerInstanceSMCustomData[collision.Instance * 4] = 0.0f;

	if (BuildingToMove->IsValidLowLevelFast()) {
		FVector diff = BuildingToMove->GetActorLocation() - Buildings[0]->GetActorLocation();

		BuildingToMove->SetActorLocation(Buildings[0]->GetActorLocation());
		BuildingToMove->SetActorRotation(Buildings[0]->GetActorRotation());

		BuildingToMove->StoreSocketLocations();

		for (ACitizen* citizen : BuildingToMove->GetOccupied()) {
			if (!citizen->AIController->CanMoveTo(BuildingToMove->GetActorLocation())) {
				BuildingToMove->RemoveCitizen(citizen);

				continue;
			}

			if (BuildingToMove->GetCitizensAtBuilding().Contains(citizen)) {
				citizen->Building.EnterLocation += diff;

				BuildingToMove->SetSocketLocation(citizen);
			}
			else {
				citizen->AIController->RecalculateMovement(BuildingToMove);
			}
		}

		UConstructionManager* cm = Camera->ConstructionManager;

		if (cm->IsBeingConstructed(BuildingToMove, nullptr)) {
			ABuilder* builder = cm->GetBuilder(BuildingToMove);

			if (builder != nullptr && !builder->GetOccupied().IsEmpty())
				builder->GetOccupied()[0]->AIController->RecalculateMovement(BuildingToMove);
		}

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