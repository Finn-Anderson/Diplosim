#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Misc/ScopeTryLock.h"
#include "EngineUtils.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Research.h"

UAIVisualiser::UAIVisualiser()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.SetAllChannels(ECR_Ignore);
	response.Visibility = ECR_Block;

	AIContainer = CreateDefaultSubobject<USceneComponent>(TEXT("AIContainer"));

	HISMCitizen = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMCitizen"));
	HISMCitizen->SetupAttachment(AIContainer);
	HISMCitizen->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMCitizen->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMCitizen->SetCollisionResponseToChannels(response);
	HISMCitizen->SetCanEverAffectNavigation(false);
	HISMCitizen->SetGenerateOverlapEvents(false);
	HISMCitizen->bWorldPositionOffsetWritesVelocity = false;
	HISMCitizen->bAutoRebuildTreeOnInstanceChanges = false;
	HISMCitizen->NumCustomDataFloats = 13;

	HISMClone = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClone"));
	HISMClone->SetupAttachment(AIContainer);
	HISMClone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMClone->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMClone->SetCollisionResponseToChannels(response);
	HISMClone->SetCanEverAffectNavigation(false);
	HISMClone->SetGenerateOverlapEvents(false);
	HISMClone->bWorldPositionOffsetWritesVelocity = false;
	HISMClone->bAutoRebuildTreeOnInstanceChanges = false;
	HISMClone->NumCustomDataFloats = 9;

	HISMRebel = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRebel"));
	HISMRebel->SetupAttachment(AIContainer);
	HISMRebel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMRebel->SetCollisionObjectType(ECC_GameTraceChannel3);
	HISMRebel->SetCollisionResponseToChannels(response);
	HISMRebel->SetCanEverAffectNavigation(false);
	HISMRebel->SetGenerateOverlapEvents(false);
	HISMRebel->bWorldPositionOffsetWritesVelocity = false;
	HISMRebel->bAutoRebuildTreeOnInstanceChanges = false;
	HISMRebel->NumCustomDataFloats = 12;

	HISMEnemy = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMEnemy"));
	HISMEnemy->SetupAttachment(AIContainer);
	HISMEnemy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HISMEnemy->SetCollisionObjectType(ECC_GameTraceChannel4);
	HISMEnemy->SetCollisionResponseToChannels(response);
	HISMEnemy->SetCanEverAffectNavigation(false);
	HISMEnemy->SetGenerateOverlapEvents(false);
	HISMEnemy->bWorldPositionOffsetWritesVelocity = false;
	HISMEnemy->bAutoRebuildTreeOnInstanceChanges = false;
	HISMEnemy->NumCustomDataFloats = 9;

	TorchNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TorchNiagaraComponent"));
	TorchNiagaraComponent->SetupAttachment(AIContainer);
	TorchNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	TorchNiagaraComponent->bAutoActivate = false;

	DiseaseNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DiseaseNiagaraComponent"));
	DiseaseNiagaraComponent->SetupAttachment(AIContainer);
	DiseaseNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	DiseaseNiagaraComponent->bAutoActivate = false;

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetupAttachment(AIContainer);
	HarvestNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	HarvestNiagaraComponent->bAutoActivate = true;

	HatsContainer = CreateDefaultSubobject<USceneComponent>(TEXT("HatsContainer"));
	HatsContainer->SetupAttachment(AIContainer);

	hatsNum = 0;
	bCitizensMoving = false;
	bAIMoving = false;
	bBuildingsDying = false;
	bBuildingsRotating = false;
	bHatsMoving = false;

	// ECC_WorldDynamic, ECC_Pawn, ECC_GameTraceChannel2, ECC_GameTraceChannel3, ECC_GameTraceChannel4: Consider ECR_Block
}

