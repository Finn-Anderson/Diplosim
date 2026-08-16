#include "Map/AIVisualiser.h"

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
#include "AI/Citizen/Components/HappinessComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Service/Research.h"
#include "Map/Grid.h"
#include "Map/AIInstancedStaticMeshComponent.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Resources/Vegetation.h"
#include "Map/Resources/Mineral.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Components/DiplomacyComponent.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Managers/AudioManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Player/Managers/EventsManager.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/AttackComponent.h"

UAIVisualiser::UAIVisualiser()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.SetAllChannels(ECR_Ignore);
	response.Visibility = ECR_Block;
	response.Pawn = ECR_Block;

	AIContainer = CreateDefaultSubobject<USceneComponent>(TEXT("AIContainer"));

	TMap<UAIInstancedStaticMeshComponent**, FName> hisms;
	hisms.Add(&HISMCitizen, TEXT("HISMCitizen"));
	hisms.Add(&HISMClone, TEXT("HISMClone"));
	hisms.Add(&HISMRebel, TEXT("HISMRebel"));
	hisms.Add(&HISMEnemy, TEXT("HISMEnemy"));
	hisms.Add(&HISMSnake, TEXT("HISMSnake"));
	hisms.Add(&HISMBird, TEXT("HISMBird"));

	for (auto& element : hisms) {
		auto hism = CreateDefaultSubobject<UAIInstancedStaticMeshComponent>(element.Value);
		*element.Key = hism;
		hism->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hism->SetCollisionObjectType(ECC_Pawn);
		hism->SetCollisionResponseToChannels(response);
		hism->SetCanEverAffectNavigation(false);
		hism->SetGenerateOverlapEvents(false);
		hism->bSupportRemoveAtSwap = false;
		hism->bWorldPositionOffsetWritesVelocity = false;

		if (hism == HISMCitizen)
			hism->NumCustomDataFloats = 20;
		else if (hism == HISMRebel)
			hism->NumCustomDataFloats = 14;
		else
			hism->NumCustomDataFloats = 10;
	}

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetAutoActivate(false);

	HarvestVisualCooldownTimer = 0.0f;
	MaxCounter = 10;
	Counter = MaxCounter;

	HatsContainer = CreateDefaultSubobject<USceneComponent>(TEXT("HatsContainer"));

	HarvestVisuals.Add("Wood", FLinearColor(0.270498f, 0.158961f, 0.07036f));
	HarvestVisuals.Add("Stone", FLinearColor(0.571125f, 0.590619f, 0.64448f));
	HarvestVisuals.Add("Marble", FLinearColor(0.768151f, 0.73791f, 0.610496f));
	HarvestVisuals.Add("Iron", FLinearColor(0.291771f, 0.097587f, 0.066626f));
	HarvestVisuals.Add("Gold", FLinearColor(1.0f, 0.672443f, 0.0f));
}

void UAIVisualiser::SetupAttachment(USceneComponent* RootComponent)
{
	AIContainer->SetupAttachment(RootComponent);

	HISMCitizen->SetupAttachment(AIContainer);
	HISMClone->SetupAttachment(AIContainer);
	HISMRebel->SetupAttachment(AIContainer);
	HISMEnemy->SetupAttachment(AIContainer);
	HISMSnake->SetupAttachment(AIContainer);
	HISMBird->SetupAttachment(AIContainer);
	HarvestNiagaraComponent->SetupAttachment(AIContainer);
	HatsContainer->SetupAttachment(AIContainer);
}

