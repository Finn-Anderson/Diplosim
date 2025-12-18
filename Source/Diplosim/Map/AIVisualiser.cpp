#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Research.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/SaveGameComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UAIVisualiser::UAIVisualiser()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.SetAllChannels(ECR_Ignore);

	AIContainer = CreateDefaultSubobject<USceneComponent>(TEXT("AIContainer"));

	HISMCitizen = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMCitizen"));
	HISMCitizen->SetupAttachment(AIContainer);
	HISMCitizen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISMCitizen->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMCitizen->SetCollisionResponseToChannels(response);
	HISMCitizen->SetCanEverAffectNavigation(false);
	HISMCitizen->SetGenerateOverlapEvents(false);
	HISMCitizen->bWorldPositionOffsetWritesVelocity = false;
	HISMCitizen->bAutoRebuildTreeOnInstanceChanges = false;
	HISMCitizen->NumCustomDataFloats = 18;

	HISMClone = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClone"));
	HISMClone->SetupAttachment(AIContainer);
	HISMClone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISMClone->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMClone->SetCollisionResponseToChannels(response);
	HISMClone->SetCanEverAffectNavigation(false);
	HISMClone->SetGenerateOverlapEvents(false);
	HISMClone->bWorldPositionOffsetWritesVelocity = false;
	HISMClone->bAutoRebuildTreeOnInstanceChanges = false;
	HISMClone->NumCustomDataFloats = 10;

	HISMRebel = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRebel"));
	HISMRebel->SetupAttachment(AIContainer);
	HISMRebel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISMRebel->SetCollisionObjectType(ECC_GameTraceChannel3);
	HISMRebel->SetCollisionResponseToChannels(response);
	HISMRebel->SetCanEverAffectNavigation(false);
	HISMRebel->SetGenerateOverlapEvents(false);
	HISMRebel->bWorldPositionOffsetWritesVelocity = false;
	HISMRebel->bAutoRebuildTreeOnInstanceChanges = false;
	HISMRebel->NumCustomDataFloats = 14;

	HISMEnemy = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMEnemy"));
	HISMEnemy->SetupAttachment(AIContainer);
	HISMEnemy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HISMEnemy->SetCollisionObjectType(ECC_GameTraceChannel4);
	HISMEnemy->SetCollisionResponseToChannels(response);
	HISMEnemy->SetCanEverAffectNavigation(false);
	HISMEnemy->SetGenerateOverlapEvents(false);
	HISMEnemy->bWorldPositionOffsetWritesVelocity = false;
	HISMEnemy->bAutoRebuildTreeOnInstanceChanges = false;
	HISMEnemy->NumCustomDataFloats = 9;

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetupAttachment(AIContainer);
	HarvestNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	HarvestNiagaraComponent->bAutoActivate = true;

	HatsContainer = CreateDefaultSubobject<USceneComponent>(TEXT("HatsContainer"));
	HatsContainer->SetupAttachment(AIContainer);

	hatsNum = 0;
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

void UAIVisualiser::ResetToDefaultValues()
{
	PendingChange.Empty();

	for (int32 i = 0; i < HISMCitizen->GetInstanceCount(); i++)
		RemoveInstance(HISMCitizen, i);

	for (int32 i = 0; i < HISMRebel->GetInstanceCount(); i++)
		RemoveInstance(HISMRebel, i);

	for (int32 i = 0; i < HISMClone->GetInstanceCount(); i++)
		RemoveInstance(HISMClone, i);

	for (int32 i = 0; i < HISMEnemy->GetInstanceCount(); i++)
		RemoveInstance(HISMEnemy, i);

	for (FHatsStruct hat : HISMHats)
		for (int32 i = 0; i < hat.HISMHat->GetInstanceCount(); i++)
			RemoveInstance(hat.HISMHat, i);
}

void UAIVisualiser::MainLoop(ACamera* Camera)
{
	for (FPendingChangeStruct pending : PendingChange) {
		if (pending.Instance == -1) {
			int32 instance = pending.HISM->AddInstance(pending.Transform, false);

			if (IsValid(pending.AI->SpawnSystem))
				UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), pending.AI->SpawnSystem, pending.Transform.GetLocation());

			if (pending.HISM == HISMClone)
				UpdateInstanceCustomData(pending.HISM, instance, 1, 3.0f);

			UpdateInstanceCustomData(pending.HISM, instance, 2, pending.AI->Colour.R);
			UpdateInstanceCustomData(pending.HISM, instance, 3, pending.AI->Colour.G);
			UpdateInstanceCustomData(pending.HISM, instance, 4, pending.AI->Colour.B);
		}
		else
			pending.HISM->RemoveInstance(pending.Instance);
	}

	if (!PendingChange.IsEmpty())
		PendingChange.Empty();

	CalculateCitizenMovement(Camera);
	UpdateHatsTransforms(Camera);

	CalculateAIMovement(Camera);

	CalculateBuildingDeath(Camera);

	CalculateBuildingRotation(Camera);
}