void UAIVisualiser::BeginPlay()
{
	Super::BeginPlay();

	for (auto& element : HatsMeshesList) {
		FString name = element.Key->GetName() + "Component";

		FHatsStruct hatsStruct;
		hatsStruct.HISMHat = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *name);
		hatsStruct.HISMHat->SetStaticMesh(element.Key);
		hatsStruct.HISMHat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		hatsStruct.HISMHat->SetCanEverAffectNavigation(false);
		hatsStruct.HISMHat->SetGenerateOverlapEvents(false);
		hatsStruct.HISMHat->bWorldPositionOffsetWritesVelocity = false;
		hatsStruct.HISMHat->bAutoRebuildTreeOnInstanceChanges = false;
		hatsStruct.HISMHat->NumCustomDataFloats = element.Value;
		hatsStruct.HISMHat->RegisterComponent();

		HISMHats.Add(hatsStruct);
	}
}

void UAIVisualiser::MainLoop(ACamera* Camera)
{
	for (FPendingChangeStruct pending : PendingChange) {
		if (pending.Instance == -1) {
			int32 instance = pending.HISM->AddInstance(pending.Transform, false);

			if (IsValid(pending.AI->SpawnSystem))
				UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), pending.AI->SpawnSystem, pending.Transform.GetLocation());

			FLinearColor colour;
			if (pending.AI->IsA<AEnemy>()) {
				AEnemy* enemy = Cast<AEnemy>(pending.AI);

				colour = enemy->Colour;
			}
			else
				colour = FLinearColor(Camera->Grid->Stream.FRandRange(0.0f, 1.0f), Camera->Grid->Stream.FRandRange(0.0f, 1.0f), Camera->Grid->Stream.FRandRange(0.0f, 1.0f));

			pending.HISM->PerInstanceSMCustomData[instance * pending.HISM->NumCustomDataFloats + 1] = colour.R;
			pending.HISM->PerInstanceSMCustomData[instance * pending.HISM->NumCustomDataFloats + 2] = colour.G;
			pending.HISM->PerInstanceSMCustomData[instance * pending.HISM->NumCustomDataFloats + 3] = colour.B;

			ActivateTorches(Camera->Grid->AtmosphereComponent->Calendar.Hour, pending.HISM, instance);

			pending.AI->AIController->DefaultAction();
		}
		else
			pending.HISM->RemoveInstance(pending.Instance);
	}
	
	if (!PendingChange.IsEmpty())
		PendingChange.Empty();

	CalculateCitizenMovement(Camera);
	UpdateHatsTransforms(Camera);

	CalculateAIMovement(Camera);

	CalculateBuildingDeath();

	CalculateBuildingRotation();
}

void UAIVisualiser::CalculateCitizenMovement(class ACamera* Camera)
{
	if (bCitizensMoving)
		return;

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&CitizenMovementLock);
		if (!lock.IsLocked())
			return;

		bCitizensMoving = true;

		TArray<ACitizen*> cs;
		TArray<ACitizen*> rebels;

		for (FFactionStruct faction : Camera->ConquestManager->Factions) {
			cs.Append(faction.Citizens);
			rebels.Append(faction.Rebels);
		}

		if (cs.IsEmpty() && rebels.IsEmpty()) {
			bCitizensMoving = false;

			return;
		}

		TArray<FVector> diseaseLocations;
		TArray<FVector> torchLocations;

		TArray<TArray<ACitizen*>> citizens;
		citizens.Add(cs);
		citizens.Add(rebels);

		for (int32 i = 0; i < citizens.Num(); i++) {
			for (int32 j = 0; j < citizens[i].Num(); j++) {
				ACitizen* citizen = citizens[i][j];

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() == 0)
					continue;

				citizen->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - citizen->MovementComponent->LastUpdatedTime);

				UHierarchicalInstancedStaticMeshComponent* hism = nullptr;

				if (i == 0) {
					float opacity = 1.0f;
					if (IsValid(citizen->Building.BuildingAt))
						opacity = 0.0f;

					HISMCitizen->PerInstanceSMCustomData[j * HISMCitizen->NumCustomDataFloats + 12] = opacity;

					hism = HISMCitizen;
				}
				else
					hism = HISMRebel;

				SetInstanceTransform(hism, j, citizen->MovementComponent->Transform);

				if (Camera->CitizenManager->Infected.Contains(citizen))
					diseaseLocations.Add(citizen->MovementComponent->Transform.GetLocation());

				if (TorchNiagaraComponent->IsActive())
					torchLocations.Add(citizen->MovementComponent->Transform.GetLocation());

				UpdateCitizenVisuals(hism, Camera, citizen, j);
			}

			if (i == 0 && !HISMCitizen->UnbuiltInstanceBoundsList.IsEmpty()) {
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMCitizen->BuildTreeIfOutdated(true, false); });
			}
			else if (!HISMRebel->UnbuiltInstanceBoundsList.IsEmpty())
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMRebel->BuildTreeIfOutdated(true, false); });
		}

		if (!torchLocations.IsEmpty()) {
			torchLocations.Append(UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(TorchNiagaraComponent, "Locations"));
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TorchNiagaraComponent, "Locations", torchLocations);
		}

		if (!diseaseLocations.IsEmpty()) {
			diseaseLocations.Append(UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(DiseaseNiagaraComponent, "Locations"));
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(DiseaseNiagaraComponent, "Locations", diseaseLocations);
		}

		bCitizensMoving = false;
	});
}

