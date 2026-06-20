#include "Player/Components/BuildComponent.h"

#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Research.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Production/Farm.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Gate.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Portal.h"
#include "Buildings/Misc/Special/Special.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Resources/Vegetation.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "DebugManager.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(1.0f / 60.0f);

	bCanRotate = true;

	Rotation = FRotator(0.0f);

	StartLocation = FVector::Zero();

	BuildingToMove, BlockedMaterial, BlueprintMaterial, InfluencedMaterial, Camera, PlaceSound = nullptr;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (Camera->bUIMode != 0)
		return;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction); 
	
	FScopeTryLock lock(&BuildLock);
	if (!lock.IsLocked() || DeltaTime > 1.0f || Buildings.IsEmpty() || Camera->bMouseCapture)
		return;

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 30000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		FVector location = hit.Location;
		location.X = FMath::RoundHalfFromZero(location.X / 100.0f) * 100.0f;
		location.Y = FMath::RoundHalfFromZero(location.Y / 100.0f) * 100.0f;
		
		FRotator rotation = GetBuildingRotation();
		rotation.Normalize();

		if (Buildings[0]->GetActorLocation().X == location.X && Buildings[0]->GetActorLocation().Y == location.Y && Buildings[0]->GetActorRotation() == rotation)
			return;

		auto bound = Camera->Grid->GetMapBounds();

		int32 x = FMath::FloorToInt(location.X / 100.0f + bound / 2.0f);
		int32 y = FMath::FloorToInt(location.Y / 100.0f + bound / 2.0f);

		if (x < 0 || x >= bound || y < 0 || y >= bound)
			return;

		GetBuildLocationZ(Buildings[0], location);

		FVector prevLocation = Buildings[0]->GetActorLocation();

		Buildings[0]->SetActorLocation(location);
		Buildings[0]->SetActorRotation(rotation);

		if (Buildings[0]->IsA<AWall>())
			Cast<AWall>(Buildings.Last())->SetRotationMesh(Rotation.Yaw);
		else if (Buildings[0]->IsA<ARoad>()) {
			ARoad* road = Cast<ARoad>(Buildings.Last());
			road->HISMRoad->SetRelativeRotation(FRotator(0.0f, -rotation.Yaw, 0.0f));

			road->RegenerateMesh(true);
		}

		if (StartLocation != FVector::Zero())
			SetBuildingsOnPath();

		SetTreeStatus(Buildings[0], false, false, prevLocation);

		if (Camera->WidgetComponent->bHiddenInGame)
			Camera->WidgetComponent->SetHiddenInGame(false);
	}

	for (ABuilding* building : Buildings) {
		if (building->IsHidden())
			continue;

		if ((StartLocation == FVector::Zero() && !IsValidLocation(building)) || !CheckBuildCosts()) {
			building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);

			if (building->IsA<AResearch>()) {
				AResearch* station = Cast<AResearch>(building);

				station->TurretMesh->SetOverlayMaterial(BlockedMaterial);
				station->TelescopeMesh->SetOverlayMaterial(BlockedMaterial);
			}

			DisplayInfluencedBuildings(building, false);
			building->ToggleDecalComponentVisibility(false);
		}
		else {
			building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

			if (building->IsA<AResearch>()) {
				AResearch* station = Cast<AResearch>(building);

				station->TurretMesh->SetOverlayMaterial(BlueprintMaterial);
				station->TelescopeMesh->SetOverlayMaterial(BlueprintMaterial);
			}

			DisplayInfluencedBuildings(building, true);
			building->ToggleDecalComponentVisibility(true);
		}
	}
}

void UBuildComponent::GetBuildLocationZ(ABuilding* Building, FVector& Location)
{
	FHitResult hit(ForceInit);

	FCollisionQueryParams params;
	for (ABuilding* building : Buildings)
		params.AddIgnoredActor(building);

	if (GetWorld()->LineTraceSingleByChannel(hit, FVector(Location.X, Location.Y, 1000.0f), FVector(Location.X, Location.Y, 0.0f), ECollisionChannel::ECC_GameTraceChannel1, params)) {
		Location.Z = FMath::RoundHalfFromZero(hit.Location.Z);

		if ((hit.GetComponent() == Camera->Grid->HISMRiver || hit.GetComponent() == Camera->Grid->HISMSea) && (Building->IsA<ARoad>() || (Building->bCanBuildOnBridge)))
			Location.Z += 20.0f;
		else if (IsValid(hit.GetComponent()) && Building->IsA(FoundationClass)) {
			if (hit.GetComponent() == Camera->Grid->HISMRiver)
				Location.Z -= 55.0f;
			else if (hit.GetComponent() == Camera->Grid->HISMSea || hit.GetComponent() == Camera->Grid->HISMRampGround)
				Location.Z -= 50.0f;
		}
	}
}

