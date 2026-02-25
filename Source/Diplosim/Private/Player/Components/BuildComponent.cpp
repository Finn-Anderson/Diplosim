#include "Player/Components/BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Research.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Map/Grid.h"
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

	bCanRotate = true;

	Rotation.Yaw = 0.0f;

	StartLocation = FVector::Zero();

	BuildingToMove = nullptr;

	lastUpdatedTime = 0.0f;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction); 

	lastUpdatedTime += DeltaTime;

	if (lastUpdatedTime < 1 / 60.0f)
		return;
	
	FScopeTryLock lock(&BuildLock);
	if (!lock.IsLocked() || DeltaTime > 1.0f || Buildings.IsEmpty() || Camera->IsUIHoveredOver() || Camera->bMouseCapture)
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

		if (Buildings[0]->GetActorLocation().X == location.X && Buildings[0]->GetActorLocation().Y == location.Y && Buildings[0]->GetActorRotation() == Rotation)
			return;

		auto bound = Camera->Grid->GetMapBounds();

		int32 x = FMath::FloorToInt(location.X / 100.0f + bound / 2);
		int32 y = FMath::FloorToInt(location.Y / 100.0f + bound / 2);

		if (x < 0 || x >= bound || y < 0 || y >= bound)
			return;

		GetBuildLocationZ(Buildings[0], location);

		FVector prevLocation = Buildings[0]->GetActorLocation();

		Buildings[0]->SetActorLocation(location);
		Buildings[0]->SetActorRotation(Rotation);

		if (Buildings[0]->IsA<AWall>())
			Cast<AWall>(Buildings[0])->SetRotationMesh(Rotation.Yaw);

		if (StartLocation != FVector::Zero())
			SetBuildingsOnPath();

		if (Buildings[0]->IsA<ARoad>() && !Buildings[0]->IsHidden())
			Cast<ARoad>(Buildings[0])->SetTier(Cast<ARoad>(Buildings[0])->Tier);

		if (FVector::Dist(prevLocation, location) <= 100.0f)
			prevLocation = FVector::Zero();

		SetTreeStatus(Buildings[0], false, false, prevLocation);
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

			if (building->DecalComponent->GetDecalMaterial() != nullptr)
				building->DecalComponent->SetVisibility(false);
		}
		else {
			building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

			if (building->IsA<AResearch>()) {
				AResearch* station = Cast<AResearch>(building);

				station->TurretMesh->SetOverlayMaterial(BlueprintMaterial);
				station->TelescopeMesh->SetOverlayMaterial(BlueprintMaterial);
			}

			DisplayInfluencedBuildings(building, true);

			if (building->DecalComponent->GetDecalMaterial() != nullptr)
				building->DecalComponent->SetVisibility(true);
		}
	}
}

void UBuildComponent::GetBuildLocationZ(ABuilding* Building, FVector& Location)
{
	FHitResult hit2(ForceInit);

	FCollisionQueryParams params;
	params.AddIgnoredActor(Building);

	if (GetWorld()->LineTraceSingleByChannel(hit2, Location, FVector(Location.X, Location.Y, 0.0f), ECollisionChannel::ECC_GameTraceChannel1, params))
		Location.Z = FMath::RoundHalfFromZero(hit2.Location.Z);

	if (IsValid(hit2.GetComponent()) && Building->IsA(FoundationClass)) {
		if (hit2.GetComponent() == Camera->Grid->HISMRiver)
			Location.Z -= 55.0f;
		else if (hit2.GetComponent() == Camera->Grid->HISMSea)
			Location.Z -= 25.0f;
		else if (hit2.GetComponent() == Camera->Grid->HISMRampGround)
			Location.Z -= 50.0f;
	}
	else if (hit2.GetComponent() == Camera->Grid->HISMRiver && Building->IsA<ARoad>())
		Location.Z += 20.0f;
}