void UAIVisualiser::CalculateAIMovement(ACamera* Camera)
{
	if (bAIMoving)
		return;

	TArray<AAI*> clones;

	for (FFactionStruct faction : Camera->ConquestManager->Factions)
		clones.Append(faction.Clones);

	if (clones.IsEmpty() && Camera->CitizenManager->Enemies.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraph, [this, Camera, clones]() {
		FScopeTryLock lock(&AIMovementLock);
		if (!lock.IsLocked())
			return;

		bAIMoving = true;

		TArray<TArray<AAI*>> ais;
		ais.Add(clones);
		ais.Add(Camera->CitizenManager->Enemies);

		for (int32 i = 0; i < ais.Num(); i++) {
			for (int32 j = 0; j < ais[i].Num(); j++) {
				AAI* ai = ais[i][j];

				if (!IsValid(ai) || ai->HealthComponent->GetHealth() == 0)
					continue;

				ai->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - ai->MovementComponent->LastUpdatedTime);

				if (i == 0)
					SetInstanceTransform(HISMClone, j, ai->MovementComponent->Transform);
				else
					SetInstanceTransform(HISMEnemy, j, ai->MovementComponent->Transform);
			}

			if (i == 0 && !HISMClone->UnbuiltInstanceBoundsList.IsEmpty())
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMClone->BuildTreeIfOutdated(true, false); });
			else if (!HISMEnemy->UnbuiltInstanceBoundsList.IsEmpty())
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMEnemy->BuildTreeIfOutdated(true, false); });
		}

		bAIMoving = false;
	});
}

void UAIVisualiser::CalculateBuildingDeath()
{
	if (bBuildingsDying || DestructingBuildings.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&BuildingDeathLock);
		if (!lock.IsLocked())
			return;

		bBuildingsDying = true;

		for (int32 i = DestructingBuildings.Num() - 1; i > -1; i--) {
			ABuilding* building = DestructingBuildings[i];

			double alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - building->DeathTime) / 10.0f, 0.0f, 1.0f);
			float z = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 1.0f;

			building->BuildingMesh->SetCustomPrimitiveDataFloat(8, FMath::Lerp(0.0f, -z, alpha));

			if (alpha == 1.0f) {
				building->DestructionComponent->Deactivate();

				DestructingBuildings.RemoveAt(i);
			}
		}

		bBuildingsDying = false;
	});
}