TArray<FHitResult> UBuildComponent::GetBuildingOverlaps(AActor* Actor, float Extent, FVector Location)
{
	TArray<FHitResult> hits;

	FCollisionQueryParams params;
	params.AddIgnoredActor(Actor);

	if (!Buildings.IsEmpty() && StartLocation != FVector::Zero())
		params.AddIgnoredActor(Buildings[0]);

	FRotator rotation = Actor->GetActorRotation();

	FVector centre, size;
	Cast<UStaticMeshComponent>(Actor->GetRootComponent())->GetStaticMesh()->GetBounds().GetBox().GetCenterAndExtents(centre, size);

	if (Location == FVector::Zero() || Location == Actor->GetActorLocation())
		Location = Actor->GetActorLocation() + rotation.RotateVector(centre);

	TArray<FVector> points;
	float xInterval = 1.0f / FMath::Max(FMath::RoundHalfFromZero(size.X / 50.0f), 1);
	float yInterval = 1.0f / FMath::Max(FMath::RoundHalfFromZero(size.Y / 50.0f), 1);

	float biggestDimension = size.X;
	if (size.Y > biggestDimension)
		biggestDimension = size.Y;

	for (float x = -1.0f; x <= 1.1f; x += xInterval)
		for (float y = -1.0f; y <= 1.1f; y += yInterval)
			points.Add(Location + rotation.RotateVector(size * Extent * FVector(x, y, 0.0f)));

	float height = size.Z + 100.0f;

	for (const FVector& point : points) {
		TArray<FHitResult> hts;

		if (GetWorld()->LineTraceMultiByChannel(hts, FVector(point.X, point.Y, point.Z + height), FVector(point.X, point.Y, 0.0f), ECC_GameTraceChannel1, params))
			hits.Append(hts);
	}

	return hits;
}

void UBuildComponent::SetTreeStatus(AActor* Actor, bool bDestroy, bool bRemoveBuilding, FVector PrevLocation)
{
	if (Actor->IsHidden() && bDestroy)
		return;

	TArray<FResourceHISMStruct> vegetation;
	vegetation.Append(Camera->Grid->TreeStruct);
	if (bDestroy)
		vegetation.Append(Camera->Grid->FlowerStruct);

	FVector size = Cast<UStaticMeshComponent>(Actor->GetRootComponent())->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;

	for (FResourceHISMStruct resource : vegetation) {
		UInstancedStaticMeshComponent* hism = resource.Resource->ResourceHISM;
		float radius = FMath::Sqrt(FMath::Square(size.X) + FMath::Square(size.Y));

		TArray<int32> instances = hism->GetInstancesOverlappingSphere(Actor->GetActorLocation(), radius);
		instances.Sort();

		for (int32 instance : hism->GetInstancesOverlappingSphere(PrevLocation, radius)) {
			if (!bRemoveBuilding && instances.Contains(instance))
				continue;

			hism->SetCustomDataValue(instance, 1, 1.0f);
		}

		if (bDestroy)
			Camera->Grid->RemoveTree(resource.Resource, instances);
		else if (!bRemoveBuilding)
			for (int32 instance : instances)
				hism->SetCustomDataValue(instance, 1, 0.0f);
	}
}

void UBuildComponent::DisplayInfluencedBuildings(class ABuilding* Building, bool bShow)
{
	if (!Building->IsA<ABooster>() || (Building->IsHidden() && bShow))
		return;

	ABooster* booster = Cast<ABooster>(Building);
	booster->DecalComponent->DecalSize = FVector(booster->Range);

	TArray<ABuilding*> influencedBuildings = booster->GetAffectedBuildings();

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(booster->FactionName);

	for (ABuilding* b : faction->Buildings) {
		if (bShow && influencedBuildings.Contains(b))
			b->BuildingMesh->SetOverlayMaterial(InfluencedMaterial);
		else
			b->BuildingMesh->SetOverlayMaterial(nullptr);
	}
}