void UAIVisualiser::CalculateCitizenMovement(class ACamera* Camera)
{
	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&CitizenMovementLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::CitizenMovement);

			return;
		}

		TArray<ACitizen*> cs;
		TArray<ACitizen*> rebels;

		for (FFactionStruct faction : Camera->ConquestManager->Factions) {
			cs.Append(faction.Citizens);
			rebels.Append(faction.Rebels);
		}

		if (cs.IsEmpty() && rebels.IsEmpty())
			return;

		TArray<TArray<ACitizen*>> citizens;
		citizens.Add(cs);
		citizens.Add(rebels);

		for (int32 k = 0; k < 2; k++) {
			Async(EAsyncExecution::TaskGraph, [this, Camera, citizens, k]() {
				if (k == 0) {
					FScopeTryLock citizenLock(&CitizenMovementLock1);
					if (!citizenLock.IsLocked())
						return;
				}
				else {
					FScopeTryLock citizenLock(&CitizenMovementLock2);
					if (!citizenLock.IsLocked())
						return;
				}

				for (int32 i = 0; i < citizens.Num(); i++) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					for (int32 j = (citizens[i].Num() * k) / 2; j < citizens[i].Num() / (2 - k); j++) {
						if (Camera->SaveGameComponent->IsLoading())
							return;

						ACitizen* citizen = citizens[i][j];

						if (!IsValid(citizen))
							continue;

						citizen->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - citizen->MovementComponent->LastUpdatedTime);

						if (citizen->HealthComponent->GetHealth() == 0)
							continue;

						UHierarchicalInstancedStaticMeshComponent* hism = nullptr;

						if (i == 0) {
							float opacity = 1.0f;
							if (IsValid(citizen->Building.BuildingAt))
								opacity = 0.0f;

							UpdateInstanceCustomData(HISMCitizen, j, 14, opacity);

							hism = HISMCitizen;
						}
						else
							hism = HISMRebel;

						SetInstanceTransform(hism, j, citizen->MovementComponent->Transform);

						UpdateCitizenVisuals(hism, Camera, citizen, j);

						UpdateArmyVisuals(Camera, citizen);
					}

					if (i == 0 && HISMCitizen->bIsOutOfDate)
						Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMCitizen->BuildTreeIfOutdated(false, false); });
					else if (HISMRebel->bIsOutOfDate)
						Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMRebel->BuildTreeIfOutdated(false, false); });
				}
			});
		}
	});
}

void UAIVisualiser::CalculateAIMovement(ACamera* Camera)
{
	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&AIMovementLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::AIMovement);

			return;
		}

		ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
		TArray<AAI*> clones;

		for (FFactionStruct faction : Camera->ConquestManager->Factions)
			clones.Append(faction.Clones);

		if (clones.IsEmpty() && gamemode->Enemies.IsEmpty())
			return;

		TArray<TArray<AAI*>> ais;
		ais.Add(clones);
		ais.Add(gamemode->Enemies);

		for (int32 i = 0; i < ais.Num(); i++) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			for (int32 j = 0; j < ais[i].Num(); j++) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				AAI* ai = ais[i][j];

				if (!IsValid(ai))
					continue;

				ai->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - ai->MovementComponent->LastUpdatedTime);

				if (ai->HealthComponent->GetHealth() == 0)
					continue;

				if (i == 0)
					SetInstanceTransform(HISMClone, j, ai->MovementComponent->Transform);
				else
					SetInstanceTransform(HISMEnemy, j, ai->MovementComponent->Transform);
			}

			if (i == 0 && HISMClone->bIsOutOfDate)
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMClone->BuildTreeIfOutdated(false, false); });
			else if (HISMEnemy->bIsOutOfDate)
				Async(EAsyncExecution::TaskGraphMainTick, [this]() { HISMEnemy->BuildTreeIfOutdated(false, false); });
		}
	});
}