void UAIVisualiser::CalculateBuildingRotation()
{
	if (bBuildingsRotating || RotatingBuildings.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&BuildingRotationLock);
		if (!lock.IsLocked())
			return;

		bBuildingsRotating = true;

		for (int32 i = RotatingBuildings.Num() - 1; i > -1; i--) {
			AResearch* building = RotatingBuildings[i];

			double alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - building->RotationTime) / 10.0f, 0.0f, 1.0f);
			float pitch = FMath::Lerp(building->PrevTelescopeTargetPitch, building->TelescopeTargetPitch, alpha);
			float yaw = FMath::Lerp(building->PrevTurretTargetYaw, building->TurretTargetYaw, alpha);

			building->TurretMesh->SetCustomPrimitiveDataFloat(10, yaw);

			building->TelescopeMesh->SetCustomPrimitiveDataFloat(9, pitch);
			building->TelescopeMesh->SetCustomPrimitiveDataFloat(10, yaw);

			if (alpha == 1.0f)
				RotatingBuildings.RemoveAt(i);
		}

		bBuildingsRotating = false;
	});
}

void UAIVisualiser::AddInstance(class AAI* AI, UHierarchicalInstancedStaticMeshComponent* HISM, FTransform Transform)
{
	AI->MovementComponent->Transform = Transform;

	FPendingChangeStruct pending;
	pending.AI = AI;
	pending.HISM = HISM;
	pending.Transform = Transform;

	PendingChange.Add(pending);
}

void UAIVisualiser::RemoveInstance(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	FPendingChangeStruct pending;
	pending.HISM = HISM;
	pending.Instance = Instance;

	PendingChange.Add(pending);
}

void UAIVisualiser::SetInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, FTransform Transform)
{
	FInstancedStaticMeshInstanceData& instanceData = HISM->PerInstanceSMData[Instance];

	if (instanceData.Transform == Transform.ToMatrixWithScale())
		return;

	instanceData.Transform = Transform.ToMatrixWithScale();

	Async(EAsyncExecution::TaskGraphMainTick, [this, HISM, Instance, Transform]() {
		FBodyInstance*& InstanceBodyInstance = HISM->InstanceBodies[Instance];
		InstanceBodyInstance->SetBodyTransform(Transform, TeleportFlagToEnum(false));
		InstanceBodyInstance->UpdateBodyScale(Transform.GetScale3D());

		HISM->UnbuiltInstanceBoundsList.Add(InstanceBodyInstance->GetBodyBounds());
	});
}

void UAIVisualiser::UpdateCitizenVisuals(UHierarchicalInstancedStaticMeshComponent* HISM, ACamera* Camera, ACitizen* Citizen, int32 Instance)
{
	if (Citizen->bGlasses && HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 10] == 0.0f)
		HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 10] = 1.0f;

	if (Camera->CitizenManager->Injured.Contains(Citizen) && HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 9] == 0.0f)
		HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 9] = 1.0f;
	else if (HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 9] == 1.0f)
		HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 9] = 0.0f;
}

