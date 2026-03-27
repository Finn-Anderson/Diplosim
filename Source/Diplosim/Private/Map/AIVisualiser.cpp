#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Misc/ScopeTryLock.h"
#include "Camera/CameraComponent.h"

#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Research.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/DiplomacyComponent.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Components/SaveGameComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/AttackComponent.h"

UAIVisualiser::UAIVisualiser()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.SetAllChannels(ECR_Ignore);
	response.Visibility = ECR_Block;

	AIContainer = CreateDefaultSubobject<USceneComponent>(TEXT("AIContainer"));

	TMap<UInstancedStaticMeshComponent**, FName> hisms;
	hisms.Add(&HISMCitizen, TEXT("HISMCitizen"));
	hisms.Add(&HISMClone, TEXT("HISMClone"));
	hisms.Add(&HISMRebel, TEXT("HISMRebel"));
	hisms.Add(&HISMEnemy, TEXT("HISMEnemy"));
	hisms.Add(&HISMSnake, TEXT("HISMSnake"));

	for (auto& element : hisms) {
		auto hism = CreateDefaultSubobject<UInstancedStaticMeshComponent>(element.Value);
		*element.Key = hism;
		hism->SetupAttachment(AIContainer);
		hism->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hism->SetCollisionObjectType(ECC_Pawn);
		hism->SetCollisionResponseToChannels(response);
		hism->SetCanEverAffectNavigation(false);
		hism->SetGenerateOverlapEvents(false);
		hism->bSupportRemoveAtSwap = false;
		hism->bWorldPositionOffsetWritesVelocity = false;

		if (hism == HISMCitizen)
			hism->NumCustomDataFloats = 19;
		else if (hism == HISMRebel)
			hism->NumCustomDataFloats = 14;
		else
			hism->NumCustomDataFloats = 10;
	}

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetupAttachment(AIContainer);
	HarvestNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	HarvestNiagaraComponent->bAutoActivate = true;

	HatsContainer = CreateDefaultSubobject<USceneComponent>(TEXT("HatsContainer"));
	HatsContainer->SetupAttachment(AIContainer);

	HarvestVisuals.Add("Wood", FLinearColor(0.270498f, 0.158961f, 0.07036f));
	HarvestVisuals.Add("Stone", FLinearColor(0.571125f, 0.590619f, 0.64448f));
	HarvestVisuals.Add("Marble", FLinearColor(0.768151f, 0.73791f, 0.610496f));
	HarvestVisuals.Add("Iron", FLinearColor(0.291771f, 0.097587f, 0.066626f));
	HarvestVisuals.Add("Gold", FLinearColor(1.0f, 0.672443f, 0.0f));
}

void UAIVisualiser::BeginPlay()
{
	Super::BeginPlay();

	for (auto& element : HatsMeshesList) {
		FString name = element.Key->GetName() + "Component";

		FHatsStruct hatsStruct;
		hatsStruct.ISMHat = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *name);
		hatsStruct.ISMHat->SetStaticMesh(element.Key);
		hatsStruct.ISMHat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		hatsStruct.ISMHat->SetCanEverAffectNavigation(false);
		hatsStruct.ISMHat->SetGenerateOverlapEvents(false);
		hatsStruct.ISMHat->bWorldPositionOffsetWritesVelocity = false;
		hatsStruct.ISMHat->NumCustomDataFloats = element.Value;
		hatsStruct.ISMHat->RegisterComponent();

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

	for (int32 i = 0; i < HISMSnake->GetInstanceCount(); i++)
		RemoveInstance(HISMSnake, i);

	for (FHatsStruct hat : HISMHats)
		for (int32 i = 0; i < hat.ISMHat->GetInstanceCount(); i++)
			RemoveInstance(hat.ISMHat, i);

	DestructingActors.Empty();
}