void UAIVisualiser::BeginPlay()
{
	Super::BeginPlay();

	for (auto& element : HatsMeshesList) {
		FString name = element.Key->GetName() + "Hat";

		FHatsStruct hatsStruct;
		hatsStruct.ISMHat = NewObject<UAIInstancedStaticMeshComponent>(this, UAIInstancedStaticMeshComponent::StaticClass(), *name);
		hatsStruct.ISMHat->SetupAttachment(HatsContainer);
		hatsStruct.ISMHat->SetStaticMesh(element.Key);
		hatsStruct.ISMHat->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hatsStruct.ISMHat->SetCollisionResponseToAllChannels(ECR_Ignore);
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
	DestructingActors.Empty();
}

void UAIVisualiser::MainLoop(ACamera* Camera, float DeltaTime)
{
	HarvestVisualCooldownTimer = FMath::Max(HarvestVisualCooldownTimer - DeltaTime, 0.0f);

	CalculateAIMovement(Camera);

	CalculateBuildingDeath(Camera);

	CalculateBuildingRotation(Camera);
}

void UAIVisualiser::RemoveInstances(class UAIInstancedStaticMeshComponent* ISM, TArray<AAI*>& AIList, int32 StartInstance, bool bRemove, TArray<int32> InstancesToDelete)
{
	for (int32 i = AIList.Num() - 1; i > -1; i--) {
		if (!AIList[i]->HealthComponent->bDead)
			continue;

		AIList.RemoveAt(i);
		InstancesToDelete.Add(StartInstance + i);
	}

	if (bRemove)
		ISM->RemoveInstances(InstancesToDelete);
}

void UAIVisualiser::RemoveInstances(class UAIInstancedStaticMeshComponent* ISM, TArray<ACitizen*>& CitizensList, int32 StartInstance, bool bRemove, TArray<int32> InstancesToDelete)
{
	for (int32 i = CitizensList.Num() - 1; i > -1; i--) {
		if (!CitizensList[i]->HealthComponent->bDead)
			continue;

		CitizensList.RemoveAt(i);
		InstancesToDelete.Add(StartInstance + i);
	}

	if (bRemove)
		ISM->RemoveInstances(InstancesToDelete);
}

void UAIVisualiser::AddInstances(ACamera* Camera, UAIInstancedStaticMeshComponent* ISM, TArray<AAI*> AIList, bool bHat)
{
	if (AIList.Num() <= ISM->GetInstanceCount())
		return;

	bool bCrystalVisible = Camera->Grid->CrystalMesh->GetCustomPrimitiveData().Data[0] > 0.0f;

	TArray<FTransform> transforms;
	for (int32 i = ISM->GetInstanceCount(); i < AIList.Num(); i++) {
		AAI* ai = AIList[i];

		FTransform transform = ai->MovementComponent->Transform;
		if (bHat)
			transform = GetHatTransform(Cast<ACitizen>(ai));

		if (ai->SpawnSystem && bCrystalVisible)
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ai->SpawnSystem, transform.GetLocation());

		transforms.Add(transform);
	}

	ISM->AddInstances(transforms, false, false, false);
}

