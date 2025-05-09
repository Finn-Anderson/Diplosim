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
#include "Buildings/Work/Service/Research.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Map/Grid.h"
#include "Map/Resources/Vegetation.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Festival.h"
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

	if (DeltaTime > 1.0f || Buildings.IsEmpty() || Camera->bInMenu || Camera->bMouseCapture)
		return;

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 30000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		UHierarchicalInstancedStaticMeshComponent* comp = nullptr;
		int32 instance = 0;

		FTransform transform;

		if (hit.GetActor()->IsA<ABuilding>()) {
			transform = hit.GetActor()->GetTransform();
		}
		else {
			comp = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.GetComponent());
			instance = hit.Item;

			comp->GetInstanceTransform(instance, transform);
		}

		if (transform.GetRotation().X != 0.0f || transform.GetRotation().Y != 0.0f)
			return;
			
		FVector location = transform.GetLocation();

		if (Buildings[0]->GetActorLocation().X == location.X && Buildings[0]->GetActorLocation().Y == location.Y && Buildings[0]->GetActorRotation() == Rotation)
			return;

		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

		int32 x = FMath::FloorToInt(location.X / 100.0f + bound / 2);
		int32 y = FMath::FloorToInt(location.Y / 100.0f + bound / 2);

		if (x < 0 || x >= Camera->Grid->Storage.Num() || y < 0 || y >= Camera->Grid->Storage.Num())
			return;

		FCollisionQueryParams params;
		params.AddIgnoredActor(Camera->Grid);

		FHitResult hit2(ForceInit);

		if (GetWorld()->LineTraceSingleByChannel(hit2, transform.GetLocation() + FVector(0.0f, 0.0f, 20000.0f), transform.GetLocation(), ECollisionChannel::ECC_GameTraceChannel1, params) && hit2.GetActor()->IsA(FoundationClass))
			location = hit2.GetActor()->GetActorLocation();

		bool bCalculate = false;

		if (location.Z >= 0.0f) {
			if (IsValid(comp) && comp == Camera->Grid->HISMRiver && comp->PerInstanceSMCustomData[instance * 4] == 1.0f && Buildings[0]->IsA(FoundationClass))
				location.Z += 30.0f;
			else if (IsValid(hit2.GetActor()))
				location.Z += 75.0f;
			else
				location.Z += 100.0f;

			location.Z = FMath::RoundHalfFromZero(location.Z);

			bCalculate = true;
		}
		else if (Buildings[0]->IsA(FoundationClass)) {
			location.Z = 25.0f;

			bCalculate = true;
		}

		if (bCalculate) {
			if (Buildings[0]->IsA(FoundationClass)) {
				if (hit.GetComponent() == Camera->Grid->HISMRampGround)
					location.Z -= 100.0f;

			}

			Buildings[0]->SetActorLocation(location);

			if (StartLocation != FVector::Zero())
				SetBuildingsOnPath();
			else if (Buildings[0]->IsA<ARoad>())
				Cast<ARoad>(Buildings[0])->RegenerateMesh();
		}
	}

	for (ABuilding* building : Buildings) {
		if (building->IsHidden())
			continue;

		UDecalComponent* decalComp = building->FindComponentByClass<UDecalComponent>();

		if (!IsValidLocation(building) || !CheckBuildCosts()) {
			building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);

			if (building->IsA<ARoad>())
				Cast<ARoad>(building)->HISMRoad->SetOverlayMaterial(BlockedMaterial);

			if (building->IsA<AResearch>()) {
				AResearch* station = Cast<AResearch>(building);

				station->TurretMesh->SetOverlayMaterial(BlockedMaterial);
				station->TelescopeMesh->SetOverlayMaterial(BlockedMaterial);
			}

			if (IsValid(decalComp) && decalComp->GetDecalMaterial() != nullptr)
				decalComp->SetVisibility(false);
		}
		else {
			building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);

			if (building->IsA<ARoad>())
				Cast<ARoad>(building)->HISMRoad->SetOverlayMaterial(BlueprintMaterial);

			if (building->IsA<AResearch>()) {
				AResearch* station = Cast<AResearch>(building);

				station->TurretMesh->SetOverlayMaterial(BlueprintMaterial);
				station->TelescopeMesh->SetOverlayMaterial(BlueprintMaterial);
			}

			if (IsValid(decalComp) && decalComp->GetDecalMaterial() != nullptr)
				decalComp->SetVisibility(true);
		}
	}
}