void UAIVisualiser::MainLoop(ACamera* Camera)
{
	for (FPendingChangeStruct pending : PendingChange) {
		if (pending.Instances.IsEmpty()) {
			int32 instance = pending.ISM->AddInstance(pending.Transform, false);

			if (IsValid(pending.AI->SpawnSystem))
				UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), pending.AI->SpawnSystem, pending.Transform.GetLocation());

			if (pending.ISM == HISMClone)
				UpdateInstanceCustomData(pending.ISM, instance, 1, 3.0f);
		}
		else
			pending.ISM->RemoveInstances(pending.Instances);
	}

	if (!PendingChange.IsEmpty())
		PendingChange.Empty();

	CalculateCitizenMovement(Camera);

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

			if (faction.PartyInPower == "")
				continue;

			UMaterialInterface* material = HISMCitizen->GetMaterial(1);
			UMaterialInstanceDynamic* dynamicMaterial = nullptr;
			if (material->IsA<UMaterialInstanceDynamic>())
				dynamicMaterial = Cast<UMaterialInstanceDynamic>(material);
			else
				dynamicMaterial = UMaterialInstanceDynamic::Create(material, this); 
			
			UTexture2D* partyTexture = *Camera->ConquestManager->DiplomacyComponent->CultureTextureList.Find(faction.PartyInPower);
			UTexture2D* religionTexture = *Camera->ConquestManager->DiplomacyComponent->CultureTextureList.Find(faction.LargestReligion);

			Async(EAsyncExecution::TaskGraphMainTick, [dynamicMaterial, partyTexture, religionTexture]() {
				dynamicMaterial->SetTextureParameterValue("Party", partyTexture);
				dynamicMaterial->SetTextureParameterValue("Religion", religionTexture);
			});

			HISMCitizen->SetCustomPrimitiveDataFloat(0, faction.FlagColour.R);
			HISMCitizen->SetCustomPrimitiveDataFloat(1, faction.FlagColour.G);
			HISMCitizen->SetCustomPrimitiveDataFloat(2, faction.FlagColour.B);
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

						UInstancedStaticMeshComponent* ism = nullptr;
						if (i == 0) {
							float opacity = 1.0f;
							if (IsValid(citizen->BuildingComponent->BuildingAt) && citizen->BuildingComponent->BuildingAt->bHideCitizen)
								opacity = 0.0f;

							UpdateInstanceCustomData(HISMCitizen, j, 14, opacity);

							UpdateHatTransform(citizen);

							ism = HISMCitizen;
						}
						else
							ism = HISMRebel;

						SetInstanceTransform(ism, j, citizen->MovementComponent->Transform);

						UpdateCitizenVisuals(ism, Camera, citizen, j);

						SetAIColour(ism, j, citizen->Colour);
					}
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

		if (clones.IsEmpty() && gamemode->Enemies.IsEmpty() && gamemode->Snakes.IsEmpty())
			return;

		TArray<TArray<AAI*>> ais;
		ais.Add(clones);
		ais.Add(gamemode->Enemies);
		ais.Add(gamemode->Snakes);

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

				UInstancedStaticMeshComponent* ism = nullptr;
				if (i == 0)
					ism = HISMClone;
				else if (i == 1)
					ism = HISMEnemy;
				else
					ism = HISMSnake;

				SetInstanceTransform(ism, j, ai->MovementComponent->Transform);

				SetAIColour(ism, j, ai->Colour);
			}
		}
	});
}