void UAIVisualiser::CalculateAIMovement(ACamera* Camera)
{
	if (Counter != MaxCounter)
		return;

	FScopeTryLock lock(&AIMovementLock);
	if (!lock.IsLocked())
		return; 

	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	if (HarvestVisualCooldownTimer == 0.0f && HarvestNiagaraComponent->IsActive())
		HarvestNiagaraComponent->Deactivate();

	TArray<AAI*> AIList;

	TArray<AAI*> cs;
	TArray<int32> citizenInstancesToDelete;

	TArray<AAI*> rebels;
	TArray<int32> rebelInstancesToDelete;

	TArray<AAI*> clones;
	TArray<int32> cloneInstancesToDelte;

	int32 count = 0;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		count++;

		AIList.Empty();
		for (ACitizen* citizen : faction.Citizens)
			AIList.Add(citizen);
		cs.Append(AIList);

		AIList.Empty();
		for (ACitizen* rebel : faction.Rebels)
			AIList.Add(rebel);
		rebels.Append(AIList);

		clones.Append(faction.Clones);

		RemoveInstances(HISMCitizen, faction.Citizens, cs.Num(), count == Camera->ConquestManager->Factions.Num(), citizenInstancesToDelete);
		RemoveInstances(HISMRebel, faction.Rebels, rebels.Num(), count == Camera->ConquestManager->Factions.Num(), rebelInstancesToDelete);
		RemoveInstances(HISMClone, faction.Clones, clones.Num(), count == Camera->ConquestManager->Factions.Num(), cloneInstancesToDelte);
	}
	RemoveInstances(HISMEnemy, gamemode->Enemies);
	RemoveInstances(HISMSnake, gamemode->Snakes);
	RemoveInstances(HISMBird, Camera->Grid->Birds);

	AddInstances(Camera, HISMCitizen, cs);
	for (FHatsStruct& hat : HISMHats) {
		AIList.Empty();
		for (ACitizen* citizen : hat.Citizens)
			AIList.Add(citizen);
		AddInstances(Camera, hat.ISMHat, AIList, true);
	}
	AddInstances(Camera, HISMRebel, rebels);
	AddInstances(Camera, HISMClone, clones);
	AddInstances(Camera, HISMEnemy, gamemode->Enemies);
	AddInstances(Camera, HISMSnake, gamemode->Snakes);
	AddInstances(Camera, HISMBird, Camera->Grid->Birds);

	Async(EAsyncExecution::TaskGraph, [this, Camera, cs, rebels, clones, gamemode]() {
		FScopeTryLock lock(&AIMovementLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::AIMovement);

			return;
		}

		for (FFactionStruct faction : Camera->ConquestManager->Factions) {
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

		TArray<AAI*> ais;
		ais.Append(cs);
		ais.Append(rebels);
		ais.Append(clones);
		ais.Append(gamemode->Enemies);
		ais.Append(gamemode->Snakes);
		ais.Append(Camera->Grid->Birds);

		MaxCounter = FMath::CeilToInt32(ais.Num() / 500.0f);
		Counter = 0;

		for (int32 i = 0; i < MaxCounter; i++) {
			Async(EAsyncExecution::TaskGraph, [this, Camera, ais, i, cs, rebels, clones, gamemode]() {
				if (Camera->SaveGameComponent->IsLoading()) {
					Counter = MaxCounter;

					return;
				}

				TMap<FString, TMap<int32, FTransform>> instanceTransformsToUpdate;
				instanceTransformsToUpdate.Add("Citizens");
				instanceTransformsToUpdate.Add("Rebels");
				instanceTransformsToUpdate.Add("Clones");
				instanceTransformsToUpdate.Add("Enemies");
				instanceTransformsToUpdate.Add("Snakes");
				instanceTransformsToUpdate.Add("Birds");

				TMap<FString, TArray<int32>> instances;
				instances.Add("Citizens");
				instances.Add("Rebels");
				instances.Add("Clones");
				instances.Add("Enemies");
				instances.Add("Snakes");
				instances.Add("Birds");

				TArray<FHatsToUpdateStruct> hatsToUpdate;

				for (int32 j = (ais.Num() * i) / MaxCounter; j < (ais.Num() * (i + 1)) / MaxCounter; j++) {
					if (Camera->SaveGameComponent->IsLoading()) {
						Counter = MaxCounter;

						return;
					}

					int32 index = j;
					AAI* ai = ais[index];

					if (ai == nullptr)
						continue;

					FString id = "";
					UAIInstancedStaticMeshComponent* ism = nullptr;
					if (cs.Contains(ai)) {
						id = "Citizens";
						ism = HISMCitizen;
					}
					else if (rebels.Contains(ai)) {
						id = "Rebels";
						ism = HISMRebel;
						index -= cs.Num();
					}
					else if (clones.Contains(ai)) {
						id = "Clones";
						ism = HISMClone;
						index -= (cs.Num() + rebels.Num());
					}
					else if (gamemode->Enemies.Contains(ai)) {
						id = "Enemies";
						ism = HISMEnemy;
						index -= (cs.Num() + rebels.Num() + clones.Num());
					}
					else if (gamemode->Snakes.Contains(ai)) {
						id = "Snakes";
						ism = HISMSnake;
						index -= (cs.Num() + rebels.Num() + clones.Num() + gamemode->Enemies.Num());
					}
					else {
						id = "Birds";
						ism = HISMBird;
						index -= (cs.Num() + rebels.Num() + clones.Num() + gamemode->Enemies.Num() + gamemode->Snakes.Num());
					}

					float deltaTime = FMath::Min(GetWorld()->GetTimeSeconds() - ai->MovementComponent->LastUpdatedTime, 1.0f);
					ai->MovementComponent->ComputeMovement(deltaTime, *instances.Find(id));

					if (id == "Citizens" || id == "Rebels") {
						ACitizen* citizen = Cast<ACitizen>(ai);

						if (id == "Citizens") {
							float opacity = 1.0f;
							if (IsValid(citizen->BuildingComponent->BuildingAt) && !Camera->PoliceManager->IsInJail(citizen)) {
								if (citizen->BuildingComponent->BuildingAt->bHideCitizen)
									opacity = 0.0f;
								else if (!citizen->BuildingComponent->BuildingAt->SocketList.IsEmpty()) {
									FSocketStruct socketStruct;
									socketStruct.Citizen = citizen;

									int32 socketIndex = citizen->BuildingComponent->BuildingAt->SocketList.Find(socketStruct);

									if (socketIndex != INDEX_NONE)
										citizen->MovementComponent->Transform.SetLocation(citizen->BuildingComponent->BuildingAt->SocketList[socketIndex].SocketLocation);
								}
							}

							UpdateInstanceCustomData(ism, index, 1, citizen->bSelected * 2.0f, *instances.Find(id));
							UpdateInstanceCustomData(ism, index, 14, opacity, *instances.Find(id));
							UpdateInstanceCustomData(ism, index, 18, citizen->bCommander, *instances.Find(id));
							UpdateInstanceCustomData(ism, index, 19, Camera->EventsManager->IsProtest(citizen), *instances.Find(id));

							UpdateHatTransform(citizen, hatsToUpdate);

							SetEyesVisuals(ism, index, citizen, *instances.Find(id));
						}

						UpdateCitizenVisuals(ism, Camera, citizen, index, *instances.Find(id));
					}
					else if (id == "Clones")
						UpdateInstanceCustomData(ism, index, 1, 3.0f, *instances.Find(id));

					SetInstanceTransform(ism, index, ai->MovementComponent->GetMovementTransform(), *instanceTransformsToUpdate.Find(id));

					UpdateGradualVisuals(ism, ai, index, deltaTime, *instances.Find(id));

					SetAIColour(ism, index, ai->Colour, *instances.Find(id));
				}

				HISMCitizen->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Citizens"));
				HISMCitizen->BatchUpdateData(*instances.Find("Citizens"));

				for (FHatsToUpdateStruct htu : hatsToUpdate) {
					htu.ISM->BatchUpdateTransforms(htu.InstanceTransformsToUpdate);
					htu.ISM->BatchUpdateData(htu.InstanceDataToUpdate);
				}

				HISMRebel->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Rebels"));
				HISMRebel->BatchUpdateData(*instances.Find("Rebels"));

				HISMClone->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Clones"));
				HISMClone->BatchUpdateData(*instances.Find("Clones"));

				HISMEnemy->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Enemies"));
				HISMEnemy->BatchUpdateData(*instances.Find("Enemies"));

				HISMSnake->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Snakes"));
				HISMSnake->BatchUpdateData(*instances.Find("Snakes"));

				HISMBird->BatchUpdateTransforms(*instanceTransformsToUpdate.Find("Birds"));
				HISMBird->BatchUpdateData(*instances.Find("Birds"));

				FScopeLock lock(&CounterLock);
				Counter++;
			});
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

			UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
			if (healthComp && healthComp->GetHealth() > 0) {
				DestructingActors.Remove(actor);

				continue;
			}

			double deathTime = *DestructingActors.Find(actor);
			double alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - deathTime) / 10.0f, 0.0f, 1.0f);

			UStaticMeshComponent* mainMesh = actor->GetComponentByClass<UStaticMeshComponent>();
			FVector dimensions = mainMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();
			float z = dimensions.Z + 1.0f;

			TArray<UStaticMeshComponent*> meshes;
			actor->GetComponents<UStaticMeshComponent>(meshes);

			for (UStaticMeshComponent* mesh : meshes)
				mesh->SetCustomPrimitiveDataFloat(8, FMath::Lerp(0.0f, -z, alpha));

			if (alpha == 1.0f)
				DestructingActors.Remove(actor);
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

void UAIVisualiser::UpdateInstanceCustomData(UAIInstancedStaticMeshComponent* ISM, int32 Instance, int32 Index, float Value, TArray<int32>& Instances)
{
	if (Instance >= ISM->GetInstanceCount())
		return;

	int32 value = ISM->PerInstanceSMCustomData[Instance * ISM->NumCustomDataFloats + Index];

	if (value == Value)
		return;

	ISM->PerInstanceSMCustomData[Instance * ISM->NumCustomDataFloats + Index] = Value;

	if (!Instances.Contains(Instance))
		Instances.Add(Instance);
}

void UAIVisualiser::SetAIColour(UAIInstancedStaticMeshComponent* ISM, int32 Instance, FLinearColor Colour, TArray<int32>& Instances)
{
	UpdateInstanceCustomData(ISM, Instance, 2, Colour.R, Instances);
	UpdateInstanceCustomData(ISM, Instance, 3, Colour.G, Instances);
	UpdateInstanceCustomData(ISM, Instance, 4, Colour.B, Instances);
}

void UAIVisualiser::SetInstanceTransform(UAIInstancedStaticMeshComponent* ISM, int32 Instance, FTransform Transform, TMap<int32, FTransform>& InstanceTransformsToUpdate)
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

	InstanceTransformsToUpdate.Add(Instance, Transform);
}