TArray<FHitResult> UBuildComponent::GetBuildingOverlaps(ABuilding* Building, float Extent, FVector Location)
{
	TArray<FHitResult> hits;

	FCollisionQueryParams params;
	params.AddIgnoredActor(Building);

	if (!Buildings.IsEmpty())
		params.AddIgnoredActor(Buildings[0]);

	FRotator rotation = Building->GetActorRotation();

	FVector centre, size;
	Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetCenterAndExtents(centre, size);

	if (Location == FVector::Zero())
		Location = Building->GetActorLocation() + rotation.RotateVector(centre);

	TArray<FVector> points;

	for (float x = -1.0f; x <= 1.0f; x += 0.5f)
		for (float y = -1.0f; y <= 1.0f; y += 0.5f)
			points.Add(Location + rotation.RotateVector(size * Extent * FVector(x, y, 0.0f)));

	for (FVector point : points) {
		FHitResult hit(ForceInit);

		if (GetWorld()->LineTraceSingleByChannel(hit, FVector(point.X, point.Y, 1000.0f), FVector(point.X, point.Y, 0.0f), ECC_Vehicle, params))
			hits.Add(hit);
	}

	return hits;
}

void UBuildComponent::SetTreeStatus(ABuilding* Building, bool bDestroy, bool bRemoveBuilding, FVector PrevLocation)
{
	TArray<FResourceHISMStruct> vegetation;
	vegetation.Append(Camera->Grid->TreeStruct);
	vegetation.Append(Camera->Grid->FlowerStruct);

	FVector size = Building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;

	for (FResourceHISMStruct resource : vegetation) {
		UHierarchicalInstancedStaticMeshComponent* hism = resource.Resource->ResourceHISM;

		TArray<int32> instances = hism->GetInstancesOverlappingSphere(Building->GetActorLocation(), FMath::Sqrt(FMath::Square(size.X) + FMath::Square(size.Y)) + 50.0f);

		if (PrevLocation != FVector::Zero())
			instances.Append(hism->GetInstancesOverlappingSphere(PrevLocation, FMath::Sqrt(FMath::Square(size.X) + FMath::Square(size.Y)) + 50.0f));

		instances.Sort();

		for (int32 i = instances.Num() - 1; i > -1; i--) {
			FTransform transform;
			hism->GetInstanceTransform(instances[i], transform);

			if (bRemoveBuilding || FMath::Abs(transform.GetLocation().X - Building->GetActorLocation().X) > size.X + 25.0f || FMath::Abs(transform.GetLocation().Y - Building->GetActorLocation().Y) > size.Y + 25.0f)
				hism->PerInstanceSMCustomData[instances[i] * hism->NumCustomDataFloats + 1] = 1.0f;
			else if (bDestroy)
				Camera->Grid->RemoveTree(resource.Resource, instances[i]);
			else 
				hism->PerInstanceSMCustomData[instances[i] * hism->NumCustomDataFloats + 1] = 0.0f;

			hism->bIsOutOfDate = true;
		}

		hism->BuildTreeIfOutdated(true, false);
	}
}