void UAIVisualiser::CalculateBuildingDeath(ACamera* Camera)
{
	if (DestructingActors.IsEmpty()) {
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

		for (int32 i = DestructingActors.Num() - 1; i > -1; i--) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			TArray<AActor*> actors;
			DestructingActors.GenerateKeyArray(actors);

			AActor* actor = actors[i];
			double deathTime = *DestructingActors.Find(actor);

			UStaticMeshComponent* mesh = actor->GetComponentByClass<UStaticMeshComponent>();
			FVector dimensions = mesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

			double alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - deathTime) / 10.0f, 0.0f, 1.0f);
			float z = dimensions.Z + 1.0f;

			mesh->SetCustomPrimitiveDataFloat(8, FMath::Lerp(0.0f, -z, alpha));

			if (alpha == 1.0f) {
				DestructingActors.Remove(actor);

				if (Camera->AttachedTo.Actor == actor)
					Async(EAsyncExecution::TaskGraphMainTick, [Camera, actor]() { Camera->DisplayInteract(actor); });
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

void UAIVisualiser::AddInstance(class AAI* AI, UInstancedStaticMeshComponent* ISM, FTransform Transform)
{
	AI->MovementComponent->Transform = Transform;

	FPendingChangeStruct pending;
	pending.AI = AI;
	pending.ISM = ISM;
	pending.Transform = Transform;

	PendingChange.Add(pending);
}

void UAIVisualiser::RemoveInstance(UInstancedStaticMeshComponent* ISM, int32 Instance)
{
	FPendingChangeStruct pending;
	pending.ISM = ISM;
	pending.Instances = { Instance };

	int32 index = PendingChange.Find(pending);

	if (index == INDEX_NONE)
		PendingChange.Add(pending);
	else
		PendingChange[index].Instances.Add(Instance);
}

void UAIVisualiser::UpdateInstanceCustomData(UInstancedStaticMeshComponent* ISM, int32 Instance, int32 Index, float Value)
{
	int32 value = Instance * ISM->NumCustomDataFloats + Index;

	if (ISM->PerInstanceSMCustomData[value] == Value)
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [ISM, Instance, Index, Value]() { ISM->SetCustomDataValue(Instance, Index, Value); });
}

void UAIVisualiser::SetAIColour(UInstancedStaticMeshComponent* ISM, int32 Instance, FLinearColor Colour)
{
	UpdateInstanceCustomData(ISM, Instance, 2, Colour.R);
	UpdateInstanceCustomData(ISM, Instance, 3, Colour.G);
	UpdateInstanceCustomData(ISM, Instance, 4, Colour.B);
}

void UAIVisualiser::SetInstanceTransform(UInstancedStaticMeshComponent* ISM, int32 Instance, FTransform Transform)
{
	FInstancedStaticMeshInstanceData& instanceData = ISM->PerInstanceSMData[Instance];

	if (ISM == HISMCitizen) {
		int32 value = Instance * ISM->NumCustomDataFloats + 8;
		float pitch = ISM->PerInstanceSMCustomData[value];
		FRotator rotation = Transform.GetRotation().Rotator() + FRotator(pitch, 0.0f, 0.0f);
		Transform.SetRotation(rotation.Quaternion());
	}

	if (instanceData.Transform == Transform.ToMatrixWithScale())
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [ISM, Instance, Transform]() { ISM->UpdateInstanceTransform(Instance, Transform); });
}

void UAIVisualiser::UpdateCitizenVisuals(UInstancedStaticMeshComponent* ISM, ACamera* Camera, ACitizen* Citizen, int32 Instance)
{
	if (Camera->DiseaseManager->Injured.Contains(Citizen))
		UpdateInstanceCustomData(ISM, Instance, 10, 1.0f);
	else
		UpdateInstanceCustomData(ISM, Instance, 10, 0.0f);

	if (Citizen->bGlasses)
		UpdateInstanceCustomData(ISM, Instance, 11, 1.0f);

	ActivateTorch(Camera->Grid->AtmosphereComponent->Calendar.Hour, ISM, Instance);

	if (Camera->DiseaseManager->Infected.Contains(Citizen))
		UpdateInstanceCustomData(ISM, Instance, 13, 1.0f);
	else
		UpdateInstanceCustomData(ISM, Instance, 13, 0.0f);
}

void UAIVisualiser::ActivateTorch(int32 Hour, UInstancedStaticMeshComponent* ISM, int32 Instance)
{
	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	int32 value = 0.0f;

	if (settings->GetRenderTorches() && (Hour >= 18 || Hour < 6))
		value = 1.0f;

	UpdateInstanceCustomData(ISM, Instance, 12, value);
}