void UBuildComponent::SetBuildingsOnPath()
{
	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	int32 xStart = FMath::FloorToInt(StartLocation.X / 100.0f + bound / 2);
	int32 yStart = FMath::FloorToInt(StartLocation.Y / 100.0f + bound / 2);

	int32 xEnd = FMath::FloorToInt(Buildings[0]->GetActorLocation().X / 100.0f + bound / 2);
	int32 yEnd = FMath::FloorToInt(Buildings[0]->GetActorLocation().Y / 100.0f + bound / 2);

	TArray<FVector> locations = CalculatePath(Camera->Grid->Storage[xStart][yStart], Camera->Grid->Storage[xEnd][yEnd]);

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		if (locations.Contains(Buildings[i]->GetActorLocation()))
			continue;

		int32 x = FMath::FloorToInt(Buildings[i]->GetActorLocation().X / 100.0f + bound / 2);
		int32 y = FMath::FloorToInt(Buildings[i]->GetActorLocation().Y / 100.0f + bound / 2);

		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		Buildings[i]->DestroyBuilding();

		Buildings.RemoveAt(i);
	}

	for (FVector location : locations) {
		if (Buildings.Last()->IsA<ARoad>())
			Cast<ARoad>(Buildings.Last())->RegenerateMesh();

		if (IsValidLocation(Buildings.Last()) || Buildings.Num() == 1) {
			SpawnBuilding(Buildings[0]->GetClass(), location);

			Buildings.Last()->SetSeed(Buildings[0]->SeedNum);

			if (Buildings[0]->IsA<AWall>())
				Cast<AWall>(Buildings.Last())->SetRotationMesh(Rotation.Yaw);

			int32 x = FMath::FloorToInt(location.X / 100.0f + bound / 2);
			int32 y = FMath::FloorToInt(location.Y / 100.0f + bound / 2);

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

	int32 x = FMath::FloorToInt(Buildings.Last()->GetActorLocation().X / 100.0f + bound / 2);
	int32 y = FMath::FloorToInt(Buildings.Last()->GetActorLocation().Y / 100.0f + bound / 2);

	if (!IsValidLocation(Buildings.Last())) {
		Buildings.Last()->DestroyBuilding();

		Buildings.RemoveAt(Buildings.Num() - 1);
	}

	Camera->DisplayInteract(Buildings[0]);
}

TArray<FVector> UBuildComponent::CalculatePath(FTileStruct StartTile, FTileStruct EndTile)
{
	TArray<FVector> locations;

	int32 x = FMath::Abs(StartTile.X - EndTile.X);
	int32 y = FMath::Abs(StartTile.Y - EndTile.Y);

	int32 xSign = 1;
	int32 ySign = 1;

	if (StartTile.X > EndTile.X)
		xSign = -1;

	if (StartTile.Y > EndTile.Y)
		ySign = -1;

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	bool bDiagonal = false;

	if (y == x || (x < y && x > y / 2.0f) || (y < x && y > x / 2.0f))
		bDiagonal = true;

	int32 num = x;

	if (x < y)
		num = y;

	for (int32 i = 0; i <= num; i++) {
		int32 xi = StartTile.X + bound / 2;
		int32 yi = StartTile.Y + bound / 2;

		if (x < y || bDiagonal)
			yi += (i * ySign);

		if (y < x || bDiagonal)
			xi += (i * xSign);

		FTileStruct* tile = &Camera->Grid->Storage[xi][yi];

		FTransform transform;
		transform = Camera->Grid->GetTransform(tile);
		
		FCollisionQueryParams params;
		params.AddIgnoredActor(Camera->Grid);

		FHitResult hit(ForceInit);

		if (GetWorld()->LineTraceSingleByChannel(hit, transform.GetLocation() + FVector(0.0f, 0.0f, 20000.0f), transform.GetLocation(), ECollisionChannel::ECC_GameTraceChannel1, params) && hit.GetActor()->IsA(FoundationClass) && !Buildings.Contains(hit.GetActor())) {
			transform = hit.GetActor()->GetTransform();

			transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, 75.0f));
		}

		if (transform.GetLocation().Z < 0.0f)
			transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, FMath::Abs(transform.GetLocation().Z) + 25.0f));
		else if (Buildings[0]->IsA(FoundationClass) && tile->bRamp)
			transform.SetLocation(transform.GetLocation() - FVector(0.0f, 0.0f, 100.0f));

		locations.Add(transform.GetLocation());
	}

	return locations;
}