void UBuildComponent::DisplayInfluencedBuildings(class ABuilding* Building, bool bShow)
{
	if (!Building->IsA<ABooster>() || Building->IsHidden())
		return;

	ABooster* booster = Cast<ABooster>(Building);

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
		if (locations.Contains(Buildings[i]->GetActorLocation()))
			continue;

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	for (FVector location : locations) {
		Buildings[0]->SetActorLocation(location);

		if (IsValidLocation(Buildings[0])) {
			SpawnBuilding(Buildings[0]->GetClass(), Buildings[0]->FactionName, location);

			Buildings.Last()->SetSeed(Buildings[0]->SeedNum);

			if (Buildings[0]->IsA<AWall>())
				Cast<AWall>(Buildings.Last())->SetRotationMesh(Rotation.Yaw); 
			else if (Buildings[0]->IsA<ARoad>())
				Cast<ARoad>(Buildings.Last())->SetTier(Cast<ARoad>(Buildings.Last())->Tier);

			SetTreeStatus(Buildings.Last(), false);
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

		FHitResult hit(ForceInit);

		if (GetWorld()->LineTraceSingleByChannel(hit, FVector(location.X, location.Y, 1000.0f), FVector(location.X, location.Y, 0.0f), ECollisionChannel::ECC_GameTraceChannel1))
			location.Z = FMath::RoundHalfFromZero(hit.Location.Z);

		if (IsValid(hit.GetComponent()) && Buildings[0]->IsA(FoundationClass)) {
			if (hit.GetComponent() == Camera->Grid->HISMRiver)
				location.Z -= 55.0f;
			else if (hit.GetComponent() == Camera->Grid->HISMSea)
				location.Z -= 25.0f;
			else if (hit.GetComponent() == Camera->Grid->HISMRampGround)
				location.Z -= 50.0f;
		}
		else if (hit.GetComponent() == Camera->Grid->HISMRiver && Buildings[0]->IsA<ARoad>())
			location.Z += 20.0f;

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
			if (!b->IsA(building->GetClass()) || b->GetActorLocation() != building->GetActorLocation())
				continue;

			cost = b->GetGradeCost(building->SeedNum);

			break;
		}

		for (FItemStruct item : cost) {
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

	for (FItemStruct item : items) {
		int32 maxAmount = rm->GetResourceAmount(Buildings[0]->FactionName, item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

bool UBuildComponent::IsValidLocation(ABuilding* Building, float Extent, FVector Location)
{
	TArray<FHitResult> hits = GetBuildingOverlaps(Building, Extent, Location);

	if (hits.IsEmpty())
		return false;

	bool bCoast = false;
	bool bResource = false;

	for (FHitResult hit : hits) {
		if (hit.GetActor()->IsHidden() || hit.GetActor()->IsA<AVegetation>() || hit.GetActor()->IsA<AAI>())
			continue;

		if (hit.GetComponent() == Camera->Grid->HISMLava || (hit.GetComponent() == Camera->Grid->HISMRampGround && !(Building->IsA(FoundationClass) || Building->IsA(RampClass))))
			return false;

		if (hit.GetComponent() == Camera->Grid->HISMSea) {
			if (Building->IsA(FoundationClass))
				continue;

			return false;
		}

		if (Camera->Grid->GetTileFromLocation(hit.Location)->bMineral && Building->FactionName != Camera->ColonyName && (!Building->IsA<AInternalProduction>() || Cast<AInternalProduction>(Building)->ResourceToOverlap == nullptr))
			return false;

		FTransform transform;

		if (IsValid(hit.GetComponent()) && hit.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>())
			Cast<UHierarchicalInstancedStaticMeshComponent>(hit.GetComponent())->GetInstanceTransform(hit.Item, transform);
		else
			transform = hit.GetActor()->GetTransform();

		if (hit.GetComponent() == Camera->Grid->HISMRiver && Camera->Grid->HISMRiver->PerInstanceSMCustomData[hit.Item * 4] == 1.0f) {
			if (Building->IsA<ARoad>()) {
				ARoad* road = Cast<ARoad>(Building);
				road->SetTier(road->GetTier());

				if (road->BuildingMesh->GetStaticMesh() == road->RoadMeshes[0])
					return false;

				return true;
			}
			else if (Building->IsA(FoundationClass)) {
				return true;
			}
		}

		if (Building->GetClass() == hit.GetActor()->GetClass() && Building->GetActorLocation() == hit.GetActor()->GetActorLocation()) {
			if (Building->SeedNum == Cast<ABuilding>(hit.GetActor())->SeedNum)
				return false;
			else
				continue;
		}

		if (Building->IsA<ARoad>() && hit.GetActor()->IsA<ARoad>())
			continue;

		if (Building->IsA<AInternalProduction>() && IsValid(Cast<AInternalProduction>(Building)->ResourceToOverlap) && hit.GetActor()->IsA<AResource>()) {
			if (hit.GetActor()->IsA(Cast<AInternalProduction>(Building)->ResourceToOverlap)) {
				bResource = true;
			}
			else {
				for (int32 i = 0; i < Building->Seeds.Num(); i++) {
					if (!hit.GetActor()->IsA(Building->Seeds[i].Resource))
						continue;

					Building->SetSeed(i); 
					
					break;
				}
			}

			if (transform.GetLocation().X != Building->GetActorLocation().X || transform.GetLocation().Y != Building->GetActorLocation().Y)
				return false;
		}
		else if (transform.GetLocation().Z != FMath::Floor(Building->GetActorLocation().Z) - 100.0f || hit.GetComponent() == Camera->Grid->HISMRiver) {
			if ((Building->IsA(FoundationClass) && (hit.GetActor()->IsA(FoundationClass) || hit.GetActor()->IsA<AGrid>())) || (Building->IsA(RampClass) && (hit.GetActor()->IsA(RampClass) || hit.GetActor()->IsA<AGrid>())))
				continue;

			FRotator rotation = (Building->GetActorLocation() - transform.GetLocation()).Rotation();

			if (Building->bCoastal && transform.GetLocation().Z < 0.0f && FMath::IsNearlyEqual(FMath::Abs(rotation.Yaw), FMath::Abs(Building->GetActorRotation().Yaw - 90.0f)))
				bCoast = true;
			else if (!hit.GetActor()->IsHidden())
				return false;
		}
	}

	if ((!bCoast && Building->bCoastal) || (!bResource && Building->IsA<AInternalProduction>() && !Building->IsA<ASpecial>()))
		return false;

	if (Camera->ConquestManager->Factions.Num() > 1) {
		for (FFactionStruct faction : Camera->ConquestManager->Factions) {
			if (faction.Name == Building->FactionName)
				continue;

			for (ABuilding* building : faction.Buildings)
				if (FVector::Dist(Building->GetActorLocation(), building->GetActorLocation()) < 500.0f)
					return false;
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

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation, params);
	building->FactionName = FactionName;

	Buildings.Add(building);

	if (StartLocation == FVector::Zero()) {
		Camera->Detach();
		Camera->DisplayInteract(Buildings[0]);
	}

	if (!building->Seeds.IsEmpty())
		Camera->SetSeedVisibility(true);
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

		SetTreeStatus(building, false, true);

		building->DestroyBuilding(true, IsValid(BuildingToMove));
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
	}
	else if (!Rotate) {
		bCanRotate = true;
	}
}

void UBuildComponent::StartPathPlace()
{
	if (StartLocation != FVector::Zero() || Buildings[0]->bUnique || BuildingToMove)
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
		SetTreeStatus(Buildings[i], false, true);

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	Camera->DisplayInteract(Buildings[0]);
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

		if (!IsValidLocation(building) || (Buildings.Num() == 1 && !building->bUnique)) {
			Camera->ShowWarning("Invalid location");

			EndPathPlace();

			return;
		}
	}

	Camera->MovementComponent->bShake = true;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	FVector dimensions = Buildings[0]->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 100.0f;
	float size = FMath::Sqrt(FMath::Sqrt((dimensions.X * dimensions.Y * dimensions.Z) / 10.0f));
	float pitch = 1.0f / size;

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		ABuilding* building = Buildings[i];

		TArray<FHitResult> hits = GetBuildingOverlaps(building);

		for (FHitResult hit : hits) {
			if (building->GetClass() != hit.GetActor()->GetClass() || building->SeedNum == Cast<ABuilding>(hit.GetActor())->SeedNum || building->GetActorLocation() != hit.GetActor()->GetActorLocation())
				continue;

			ABuilding* b = Cast<ABuilding>(hit.GetActor());
			b->Build(false, true, building->SeedNum);

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
	
	for (ABuilding* building : Buildings) {
		TArray<UDecalComponent*> decalComponents;
		building->GetComponents<UDecalComponent>(decalComponents);

		for (UDecalComponent* decalComp : decalComponents)
			decalComp->SetVisibility(false);

		building->StoreSocketLocations();

		building->Build();

		Camera->PlayAmbientSound(building->AmbientAudioComponent, PlaceSound);
	}

	if (Camera->Start)
		Camera->OnEggTimerPlace(Buildings[0]);

	if (bQuick)
		ResetBuilding(b);
	else
		DetachBuilding();
}

void UBuildComponent::QuickPlace()
{
	if (Camera->Start || BuildingToMove != nullptr) {
		Place();

		return;
	}

	Place(true);
}