void UAIVisualiser::SetHarvestVisuals(ACitizen* Citizen, AResource* Resource)
{
	USoundBase* sound = nullptr;

	FHitResult hit(ForceInit);
	FCollisionQueryParams params;
	params.AddIgnoredActor(Citizen);

	FLinearColor colour = FLinearColor();
	for (FResourceStruct resourceStruct : Citizen->Camera->ResourceManager->ResourceList) {
		if (!Resource->IsA(resourceStruct.Type))
			continue;

		FString category = resourceStruct.Category;
		colour = *HarvestVisuals.Find(category);
	}

	if (Resource->IsA<AMineral>())
		sound = Citizen->Mines[Citizen->Camera->Stream.RandRange(0, Citizen->Mines.Num() - 1)];
	else
		sound = Citizen->Chops[Citizen->Camera->Stream.RandRange(0, Citizen->Chops.Num() - 1)];

	FVector location = AddHarvestVisual(Citizen, colour);

	Citizen->AmbientAudioComponent->SetRelativeLocation(location);
	Citizen->Camera->PlayAmbientSound(Citizen->AmbientAudioComponent, sound);
}

FVector UAIVisualiser::AddHarvestVisual(AAI* AI, FLinearColor Colour)
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

void UAIVisualiser::SetEyesVisuals(ACitizen* Citizen, int32 HappinessValue)
{
	auto element = GetAIHISM(Citizen);

	float val15 = 0.0f;
	float val16 = 0.0f;
	float val17 = 0.0f;

	if (!Citizen->AttackComponent->OverlappingEnemies.IsEmpty())
		val16 = 1.0f;
	else if (Citizen->Camera->DiseaseManager->Infected.Contains(Citizen) || Citizen->Camera->DiseaseManager->Injured.Contains(Citizen) || HappinessValue < 35)
		val17 = 1.0f;
	else if (HappinessValue > 65)
		val15 = 1.0f;

	UpdateInstanceCustomData(element.Key, element.Value, 15, val15);
	UpdateInstanceCustomData(element.Key, element.Value, 16, val16);
	UpdateInstanceCustomData(element.Key, element.Value, 17, val17);
}

TTuple<UInstancedStaticMeshComponent*, int32> UAIVisualiser::GetAIHISM(AAI* AI)
{
	TTuple<UInstancedStaticMeshComponent*, int32> info = TTuple<UInstancedStaticMeshComponent*, int32>(nullptr, INDEX_NONE);

	if (!IsValid(AI))
		return info;

	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	if (gamemode->Enemies.Contains(AI)) {
		info.Key = HISMEnemy;
		info.Value = gamemode->Enemies.Find(AI);
	}
	else if (gamemode->Snakes.Contains(AI)) {
		info.Key = HISMSnake;
		info.Value = gamemode->Snakes.Find(AI);
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

AAI* UAIVisualiser::GetHISMAI(ACamera* Camera, UInstancedStaticMeshComponent* ISM, int32 Instance)
{
	AAI* ai = nullptr;
	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	TArray<ACitizen*> citizens;
	TArray<ACitizen*> rebels;
	TArray<AAI*> clones;

	for (FFactionStruct faction : Camera->ConquestManager->Factions) {
		citizens.Append(faction.Citizens);
		rebels.Append(faction.Rebels);
		clones.Append(faction.Clones);
	}

	if (ISM == HISMCitizen)
		ai = citizens[Instance];
	else if (ISM == HISMRebel)
		ai = rebels[Instance];
	else if (ISM == HISMClone)
		ai = clones[Instance];
	else if (ISM == HISMEnemy)
		ai = gamemode->Enemies[Instance];
	else if (ISM == HISMSnake)
		ai = gamemode->Snakes[Instance];

	return ai;
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;

	FTransform transform = FTransform();

	TTuple<UInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Value == INDEX_NONE || info.Value >= info.Key->GetNumInstances())
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
	TTuple<UInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Value == -1)
		return;

	UpdateInstanceCustomData(info.Key, info.Value, 5, Transform.GetLocation().X);
	UpdateInstanceCustomData(info.Key, info.Value, 6, Transform.GetLocation().Y);
	UpdateInstanceCustomData(info.Key, info.Value, 7, Transform.GetLocation().Z);
	UpdateInstanceCustomData(info.Key, info.Value, 8, Transform.GetRotation().Rotator().Pitch);
}