void UAIVisualiser::ActivateTorches(int32 Hour, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	if (HISM == HISMClone || HISM == HISMEnemy)
		return;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	int32 value = 0.0f;

	if (settings->GetRenderTorches() && (Hour >= 18 || Hour < 6))
		value = 1.0f;

	if (Instance == INDEX_NONE) {
		for (int32 i = 0; i < HISMCitizen->GetInstanceCount(); i++)
			HISMCitizen->PerInstanceSMCustomData[i * HISMCitizen->NumCustomDataFloats + 11] = value;

		for (int32 i = 0; i < HISMRebel->GetInstanceCount(); i++)
			HISMRebel->PerInstanceSMCustomData[i * HISMRebel->NumCustomDataFloats + 11] = value;

		if (value == 1.0f && !TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Activate();
		else if (TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Deactivate();
	}
	else {
		HISM->PerInstanceSMCustomData[Instance * HISM->NumCustomDataFloats + 11] = value;
	}
}

void UAIVisualiser::AddHarvestVisual(FVector Location, FLinearColor Colour)
{
	TArray<FVector> locations = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(HarvestNiagaraComponent, "Locations");
	locations.Add(Location);

	TArray<FLinearColor> colours = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayColor(HarvestNiagaraComponent, "Colours");
	colours.Add(Colour);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(HarvestNiagaraComponent, "Locations", locations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(HarvestNiagaraComponent, "Colours", colours);
}

TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> UAIVisualiser::GetAIHISM(AAI* AI)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info;

	if (cm->Enemies.Contains(AI)) {
		info.Key = HISMEnemy;
		info.Value = cm->Enemies.Find(AI);
	}
	else {
		for (FFactionStruct faction : AI->Camera->ConquestManager->Factions) {
			if (faction.Citizens.Contains(AI)) {
				info.Key = HISMCitizen;
				info.Value = faction.Citizens.Find(Cast<ACitizen>(AI));
			}
			else if (faction.Rebels.Contains(AI)) {
				info.Key = HISMRebel;
				info.Value = faction.Rebels.Find(Cast<ACitizen>(AI));
			}
			else if (faction.Clones.Contains(AI)) {
				info.Key = HISMClone;
				info.Value = faction.Clones.Find(AI);
			}

			if (IsValid(info.Key))
				break;
		}
	}

	return info;
}

AAI* UAIVisualiser::GetHISMAI(ACamera* Camera, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	UCitizenManager* cm = Camera->CitizenManager;

	AAI* ai = nullptr;

	TArray<ACitizen*> citizens;
	TArray<ACitizen*> rebels;
	TArray<AAI*> clones;

	for (FFactionStruct faction : Camera->ConquestManager->Factions) {
		citizens.Append(faction.Citizens);
		rebels.Append(faction.Rebels);
		clones.Append(faction.Clones);
	}

	if (HISM == HISMCitizen)
		ai = citizens[Instance];
	else if (HISM == HISMRebel)
		ai = rebels[Instance];
	else if (HISM == HISMClone)
		ai = clones[Instance];
	else if (HISM == HISMEnemy)
		ai = cm->Enemies[Instance];

	return ai;
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;
	
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;

	FTransform transform;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Key->GetNumInstances() == 0)
		return transform;

	position.X = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 4];
	position.Y = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 5];
	position.Z = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 6];
	rotation.Pitch = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 7];

	transform.SetLocation(position);
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::SetAnimationPoint(AAI* AI, FTransform Transform)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 4] = Transform.GetLocation().X;
	info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 5] = Transform.GetLocation().Y;
	info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 6] = Transform.GetLocation().Z;
	info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 7] = Transform.GetRotation().Rotator().Pitch;
}

TArray<AActor*> UAIVisualiser::GetOverlaps(ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType)
{
	TArray<AActor*> actors;

	UCitizenManager* cm = Camera->CitizenManager;

	TArray<AActor*> actorsToCheck;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Actor);

	for (FFactionStruct& f : Camera->ConquestManager->Factions) {
		if (FactionType != EFactionType::Both && faction != nullptr && ((FactionType == EFactionType::Same && faction->Name != f.Name) || (FactionType == EFactionType::Different && faction->Name == f.Name)))
			continue;

		if (!RequestedOverlaps.IsGettingCitizenEnemies() || faction->AtWar.Contains(f.Name)) {
			if (RequestedOverlaps.bBuildings)
				actorsToCheck.Append(f.Buildings);

			if (RequestedOverlaps.bCitizens)
				actorsToCheck.Append(f.Citizens);

			if (RequestedOverlaps.bClones)
				actorsToCheck.Append(f.Clones);
		}

		if (RequestedOverlaps.bRebels)
			actorsToCheck.Append(f.Rebels);
	}

	if (RequestedOverlaps.bEnemies)
		actorsToCheck.Append(cm->Enemies);

	if (actorsToCheck.Contains(Actor))
		actorsToCheck.Remove(Actor);

	FVector location = Camera->GetTargetActorLocation(Actor);

	for (AActor* actor : actorsToCheck) {
		if (!IsValid(actor) || actor->IsPendingKillPending())
			continue;

		FVector loc = Camera->GetTargetActorLocation(actor);

		if (actor->IsA<ABuilding>())
			Cast<ABuilding>(actor)->BuildingMesh->GetClosestPointOnCollision(location, loc);

		float distance = FVector::Dist(location, loc);

		if (distance <= Range)
			actors.Add(actor);
	}

	if (RequestedOverlaps.bResources) {
		for (FResourceHISMStruct resourceStruct : Camera->Grid->TreeStruct) {
			TArray<int32> instances = resourceStruct.Resource->ResourceHISM->GetInstancesOverlappingSphere(location, Range);

			if (!instances.IsEmpty())
				actors.Add(resourceStruct.Resource);
		}
	}

	return actors;
}