void UAIVisualiser::UpdateCitizenVisuals(UAIInstancedStaticMeshComponent* ISM, ACamera* Camera, ACitizen* Citizen, int32 Instance, TArray<int32>& Instances)
{
	TTuple<bool, bool> status = Camera->DiseaseManager->HasInjuryAndInfection(Citizen);

	UpdateInstanceCustomData(ISM, Instance, 10, status.Key, Instances);

	if (Citizen->bGlasses)
		UpdateInstanceCustomData(ISM, Instance, 11, 1.0f, Instances);

	ActivateTorch(Camera, ISM, Instance, Instances);

	UpdateInstanceCustomData(ISM, Instance, 13, status.Value, Instances);
}

void UAIVisualiser::UpdateGradualVisuals(UAIInstancedStaticMeshComponent* ISM, AAI* AI, int32 Instance, float DeltaTime, TArray<int32>& Instances)
{
	// Wet
	int32 value = ISM->PerInstanceSMCustomData[Instance * ISM->NumCustomDataFloats];
	if (AI->bWet) {
		value += DeltaTime;

		if (AI->bWet && value >= 1.0f)
			value = 30.0f;
	}
	else if (value > 0.0f)
		value -= DeltaTime;

	UpdateInstanceCustomData(ISM, Instance, 0, FMath::Max(value, 0.0f), Instances);

	// Damage
	AI->DamageOverlayTimer = FMath::Max(AI->DamageOverlayTimer - DeltaTime, 0.0f);
	UpdateInstanceCustomData(ISM, Instance, 9, AI->DamageOverlayTimer > 0.0f ? 1.0f : 0.0f, Instances);
}