TArray<FItemStruct> UBuildComponent::GetBuildCosts()
{
	TArray<FItemStruct> items;

	if (Buildings.IsEmpty() || Camera->bInstantBuildCheat)
		return items;

	int32 count = 0;

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		ABuilding* building = Buildings[i];

		for (FCollisionStruct collision : building->Collisions) {
			if (building->GetClass() != collision.Actor->GetClass() || building->SeedNum == Cast<ABuilding>(collision.Actor)->SeedNum || building->GetActorLocation() != collision.Actor->GetActorLocation())
				continue;

			ABuilding* b = Cast<ABuilding>(collision.Actor);
			TArray<FItemStruct> cost = b->GetGradeCost(building->SeedNum);

			for (FItemStruct item : cost) {
				int32 index = items.Find(item);

				if (index == INDEX_NONE)
					items.Add(item);
				else
					items[index].Amount += item.Amount;
			}

			count++;
		}
	}
	
	items = Buildings[0]->CostList;

	int32 modifier = 0;

	if (StartLocation != FVector::Zero())
		modifier = 1;

	for (int32 i = 0; i < items.Num(); i++)
		items[i].Amount *= (Buildings.Num() - count - modifier);

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
		if (collision.Actor->IsHidden() || collision.HISM == Camera->Grid->HISMWall)
			continue;

		if (collision.HISM == Camera->Grid->HISMLava || (collision.HISM == Camera->Grid->HISMRampGround && !(building->IsA(FoundationClass) || building->IsA(RampClass))))
			return false;

		FTransform transform;

		if (collision.HISM->IsValidLowLevelFast())
			collision.HISM->GetInstanceTransform(collision.Instance, transform);
		else
			transform = collision.Actor->GetTransform();

		if (collision.HISM == Camera->Grid->HISMRiver && Camera->Grid->HISMRiver->PerInstanceSMCustomData[collision.Instance * 4] == 1.0f) {
			if (building->IsA<ARoad>()) {
				ARoad* road = Cast<ARoad>(building);

				if (road->BuildingMesh->GetStaticMesh() == road->RoadMeshes[0])
					return false;

				return true;
			}
			else if (building->IsA(FoundationClass)) {
				return true;
			}
		}

		if (building->GetClass() == collision.Actor->GetClass() && building->GetActorLocation() == collision.Actor->GetActorLocation()) {
			if (building->SeedNum == Cast<ABuilding>(collision.Actor)->SeedNum)
				return false;
			else
				continue;
		}

		if (building->IsA<ARoad>() && collision.Actor->IsA<ARoad>())
			continue;

		if (building->IsA<AInternalProduction>() && Cast<AInternalProduction>(building)->ResourceToOverlap != nullptr && collision.Actor->IsA<AResource>()) {
			if (collision.Actor->IsA(Cast<AInternalProduction>(building)->ResourceToOverlap))
				bResource = true;

			if (transform.GetLocation().X != building->GetActorLocation().X || transform.GetLocation().Y != building->GetActorLocation().Y)
				return false;
		}
		else if (transform.GetLocation().Z != FMath::Floor(building->GetActorLocation().Z) - 100.0f || collision.HISM == Camera->Grid->HISMRiver) {
			if (collision.Actor->IsA(FoundationClass) && (building->IsA(FoundationClass) || FMath::RoundHalfFromZero(building->GetActorLocation().Z - collision.Actor->GetActorLocation().Z) > 0.0f))
				continue;
			else if (collision.Actor->IsA<AGrid>() && IsValid(collision.HISM) && building->IsA(FoundationClass)) {
				auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

				int32 x = FMath::FloorToInt(transform.GetLocation().X / 100.0f + bound / 2);
				int32 y = FMath::FloorToInt(transform.GetLocation().Y / 100.0f + bound / 2);

				if (x < 0 || x >= Camera->Grid->Storage.Num() || y < 0 || y >= Camera->Grid->Storage.Num())
					return false;

				FTileStruct* tile = &Camera->Grid->Storage[x][y];

				float z = tile->Level * 75.0f;

				if (transform.GetLocation().Z != z || tile->bRamp)
					continue;
			}

			FRotator rotation = (building->GetActorLocation() - transform.GetLocation()).Rotation();

			FCollisionQueryParams params;
			params.AddIgnoredActor(collision.Actor);
			params.AddIgnoredActor(Camera->Grid);

			FVector extent;
			FVector offset = FVector::Zero();
			
			if (IsValid(collision.HISM)) {
				extent = (collision.HISM->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f);
				offset = FVector(0.0f, 0.0f, 100.0f - extent.Z);
				extent *= 0.7f;
			}
			else {
				UStaticMeshComponent* mesh = collision.Actor->FindComponentByClass<UStaticMeshComponent>();

				extent = (mesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f) * 0.8f;
				offset = FVector(0.0f, 0.0f, extent.Z);
			}

			FHitResult hit;

			if (building->bCoastal && transform.GetLocation().Z < 0.0f && FMath::IsNearlyEqual(FMath::Abs(rotation.Yaw), FMath::Abs(building->GetActorRotation().Yaw - 90.0f)))
				bCoast = true;
			else if (GetWorld()->SweepSingleByChannel(hit, transform.GetLocation() + offset, transform.GetLocation() + offset, FQuat(0.0f), ECollisionChannel::ECC_PhysicsBody, FCollisionShape::MakeBox(extent), params) && !hit.GetActor()->IsHidden() && hit.GetActor() == building)
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

	FActorSpawnParameters params;
	params.bNoFail = true;

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation, params);

	Buildings.Add(building);

	if (StartLocation == FVector::Zero())
		Camera->DisplayInteract(Buildings[0]);

	if (building->IsA<ARoad>())
		Cast<ARoad>(building)->RegenerateMesh();

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

	Camera->SetSeedVisibility(false);
}