void UBuildComponent::SetBuildingsOnPath()
{
	FVector endLocation = Buildings[0]->GetActorLocation();

	FTileStruct* startTile = Camera->Grid->GetTileFromLocation(StartLocation);
	FTileStruct* endTile = Camera->Grid->GetTileFromLocation(endLocation);

	TArray<FVector> locations = CalculatePath(startTile, endTile);

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		if (locations.Contains(Buildings[i]->GetActorLocation())) {
			locations.Remove(Buildings[i]->GetActorLocation());

			continue;
		}

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	for (const FVector& location : locations) {
		Buildings[0]->SetActorLocation(location);

		if (IsValidLocation(Buildings[0])) {
			SpawnBuilding(Buildings[0]->GetClass(), Buildings[0]->FactionName, location);

			Buildings.Last()->SeedNum = Buildings[0]->SeedNum;
			Buildings.Last()->SetTier(Buildings[0]->GetTier());

			if (Buildings[0]->IsA<AWall>())
				Cast<AWall>(Buildings.Last())->SetRotationMesh(Rotation.Yaw); 
			else if (Buildings[0]->IsA<ARoad>())
				Cast<ARoad>(Buildings.Last())->RegenerateMesh(true);

			SetTreeStatus(Buildings.Last(), false);

			Buildings.Last()->ToggleDecalComponentVisibility(Buildings[0]->GetDecalComponentVisibility());
		}
	}

	Buildings[0]->SetActorLocation(endLocation);

	Camera->DisplayInteract(Buildings[0]);
}

TArray<FVector> UBuildComponent::CalculatePath(FTileStruct* StartTile, FTileStruct* EndTile)
{
	TArray<FVector> locations;

	int32 x = FMath::Abs(StartTile->X - EndTile->X);
	int32 y = FMath::Abs(StartTile->Y - EndTile->Y);

	int32 xSign = 1;
	int32 ySign = 1;

	if (StartTile->X > EndTile->X)
		xSign = -1;

	if (StartTile->Y > EndTile->Y)
		ySign = -1;

	bool bDiagonal = false;

	if (y == x || (x < y && x > y / 2.0f) || (y < x && y > x / 2.0f))
		bDiagonal = true;

	int32 num = x;

	if (x < y)
		num = y;

	for (int32 i = 0; i <= num; i++) {
		FVector location = StartLocation;

		if (x < y || bDiagonal)
			location.Y += (i * ySign * 100.0f);

		if (y < x || bDiagonal)
			location.X += (i * xSign * 100.0f);

		GetBuildLocationZ(Buildings[0], location);

		locations.Add(location);
	}

	return locations;
}

TArray<FItemStruct> UBuildComponent::GetBuildCosts()
{
	TArray<FItemStruct> items;

	if (Buildings.IsEmpty())
		return items;

	int32 count = 0;

	for (ABuilding* building : Buildings) {
		if (StartLocation != FVector::Zero() && building == Buildings[0])
			continue;

		TArray<FItemStruct> cost = building->CostList;

		FFactionStruct* faction = Camera->ConquestManager->GetFaction(building->FactionName);

		if (faction == nullptr)
			continue;

		for (ABuilding* b : faction->Buildings) {
			if (b->GetClass() != building->GetClass() || b->GetActorLocation() != building->GetActorLocation())
				continue;

			cost = b->GetGradeCost();

			break;
		}

		for (const FItemStruct& item : cost) {
			int32 index = items.Find(item);

			if (index == INDEX_NONE)
				items.Add(item);
			else
				items[index].Amount += item.Amount;
		}
	}

	return items;
}