void UAIVisualiser::CalculateBuildingDeath(ACamera* Camera)
{
	if (DestructingBuildings.IsEmpty()) {
		if (Camera->SaveGameComponent->IsLoading())
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::BuildingDeath);

		return;
	}

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&BuildingDeathLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::BuildingDeath);

			return;
		}

		for (int32 i = DestructingBuildings.Num() - 1; i > -1; i--) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			ABuilding* building = DestructingBuildings[i];

			double alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - building->DeathTime) / 10.0f, 0.0f, 1.0f);
			float z = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z + 1.0f;

			building->BuildingMesh->SetCustomPrimitiveDataFloat(8, FMath::Lerp(0.0f, -z, alpha));

			if (alpha == 1.0f) {
				building->DestructionComponent->Deactivate();

				DestructingBuildings.RemoveAt(i);
			}
		}
	});
}

void UAIVisualiser::CalculateBuildingRotation(ACamera* Camera)
{
	if (RotatingBuildings.IsEmpty()) {
		if (Camera->SaveGameComponent->IsLoading())
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::BuildingRotation);

		return;
	}

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&BuildingRotationLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::BuildingRotation);

			return;
		}

		for (int32 i = RotatingBuildings.Num() - 1; i > -1; i--) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

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

void UAIVisualiser::UpdateInstanceCustomData(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, int32 Index, float Value)
{
	int32 value = Instance * HISM->NumCustomDataFloats + Index;

	if (HISM->PerInstanceSMCustomData[value] == Value)
		return;

	HISM->PerInstanceSMCustomData[value] = Value;

	HISM->bIsOutOfDate = true;
}

void UAIVisualiser::SetInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance, FTransform Transform)
{
	FInstancedStaticMeshInstanceData& instanceData = HISM->PerInstanceSMData[Instance];

	if (instanceData.Transform == Transform.ToMatrixWithScale())
		return;

	instanceData.Transform = Transform.ToMatrixWithScale();

	HISM->bIsOutOfDate = true;
}

void UAIVisualiser::UpdateCitizenVisuals(UHierarchicalInstancedStaticMeshComponent* HISM, ACamera* Camera, ACitizen* Citizen, int32 Instance)
{
	if (Camera->DiseaseManager->Injured.Contains(Citizen))
		UpdateInstanceCustomData(HISM, Instance, 10, 1.0f);
	else
		UpdateInstanceCustomData(HISM, Instance, 10, 0.0f);

	if (Citizen->bGlasses)
		UpdateInstanceCustomData(HISM, Instance, 11, 1.0f);

	ActivateTorch(Camera->Grid->AtmosphereComponent->Calendar.Hour, HISM, Instance);

	if (Camera->DiseaseManager->Infected.Contains(Citizen))
		UpdateInstanceCustomData(HISM, Instance, 13, 1.0f);
	else
		UpdateInstanceCustomData(HISM, Instance, 13, 0.0f);
}

void UAIVisualiser::ActivateTorch(int32 Hour, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	int32 value = 0.0f;

	if (settings->GetRenderTorches() && (Hour >= 18 || Hour < 6))
		value = 1.0f;

	UpdateInstanceCustomData(HISM, Instance, 11, value);
}

void UAIVisualiser::UpdateArmyVisuals(ACamera* Camera, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FArmyStruct& army : faction->Armies) {
		if (army.Citizens.IsEmpty() || army.Citizens[0] != Citizen)
			continue;

		army.WidgetComponent->SetRelativeLocation(Camera->GetTargetActorLocation(Citizen) + FVector(0.0f, 0.0f, 300.0f));

		break;
	}
}