//
// Hats
//
FTransform UAIVisualiser::GetHatTransform(ACitizen* Citizen)
{
	FTransform transform = Citizen->MovementComponent->Transform + GetAnimationPoint(Citizen);
	transform.SetLocation(transform.GetLocation() + FVector(0.0f, 0.0f, HISMCitizen->GetStaticMesh()->GetBounds().GetBox().GetSize().Z / 2.0f));

	return transform;
}

void UAIVisualiser::AddCitizenToHISMHat(ACitizen* Citizen, UStaticMesh* HatMesh)
{
	for (FHatsStruct& hat : HISMHats) {
		if (hat.HISMHat->GetStaticMesh() != HatMesh)
			continue;

		hat.Citizens.Add(Citizen);
		hat.HISMHat->AddInstance(GetHatTransform(Citizen));

		hatsNum++;

		break;
	}
}

void UAIVisualiser::RemoveCitizenFromHISMHat(ACitizen* Citizen)
{
	for (FHatsStruct& hat : HISMHats) {
		if (!hat.Citizens.Contains(Citizen))
			continue;

		hat.Citizens.Remove(Citizen);

		hatsNum--;

		break;
	}
}

void UAIVisualiser::UpdateHatsTransforms(ACamera* Camera)
{
	if (bHatsMoving || hatsNum == 0)
		return;

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&HatLock);
		if (!lock.IsLocked())
			return;

		bHatsMoving = false;

		for (FHatsStruct& hat : HISMHats) {
			if (hat.Citizens.IsEmpty())
				continue;

			for (int32 i = 0; i < hat.Citizens.Num(); i++) {
				FTransform transform = GetHatTransform(hat.Citizens[i]);

				FInstancedStaticMeshInstanceData& instanceData = hat.HISMHat->PerInstanceSMData[i];

				if (instanceData.Transform == transform.ToMatrixWithScale())
					return;

				instanceData.Transform = transform.ToMatrixWithScale();

				FBodyInstance*& InstanceBodyInstance = hat.HISMHat->InstanceBodies[i];
				hat.HISMHat->UnbuiltInstanceBoundsList.Add(InstanceBodyInstance->GetBodyBounds());
			}

			Async(EAsyncExecution::TaskGraphMainTick, [this, hat]() { hat.HISMHat->BuildTreeIfOutdated(false, false); });
		}

		bHatsMoving = true;
	});
}

bool UAIVisualiser::DoesCitizenHaveHat(ACitizen* Citizen)
{
	for (FHatsStruct& hat : HISMHats) {
		if (!hat.Citizens.Contains(Citizen))
			continue;

		return true;
	}

	return false;
}

void UAIVisualiser::ToggleOfficerLights(ACitizen* Citizen, float Value)
{
	for (FHatsStruct& hat : HISMHats) {
		if (!hat.Citizens.Contains(Citizen))
			continue;

		int32 instance = hat.Citizens.Find(Citizen);

		hat.HISMHat->PerInstanceSMCustomData[instance * hat.HISMHat->NumCustomDataFloats + 1] = Value;
	}
}