void UAIVisualiser::ActivateTorch(ACamera* Camera, UAIInstancedStaticMeshComponent* ISM, int32 Instance, TArray<int32>& Instances)
{
	int32 hour = Camera->Grid->AtmosphereComponent->Calendar.Hour;

	int32 value = 0.0f;
	if (Camera->Settings->GetRenderTorches() && (hour >= 18 || hour < 6))
		value = 1.0f;

	UpdateInstanceCustomData(ISM, Instance, 12, value, Instances);
}

void UAIVisualiser::SetHarvestVisuals(ACitizen* Citizen, AResource* Resource)
{
	USoundBase* sound = nullptr;
	FVector location = Citizen->MovementComponent->GetMovementTransform().GetLocation();

	if (IsValid(Citizen->BuildingComponent->Employment) && !Citizen->BuildingComponent->Employment->bHideCitizen) {
		FLinearColor colour = FLinearColor();
		for (FResourceStruct resourceStruct : Citizen->Camera->ResourceManager->ResourceList) {
			if (!Resource->IsA(resourceStruct.Type))
				continue;

			FString category = resourceStruct.Category;
			colour = *HarvestVisuals.Find(category);
		}

		location = AddHarvestVisual(Citizen, colour);
	}

	TArray<USoundBase*> sounds;
	if (Resource->IsA<AMineral>())
		sounds = Citizen->Mines;
	else if (Resource->IsA<AVegetation>())
		sounds = Citizen->Chops;
	else
		sounds = Citizen->Anvils;

	sound = sounds[Citizen->Camera->Stream.RandRange(0, sounds.Num() - 1)];

	HarvestVisualCooldownTimer = 5.0f;

	Citizen->AmbientAudioComponent->SetRelativeLocation(location);
	Citizen->Camera->AudioManager->PlayAmbientSound(Citizen->AmbientAudioComponent, sound);
}