FVector UAIVisualiser::AddHarvestVisual(class AAI* AI, FLinearColor Colour)
{
	FVector location = AI->MovementComponent->Transform.GetLocation() + FVector(20.0f, 0.0f, 17.0f);

	TArray<FVector> locations = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(HarvestNiagaraComponent, "Locations");
	locations.Add(location);

	TArray<FLinearColor> colours = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayColor(HarvestNiagaraComponent, "Colours");
	colours.Add(Colour);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(HarvestNiagaraComponent, "Locations", locations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(HarvestNiagaraComponent, "Colours", colours);

	return location;
}

TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> UAIVisualiser::GetAIHISM(AAI* AI)
{
	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info;

	if (!IsValid(AI))
		return info;

	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	if (gamemode->Enemies.Contains(AI)) {
		info.Key = HISMEnemy;
		info.Value = gamemode->Enemies.Find(AI);
	}
	else {
		FFactionStruct* faction = AI->Camera->ConquestManager->GetFaction("", AI);

		if (faction == nullptr)
			return info;

		if (faction->Citizens.Contains(AI)) {
			info.Key = HISMCitizen;
			info.Value = faction->Citizens.Find(Cast<ACitizen>(AI));
		}
		else if (faction->Rebels.Contains(AI)) {
			info.Key = HISMRebel;
			info.Value = faction->Rebels.Find(Cast<ACitizen>(AI));
		}
		else if (faction->Clones.Contains(AI)) {
			info.Key = HISMClone;
			info.Value = faction->Clones.Find(AI);
		}
	}

	return info;
}

AAI* UAIVisualiser::GetHISMAI(ACamera* Camera, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
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
		ai = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies[Instance];

	return ai;
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;

	FTransform transform;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Key->GetNumInstances() == 0)
		return transform;

	position.X = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 5];
	position.Y = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 6];
	position.Z = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 7];
	rotation.Pitch = info.Key->PerInstanceSMCustomData[info.Value * info.Key->NumCustomDataFloats + 8];

	transform.SetLocation(position);
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::SetAnimationPoint(AAI* AI, FTransform Transform)
{
	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Value == -1)
		return;

	UpdateInstanceCustomData(info.Key, info.Value, 5, Transform.GetLocation().X);
	UpdateInstanceCustomData(info.Key, info.Value, 6, Transform.GetLocation().Y);
	UpdateInstanceCustomData(info.Key, info.Value, 7, Transform.GetLocation().Z);
	UpdateInstanceCustomData(info.Key, info.Value, 8, Transform.GetRotation().Rotator().Pitch / 360.0f);
}

TArray<AActor*> UAIVisualiser::GetOverlaps(ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType, FFactionStruct* Faction, FVector Location)
{
	TArray<AActor*> actors;

	TArray<AActor*> actorsToCheck;

	if (Faction == nullptr)
		Faction = Camera->ConquestManager->GetFaction("", Actor);

	if (FactionType != EFactionType::Same) {
		for (FFactionStruct& f : Camera->ConquestManager->Factions) {
			if (FactionType != EFactionType::Both && Faction != nullptr && Faction->Name == f.Name)
				continue;

			if (!RequestedOverlaps.IsGettingCitizenEnemies() || Faction->AtWar.Contains(f.Name)) {
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
	}
	else {
		if (RequestedOverlaps.bBuildings)
			actorsToCheck.Append(Faction->Buildings);

		if (RequestedOverlaps.bCitizens)
			actorsToCheck.Append(Faction->Citizens);

		if (RequestedOverlaps.bClones)
			actorsToCheck.Append(Faction->Clones);

		if (RequestedOverlaps.bRebels)
			actorsToCheck.Append(Faction->Rebels);
	}

	if (RequestedOverlaps.bEnemies)
		actorsToCheck.Append(GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies);

	if (actorsToCheck.Contains(Actor))
		actorsToCheck.Remove(Actor);

	if (Location == FVector::Zero())
		Location = Camera->GetTargetActorLocation(Actor);

	for (AActor* actor : actorsToCheck) {
		if (!IsValid(actor))
			continue;

		UHealthComponent* healthComp = actor->FindComponentByClass<UHealthComponent>();

		if (IsValid(healthComp) && healthComp->GetHealth() == 0)
			continue;

		FVector loc = Camera->GetTargetActorLocation(actor);

		if (actor->IsA<ABuilding>())
			Cast<ABuilding>(actor)->BuildingMesh->GetClosestPointOnCollision(Location, loc);

		float distance = FVector::Dist(Location, loc);

		if (distance <= Range)
			actors.Add(actor);
	}

	if (RequestedOverlaps.bResources) {
		for (FResourceHISMStruct resourceStruct : Camera->Grid->TreeStruct) {
			TArray<int32> instances = resourceStruct.Resource->ResourceHISM->GetInstancesOverlappingSphere(Location, Range);

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
	if (hatsNum == 0) {
		if (Camera->SaveGameComponent->IsLoading())
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Hats);

		return;
	}

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&HatLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Hats);

			return;
		}

		for (FHatsStruct& hat : HISMHats) {
			if (hat.Citizens.IsEmpty())
				continue;

			for (int32 i = 0; i < hat.Citizens.Num(); i++) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

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