TArray<AActor*> UAIVisualiser::GetOverlaps(ACamera* Camera, AActor* Actor, float Range, FOverlapsStruct RequestedOverlaps, EFactionType FactionType, FFactionStruct* Faction, FVector Location)
{
	TArray<AActor*> actors;

	TArray<AActor*> actorsToCheck;

	FString factionName = "";
	if (Actor->IsA<ABuilding>())
		factionName = Cast<ABuilding>(Actor)->FactionName;

	if (Faction == nullptr)
		Faction = Camera->ConquestManager->GetFaction(factionName, Actor);

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

	if (RequestedOverlaps.bEnemies) {
		ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

		actorsToCheck.Append(gamemode->Enemies);
		actorsToCheck.Append(gamemode->Snakes);

		if (RequestedOverlaps.bBuildings)
			actorsToCheck.Append(gamemode->SnakeSpawners);
	}

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
	FTransform animTransform = GetAnimationPoint(Citizen);
	FTransform transform = Citizen->MovementComponent->Transform;

	FVector location = transform.GetLocation() + animTransform.GetLocation();
	transform.SetLocation(location);

	FRotator rotation = transform.GetRotation().Rotator() + animTransform.GetRotation().Rotator();
	rotation.Normalize();
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::UpdateHatTransform(ACitizen* Citizen)
{
	FHatsStruct* hatStruct = GetCitizenHat(Citizen);

	if (hatStruct != nullptr) {
		int32 index = hatStruct->Citizens.Find(Citizen);

		FTransform transform = GetHatTransform(Citizen);
		SetInstanceTransform(hatStruct->ISMHat, index, transform);

		float opacity = 1.0f;
		if (IsValid(Citizen->BuildingComponent->BuildingAt))
			opacity = 0.0f;

		UpdateInstanceCustomData(hatStruct->ISMHat, index, 1, opacity);
	}
}

void UAIVisualiser::AddCitizenToHISMHat(ACitizen* Citizen, UStaticMesh* HatMesh)
{
	for (FHatsStruct& hat : HISMHats) {
		if (hat.ISMHat->GetStaticMesh() != HatMesh)
			continue;

		hat.Citizens.Add(Citizen);
		hat.ISMHat->AddInstance(GetHatTransform(Citizen));

		break;
	}
}

void UAIVisualiser::RemoveCitizenFromHISMHat(ACitizen* Citizen)
{
	for (FHatsStruct& hat : HISMHats) {
		int32 index = hat.Citizens.Find(Citizen);

		if (index == INDEX_NONE)
			continue;

		hat.Citizens.RemoveAt(index);
		hat.ISMHat->RemoveInstance(index);

		break;
	}
}

FHatsStruct* UAIVisualiser::GetCitizenHat(ACitizen* Citizen)
{
	FHatsStruct* hatStruct = nullptr;

	for (FHatsStruct& hat : HISMHats) {
		if (!hat.Citizens.Contains(Citizen))
			continue;

		hatStruct = &hat;

		break;
	}

	return hatStruct;
}

bool UAIVisualiser::DoesCitizenHaveHat(ACitizen* Citizen)
{
	return GetCitizenHat(Citizen) != nullptr;
}

void UAIVisualiser::ToggleOfficerLights(ACitizen* Citizen, float Value)
{
	for (FHatsStruct& hat : HISMHats) {
		if (!hat.Citizens.Contains(Citizen))
			continue;

		int32 instance = hat.Citizens.Find(Citizen);

		hat.ISMHat->PerInstanceSMCustomData[instance * hat.ISMHat->NumCustomDataFloats + 1] = Value;
	}
}