FVector UAIVisualiser::AddHarvestVisual(AAI* AI, FLinearColor Colour)
{
	FVector location = AI->MovementComponent->GetMovementTransform().GetLocation() + (AI->MovementComponent->Transform.Rotator().Vector() * 20.0f) + FVector(0.0f, 0.0f, 17.0f);

	TArray<FVector> locations = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(HarvestNiagaraComponent, "Locations");
	locations.Add(location);

	TArray<FLinearColor> colours = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayColor(HarvestNiagaraComponent, "Colours");
	colours.Add(Colour);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(HarvestNiagaraComponent, "Locations", locations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(HarvestNiagaraComponent, "Colours", colours);

	if (!HarvestNiagaraComponent->IsActive())
		Async(EAsyncExecution::TaskGraphMainTick, [this]() { HarvestNiagaraComponent->Activate(); });

	return location;
}

void UAIVisualiser::SetEyesVisuals(class UAIInstancedStaticMeshComponent* ISM, int32 Instance, ACitizen* Citizen, TArray<int32>& Instances)
{
	int32 happinessValue = Citizen->HappinessComponent->GetHappiness();

	float val15 = 0.0f;
	float val16 = 0.0f;
	float val17 = 0.0f;

	if (!Citizen->AttackComponent->OverlappingEnemies.IsEmpty())
		val16 = 1.0f;
	else if (!Citizen->HealthIssues.IsEmpty() || happinessValue < 35)
		val17 = 1.0f;
	else if (happinessValue > 65)
		val15 = 1.0f;

	UpdateInstanceCustomData(ISM, Instance, 15, val15, Instances);
	UpdateInstanceCustomData(ISM, Instance, 16, val16, Instances);
	UpdateInstanceCustomData(ISM, Instance, 17, val17, Instances);
}

TTuple<UAIInstancedStaticMeshComponent*, int32> UAIVisualiser::GetAIHISM(AAI* AI)
{
	TTuple<UAIInstancedStaticMeshComponent*, int32> info = TTuple<UAIInstancedStaticMeshComponent*, int32>(nullptr, INDEX_NONE);

	if (AI == nullptr)
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
	else if (AI->Camera->Grid->Birds.Contains(AI)) {
		info.Key = HISMBird;
		info.Value = AI->Camera->Grid->Birds.Find(AI);
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

AAI* UAIVisualiser::GetHISMAI(ACamera* Camera, UAIInstancedStaticMeshComponent* ISM, int32 Instance)
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

	if (ISM == HISMCitizen && citizens.Num() > Instance)
		ai = citizens[Instance];
	else if (ISM == HISMRebel && rebels.Num() > Instance)
		ai = rebels[Instance];
	else if (ISM == HISMClone && clones.Num() > Instance)
		ai = clones[Instance];
	else if (ISM == HISMEnemy && gamemode->Enemies.Num() > Instance)
		ai = gamemode->Enemies[Instance];
	else if (ISM == HISMSnake && gamemode->Snakes.Num() > Instance)
		ai = gamemode->Snakes[Instance];
	else if (ISM == HISMBird && Camera->Grid->Birds.Num() > Instance)
		ai = Camera->Grid->Birds[Instance];

	return ai;
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;

	FTransform transform = FTransform();

	auto info = GetAIHISM(AI);

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

void UAIVisualiser::SetAnimationPoint(AAI* AI, FTransform Transform, TArray<int32>& Instances)
{
	auto info = GetAIHISM(AI);

	if (!IsValid(info.Key) || info.Value == -1)
		return;

	UpdateInstanceCustomData(info.Key, info.Value, 5, Transform.GetLocation().X, Instances);
	UpdateInstanceCustomData(info.Key, info.Value, 6, Transform.GetLocation().Y, Instances);
	UpdateInstanceCustomData(info.Key, info.Value, 7, Transform.GetLocation().Z, Instances);
	UpdateInstanceCustomData(info.Key, info.Value, 8, Transform.GetRotation().Rotator().Pitch, Instances);
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

		if (RequestedOverlaps.bUnbuiltBuildings)
			actorsToCheck.Append(Camera->BuildComponent->Buildings);

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
	FTransform transform = Citizen->MovementComponent->GetMovementTransform();

	FVector location = transform.GetLocation() + animTransform.GetLocation();
	transform.SetLocation(location);

	FRotator rotation = transform.GetRotation().Rotator() + animTransform.GetRotation().Rotator();
	rotation.Normalize();
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::UpdateHatTransform(ACitizen* Citizen, TArray<FHatsToUpdateStruct>& HatsToUpdate)
{
	FHatsStruct* hatStruct = GetCitizenHat(Citizen);

	if (hatStruct == nullptr)
		return;
	else if (!IsValid(Citizen->BuildingComponent->Employment)) {
		RemoveCitizenFromHISMHat(Citizen);

		return;
	}

	int32 index = hatStruct->Citizens.Find(Citizen);

	FHatsToUpdateStruct htu;
	htu.ISM = hatStruct->ISMHat;
	int32 i = HatsToUpdate.Find(htu);

	if (i == INDEX_NONE)
		i = HatsToUpdate.Add(htu);

	FTransform transform = GetHatTransform(Citizen);
	SetInstanceTransform(hatStruct->ISMHat, index, transform, HatsToUpdate[i].InstanceTransformsToUpdate);

	float opacity = 1.0f;
	if (IsValid(Citizen->BuildingComponent->BuildingAt) && Citizen->BuildingComponent->BuildingAt->bHideCitizen)
		opacity = 0.0f;

	UpdateInstanceCustomData(hatStruct->ISMHat, index, 1, opacity, HatsToUpdate[i].InstanceDataToUpdate);

	if (hatStruct->ISMHat->NumCustomDataFloats < 3)
		return;

	float lights = 0.0f;
	if (Citizen->Camera->PoliceManager->IsPoliceOfficer(Citizen) && Citizen->Camera->PoliceManager->IsInAPoliceReport(Citizen, Citizen->Camera->ConquestManager->GetFaction("", Citizen)))
		lights = 1.0f;

	UpdateInstanceCustomData(hatStruct->ISMHat, index, 2, lights, HatsToUpdate[i].InstanceDataToUpdate);
}

void UAIVisualiser::AddCitizenToHISMHat(ACitizen* Citizen, UStaticMesh* HatMesh)
{
	for (FHatsStruct& hat : HISMHats) {
		if (hat.ISMHat->GetStaticMesh() != HatMesh)
			continue;

		hat.Citizens.Add(Citizen);

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