void UBuildComponent::RemoveBuilding()
{
	if (Buildings.IsEmpty())
		return;

	for (ABuilding* building : Buildings) {
		for (FTreeStruct treeStruct : building->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

		int32 x = FMath::FloorToInt(building->GetActorLocation().X / 100.0f + bound / 2);
		int32 y = FMath::FloorToInt(building->GetActorLocation().Y / 100.0f + bound / 2);

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

		for (ABuilding* building : Buildings) {
			building->SetActorRotation(Rotation);

			if (building->IsA<AWall>())
				Cast<AWall>(building)->SetRotationMesh(yaw);
		}
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

	auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		for (FTreeStruct treeStruct : Buildings[i]->TreeList)
			treeStruct.Resource->ResourceHISM->SetCustomDataValue(treeStruct.Instance, 1, 1.0f);

		int32 x = FMath::FloorToInt(Buildings[i]->GetActorLocation().X / 100.0f + bound / 2);
		int32 y = FMath::FloorToInt(Buildings[i]->GetActorLocation().Y / 100.0f + bound / 2);

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

	Camera->SetInteractAudioSound(PlaceSound, settings->GetMasterVolume() * settings->GetSFXVolume());
	Camera->PlayInteractSound();

	for (int32 i = Buildings.Num() - 1; i > 0; i--) {
		ABuilding* building = Buildings[i];

		for (FCollisionStruct collision : building->Collisions) {
			if (building->GetClass() != collision.Actor->GetClass() || building->SeedNum == Cast<ABuilding>(collision.Actor)->SeedNum || building->GetActorLocation() != collision.Actor->GetActorLocation())
				continue;

			ABuilding* b = Cast<ABuilding>(collision.Actor);
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

	for (ABuilding* building : Buildings)
		for (FTreeStruct treeStruct : building->TreeList)
			Camera->Grid->RemoveTree(treeStruct.Resource, treeStruct.Instance);

	if (!Buildings.IsEmpty() && (Buildings[0]->IsA(FoundationClass) || Buildings[0]->IsA(RampClass) || Buildings[0]->IsA<ARoad>())) {
		for (ABuilding* building : Buildings)
			RemoveWalls(building);

		if (!Buildings[0]->IsA<ARoad>()) {
			for (ABuilding* building : Buildings) {
				int32 buildingX = FMath::RoundHalfFromZero(building->GetActorLocation().X);
				int32 buildingY = FMath::RoundHalfFromZero(building->GetActorLocation().Y);

				auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

				int32 x = buildingX / 100.0f + (bound / 2);
				int32 y = buildingY / 100.0f + (bound / 2);

				FTileStruct* tile = &Camera->Grid->Storage[x][y];

				if (Buildings[0]->IsA(FoundationClass))
					tile->Level++;
				else
					tile->bRamp = true;
			}

			for (ABuilding* building : Buildings) {
				int32 buildingX = FMath::RoundHalfFromZero(building->GetActorLocation().X);
				int32 buildingY = FMath::RoundHalfFromZero(building->GetActorLocation().Y);

				auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Camera->Grid->Size));

				int32 x = buildingX / 100.0f + (bound / 2);
				int32 y = buildingY / 100.0f + (bound / 2);

				FTileStruct* tile = &Camera->Grid->Storage[x][y];
				tile->Rotation = (building->GetActorRotation() + FRotator(0.0f, 90.0f, 0.0f)).Quaternion();

				Camera->Grid->CreateEdgeWalls(tile);
			}
		}

		Camera->Grid->HISMWall->BuildTreeIfOutdated(true, false);
	}

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
	
	for (ABuilding* building : Buildings) {
		TArray<UDecalComponent*> decalComponents;
		building->GetComponents<UDecalComponent>(decalComponents);

		if (decalComponents.Num() >= 2)
			decalComponents[1]->SetVisibility(false);

		building->StoreSocketLocations();

		building->Build();
	}

	if (Camera->Start)
		Camera->OnBrochPlace(Buildings[0]);

	if (bQuick)
		ResetBuilding(b);
	else
		DetachBuilding();
}

void UBuildComponent::QuickPlace()
{
	if (Camera->Start || BuildingToMove->IsValidLowLevelFast()) {
		Place();

		return;
	}

	Place(true);
}

void UBuildComponent::RemoveWalls(ABuilding* Building)
{
	int32 buildingX = FMath::RoundHalfFromZero(Building->GetActorLocation().X);
	int32 buildingY = FMath::RoundHalfFromZero(Building->GetActorLocation().Y);

	TArray<FVector> ends;
	ends.Add(Building->GetActorLocation() + FVector(60.0f, 0.0f, 75.0f));
	ends.Add(Building->GetActorLocation() + FVector(-60.0f, 0.0f, 75.0f));
	ends.Add(Building->GetActorLocation() + FVector(0.0f, 60.0f, 75.0f));
	ends.Add(Building->GetActorLocation() + FVector(0.0f, -60.0f, 75.0f));

	FCollisionQueryParams params;
	params.AddIgnoredActor(Building);

	FHitResult hit(ForceInit);

	for (FVector end : ends) {
		if (!GetWorld()->LineTraceSingleByChannel(hit, Building->GetActorLocation() + FVector(0.0f, 0.0f, 75.0f), end, ECollisionChannel::ECC_Destructible, params) || hit.GetComponent() != Camera->Grid->HISMWall)
			continue;

		if (Building->IsA(RampClass)) {
			FTransform transform;
			Camera->Grid->HISMWall->GetInstanceTransform(hit.Item, transform);

			if (transform.GetLocation().Z > Building->GetActorLocation().Z && FMath::RoundHalfFromZero(Building->GetActorRotation().Yaw - 90.0f) != FMath::RoundHalfFromZero((transform.GetLocation() - Building->GetActorLocation()).Rotation().Yaw))
				continue;
		}

		Camera->Grid->HISMWall->RemoveInstance(hit.Item);
	}

	for (FCollisionStruct collision : Building->Collisions)
		if (collision.HISM == Camera->Grid->HISMRiver && Building->IsA(FoundationClass))
			collision.HISM->PerInstanceSMCustomData[collision.Instance * 4] = 0.0f;

	Camera->Grid->HISMWall->BuildTreeIfOutdated(true, false);
}