bool UBuildComponent::CheckBuildCosts()
{
	if (Cast<UDebugManager>(Camera->PController->CheatManager)->bInstantBuildCheat)
		return true;
	
	UResourceManager* rm = Camera->ResourceManager;

	TArray<FItemStruct> items = GetBuildCosts();

	for (const FItemStruct& item : items) {
		int32 maxAmount = rm->GetResourceAmount(Buildings[0]->FactionName, item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

bool UBuildComponent::IsValidLocation(AActor* Actor, float Extent, FVector Location)
{
	if (Location == FVector::Zero())
		Location = Actor->GetActorLocation();

	if (Location.Z < 0.0f)
		return false;

	TArray<FHitResult> hits = GetBuildingOverlaps(Actor, Extent, Location);

	if (hits.IsEmpty())
		return false;

	bool bCoast = false;
	bool bResource = false;

	for (const FHitResult& hit : hits) {
		if (hit.GetActor()->IsHidden() || hit.GetActor()->IsA<AVegetation>() || hit.GetActor()->IsA<AAI>())
			continue;

		if (hit.GetComponent() == Camera->Grid->HISMLava || (hit.GetComponent() == Camera->Grid->HISMRampGround && !Actor->IsA(FoundationClass) && !Actor->IsA<ARoad>()))
			return false;

		FTransform transform;
		if (IsValid(hit.GetComponent()) && hit.GetComponent()->IsA<UInstancedStaticMeshComponent>() && !hit.GetActor()->IsA<ARoad>())
			Cast<UInstancedStaticMeshComponent>(hit.GetComponent())->GetInstanceTransform(hit.Item, transform);
		else
			transform = hit.GetActor()->GetTransform();

		if (hit.GetComponent() == Camera->Grid->HISMSea || hit.GetComponent() == Camera->Grid->HISMRiver) {
			FVector location = transform.GetLocation();

			if (hit.GetComponent() == Camera->Grid->HISMSea) {
				auto bound = Camera->Grid->GetMapBounds();

				int32 x = FMath::RoundHalfFromZero(hit.Location.X / 100.0f) * 100.0f;
				int32 y = FMath::RoundHalfFromZero(hit.Location.Y / 100.0f) * 100.0f;
				int32 z = location.Z;

				location = FVector(x, y, z);
			}
			else if (hit.GetComponent() == Camera->Grid->HISMRiver && Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->bCanBuildOnBridge) {
				ARoad* bridge = nullptr;
				for (const FHitResult& ht : hits) {
					if (ht.GetActor()->IsA<ARoad>()) {
						bridge = Cast<ARoad>(ht.GetActor());

						break;
					}
				}

				if (!IsValid(bridge))
					return false;

				FRotator rotation = Actor->GetActorRotation() - bridge->GetActorRotation();
				rotation.Normalize();

				if (FMath::RoundToInt32(rotation.Yaw) % 90 == 0)
					continue;
				else
					return false;
			}

			FRotator rotation = (location - Location).Rotation() - FRotator(0.0f, 90.0f, 0.0f);
			rotation.Normalize();

			if (Actor->IsA(FoundationClass) || (Actor->IsA<ARoad>() && Cast<ARoad>(Actor)->SeedNum > 0))
				continue;
			else if (Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->bCoastal && FMath::IsNearlyEqual(rotation.Yaw, Actor->GetActorRotation().Yaw)) {
				bCoast = true;

				continue;
			}
			else
				return false;
		}
		else if (!Actor->IsA<ASpecial>() && hit.GetComponent() == Camera->Grid->HISMGround && FMath::RoundHalfFromZero(Actor->GetActorLocation().Z - 50.0f) < FMath::RoundHalfFromZero(transform.GetLocation().Z)) {
			UStaticMeshComponent* mesh = Cast<UStaticMeshComponent>(Actor->GetRootComponent());

			FVector location;
			mesh->GetClosestPointOnCollision(transform.GetLocation(), location);

			if (FVector::Dist(location, transform.GetLocation()) >= 50.0f)
				continue;
		}

		FTileStruct* tile = Camera->Grid->GetTileFromLocation(hit.Location);

		if (tile == nullptr || (tile->bMineral && (Camera->Start || (Actor->IsA<ABuilding>() && !Cast<ABuilding>(Actor)->bCanMove))))
			return false;
		else if (Actor->IsA<ABuilding>() && hit.GetActor()->IsA<ABuilding>() && !Actor->IsA(FoundationClass) && !hit.GetActor()->IsA(FoundationClass)) {
			ABuilding* building = Cast<ABuilding>(Actor);
			ABuilding* hitBuilding = Cast<ABuilding>(hit.GetActor());

			if (building->CanBuildOnTop != hitBuilding->CanBuildOnTop && (!building->IsA<AFestival>() || !building->IsActorTickEnabled()) && (!hitBuilding->IsA<AFestival>() || !hitBuilding->IsActorTickEnabled()) && (building->CanBuildOnTop != EBuildOnTop::Bridge || hitBuilding->bCanBuildOnBridge) && (hitBuilding->CanBuildOnTop != EBuildOnTop::Bridge || building->bCanBuildOnBridge))
				continue;
		}

		transform.SetLocation(FVector(FMath::RoundHalfFromZero(transform.GetLocation().X), FMath::RoundHalfFromZero(transform.GetLocation().Y), FMath::RoundHalfFromZero(transform.GetLocation().Z)));

		if (Location == transform.GetLocation()) {
			if (Actor->GetClass() == hit.GetActor()->GetClass()) {
				if (!Actor->IsA<ABuilding>())
					return false;

				ABuilding* building = Cast<ABuilding>(Actor);
				ABuilding* hitBuilding = Cast<ABuilding>(hit.GetActor());

				if (building->SeedNum == hitBuilding->SeedNum && building->Tier == hitBuilding->Tier)
					return false;
				else
					continue;
			}
			else if (Actor->IsA<AInternalProduction>() && IsValid(Cast<AInternalProduction>(Actor)->ResourceToOverlap) && hit.GetActor()->IsA<AResource>()) {
				AInternalProduction* ipBuilding = Cast<AInternalProduction>(Actor);

				if (hit.GetActor()->IsA(ipBuilding->ResourceToOverlap)) {
					bResource = true;
				}
				else {
					for (int32 i = 0; i < ipBuilding->Seeds.Num(); i++) {
						if (!hit.GetActor()->IsA(ipBuilding->Seeds[i].Resource))
							continue;

						ipBuilding->SetSeed(i);

						break;
					}
				}

				continue;
			}
			else
				return false;
		}
		else if (Actor->IsA<ARoad>() && hit.GetComponent() == Camera->Grid->HISMRampGround)
			continue;

		if (transform.GetLocation().Z != FMath::Floor(Location.Z) - 100.0f || hit.GetComponent() == Camera->Grid->HISMRiver) {
			if ((Actor->IsA(FoundationClass) && (hit.GetActor()->IsA<ABuilding>() || hit.GetActor()->IsA<AGrid>())) || (Actor->IsA(RampClass) && (hit.GetActor()->IsA(RampClass) || hit.GetActor()->IsA<AGrid>())))
				continue;
			else if (!hit.GetActor()->IsHidden())
				return false;
		}
	}

	if (Actor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(Actor);

		if ((!bCoast && building->bCoastal) || (!bResource && Actor->IsA<AInternalProduction>() && IsValid(Cast<AInternalProduction>(Actor)->ResourceToOverlap)))
			return false;

		if (Camera->ConquestManager->Factions.Num() > 1) {
			for (FFactionStruct faction : Camera->ConquestManager->Factions) {
				if (faction.Name == building->FactionName)
					continue;

				for (ABuilding* b : faction.Buildings)
					if (FVector::Dist(building->GetActorLocation(), b->GetActorLocation()) < 500.0f)
						return false;
			}
		}
	}

	return true;
}

void UBuildComponent::SpawnBuilding(TSubclassOf<class ABuilding> BuildingClass, FString FactionName, FVector location)
{
	if (!IsComponentTickEnabled())
		SetComponentTickEnabled(true);

	FActorSpawnParameters params;
	params.bNoFail = true;

	FRotator rotation = GetBuildingRotation();
	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, rotation, params);
	building->FactionName = FactionName;

	if (building->IsA<ARoad>())
		Cast<ARoad>(building)->HISMRoad->SetRelativeRotation(FRotator(0.0f, -rotation.Yaw, 0.0f));

	Buildings.Add(building);

	if (StartLocation == FVector::Zero()) {
		Camera->Detach();
		Camera->DisplayInteract(Buildings[0]);
		Camera->WidgetComponent->SetHiddenInGame(true);
	}

	if (IsValid(BuildingToMove)) {
		building->SeedNum = BuildingToMove->SeedNum;
		building->SetTier(BuildingToMove->Tier);
	}
	else if (!building->Seeds.IsEmpty())
		Camera->SetSeedVisibility(true);

	DisplayInfluencedBuildings(building, false);
	building->ToggleDecalComponentVisibility(false);
}

void UBuildComponent::ResetBuilding(ABuilding* Building)
{
	StartLocation = FVector::Zero();

	Buildings.Empty();

	Building->SetActorHiddenInGame(false);

	Buildings.Add(Building);
}

void UBuildComponent::DetachBuilding()
{
	StartLocation = FVector::Zero();
	
	Camera->SetInteractStatus(nullptr, false);

	SetComponentTickEnabled(false);

	Buildings.Empty();
	BuildingToMove = nullptr;

	Camera->SetSeedVisibility(false);
}

void UBuildComponent::RemoveBuilding()
{
	if (Buildings.IsEmpty())
		return;

	for (ABuilding* building : Buildings) {
		DisplayInfluencedBuildings(building, false);

		SetTreeStatus(building, false, true, building->GetActorLocation());

		building->DestroyBuilding(true, IsValid(BuildingToMove));
	}

	DetachBuilding();
}

void UBuildComponent::RotateBuilding(bool Rotate)
{
	if (Buildings.IsEmpty())
		return;

	if (Rotate && bCanRotate) {
		int32 yaw = Rotation.Yaw;

		if (yaw % 45 == 0)
			yaw += 45;

		if (yaw == 360)
			yaw = 0;

		bCanRotate = false;

		Rotation.Yaw = yaw;
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

FRotator UBuildComponent::GetBuildingRotation()
{
	if (Buildings.IsEmpty() || !Buildings[0]->IsA<ARoad>())
		return Rotation;

	FVector location = Buildings[0]->GetActorLocation();
	FRotator rotation = Rotation;

	FHitResult hit;
	FCollisionQueryParams params;
	for (ABuilding* building : Buildings)
		params.AddIgnoredActor(building);

	if (GetWorld()->LineTraceSingleByChannel(hit, FVector(location.X, location.Y, location.Z + 10.0f), FVector(location.X, location.Y, 0.0f), ECollisionChannel::ECC_GameTraceChannel1, params) && (hit.GetComponent() == Camera->Grid->HISMRampGround || hit.GetActor()->IsA(RampClass))) {
		FTransform transform;
		if (hit.GetComponent() == Camera->Grid->HISMRampGround)
			Camera->Grid->HISMRampGround->GetInstanceTransform(hit.Item, transform);
		else {
			transform = hit.GetActor()->GetTransform();
			transform.SetRotation(transform.GetRotation() + FRotator(0.0f, 90.0f, 0.0f).Quaternion());
		}

		FVector targetLocation = transform.GetLocation() - transform.GetRotation().Rotator().Vector() * 50.0f + FVector(0.0f, 0.0f, 75.0f);
		rotation = (targetLocation - location).Rotation();
	}
	else {
		rotation.Pitch = 0.0f;
		rotation.Roll = 0.0f;
	}

	return rotation;
}

void UBuildComponent::StartPathPlace()
{
	if (StartLocation != FVector::Zero() || Buildings[0]->bUnique || BuildingToMove || (Buildings[0]->IsA<AInternalProduction>() && IsValid(Cast<AInternalProduction>(Buildings[0])->ResourceToOverlap)))
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
		SetTreeStatus(Buildings[i], false, true, Buildings[i]->GetActorLocation());

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}
}

void UBuildComponent::Place(bool bQuick)
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

		if (!IsValidLocation(building) || (Buildings.Num() == 1 && Buildings[0]->IsHidden())) {
			Camera->ShowWarning("Invalid location");

			EndPathPlace();

			return;
		}
	}

	Camera->MovementComponent->bShake = true;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		ABuilding* building = Buildings[i];

		TArray<FHitResult> hits = GetBuildingOverlaps(building);

		for (const FHitResult& hit : hits) {
			if (building->GetActorLocation() != hit.GetActor()->GetActorLocation() || building->GetClass() != hit.GetActor()->GetClass())
				continue;

			ABuilding* b = Cast<ABuilding>(hit.GetActor());
			b->SeedNum = building->SeedNum;
			b->Build(false, true, building->Tier);

			building->DestroyBuilding();

			Buildings.RemoveAt(i);

			break;
		}
	}

	ABuilding* b = Buildings[0];

	if (StartLocation != FVector::Zero()) {
		if (!bQuick)
			Buildings[0]->DestroyBuilding();

		Buildings.RemoveAt(0);
	}

	for (ABuilding* building : Buildings) {
		DisplayInfluencedBuildings(building, false);

		SetTreeStatus(building, true);

		FVector dimensions = Buildings[0]->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;
		float size = dimensions.X;
		if (dimensions.Y > size)
			size = dimensions.Y;

		FFactionStruct* faction = Camera->ConquestManager->GetFaction(building->FactionName);

		FOverlapsStruct overlaps;
		overlaps.GetEveryPawn();
		TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, building, size, overlaps, EFactionType::Both, faction);

		if (actors.IsEmpty())
			continue;

		FVector location = building->GetActorLocation();
		if (building->BuildingMesh->DoesSocketExist("Entrance"))
			location = building->BuildingMesh->GetSocketLocation("Entrance");

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

		FNavLocation targetLoc;
		nav->ProjectPointToNavigation(location, targetLoc, FVector(400.0f, 400.0f, 40.0f));
		location = targetLoc.Location;

		for (AActor* actor : actors)
			Cast<AAI>(actor)->MovementComponent->Transform.SetLocation(location);

		for (ACitizen* citizen : faction->Citizens)
			if (!actors.Contains(citizen) && !citizen->MovementComponent->Points.IsEmpty() && FVector::Dist(citizen->MovementComponent->Points.Last(), building->GetActorLocation()) < size)
				citizen->AIController->DefaultAction();
	}

	if (!Buildings.IsEmpty() && (Buildings[0]->IsA(FoundationClass) || Buildings[0]->IsA(RampClass) || Buildings[0]->IsA<ARoad>())) {
		if (!Buildings[0]->IsA<ARoad>()) {
			for (ABuilding* building : Buildings) {
				FTileStruct* tile = Camera->Grid->GetTileFromLocation(building->GetActorLocation());

				if (Buildings[0]->IsA(FoundationClass))
					tile->Level++;
				else
					tile->bRamp = true;

				tile->Rotation = (building->GetActorRotation() + FRotator(0.0f, 90.0f, 0.0f)).Quaternion();
			}
		}
	}

	if (IsValid(BuildingToMove)) {
		FVector diff = BuildingToMove->GetActorLocation() - Buildings[0]->GetActorLocation();

		BuildingToMove->SetActorLocation(Buildings[0]->GetActorLocation());
		BuildingToMove->SetActorRotation(Buildings[0]->GetActorRotation());

		BuildingToMove->SetSeed(Buildings[0]->SeedNum);

		BuildingToMove->StoreSocketLocations();

		for (ACitizen* citizen : BuildingToMove->GetOccupied()) {
			if (!citizen->AIController->CanMoveTo(BuildingToMove->GetActorLocation())) {
				BuildingToMove->RemoveCitizen(citizen);
			}
			else if (BuildingToMove->GetCitizensAtBuilding().Contains(citizen)) {
				citizen->BuildingComponent->EnterLocation += diff;

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

		if (BuildingToMove->IsA(Camera->PoliceManager->PoliceStationClass)) {
			FFactionStruct* faction = Camera->ConquestManager->GetFaction(BuildingToMove->FactionName);

			for (ACitizen* citizen : faction->Citizens) {
				if (citizen->BuildingComponent->BuildingAt != BuildingToMove || !faction->Police.Arrested.Contains(citizen))
					continue;

				Camera->PoliceManager->SetInNearestJail(*faction, nullptr, citizen);
			}
		}

		BuildingToMove = nullptr;

		RemoveBuilding();

		return;
	}

	TArray<ABuilding*> bs = Buildings;

	if (bQuick)
		ResetBuilding(b);
	else
		DetachBuilding();
	
	for (ABuilding* building : bs) {
		TArray<UDecalComponent*> decalComponents;
		building->GetComponents<UDecalComponent>(decalComponents);

		for (UDecalComponent* decalComp : decalComponents)
			decalComp->SetVisibility(false);

		building->StoreSocketLocations();

		building->Build();

		Camera->PlayAmbientSound(building->AmbientAudioComponent, PlaceSound);
	}

	if (Camera->Start)
		Camera->OnEggTimerPlace(b);
}

void UBuildComponent::QuickPlace()
{
	if (Camera->Start || BuildingToMove != nullptr) {
		Place();

		return;
	}

	Place(true);
}