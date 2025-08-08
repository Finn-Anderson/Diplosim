#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "Universal/DiplosimUserSettings.h"

UAIVisualiser::UAIVisualiser()
{
	PrimaryComponentTick.bCanEverTick = false;

	FCollisionResponseContainer response;
	response.SetAllChannels(ECR_Ignore);
	response.Visibility = ECR_Block;
	response.WorldStatic = ECR_Block;

	HISMContainer = CreateDefaultSubobject<USceneComponent>(TEXT("HISMContainer"));

	HISMCitizen = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMCitizen"));
	HISMCitizen->SetupAttachment(HISMContainer);
	HISMCitizen->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMCitizen->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMCitizen->SetCollisionResponseToChannels(response);
	HISMCitizen->SetCanEverAffectNavigation(false);
	HISMCitizen->SetGenerateOverlapEvents(false);
	HISMCitizen->bWorldPositionOffsetWritesVelocity = false;
	HISMCitizen->bAutoRebuildTreeOnInstanceChanges = false;
	HISMCitizen->NumCustomDataFloats = 11;

	HISMClone = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClone"));
	HISMClone->SetupAttachment(HISMContainer);
	HISMClone->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMClone->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMClone->SetCollisionResponseToChannels(response);
	HISMClone->SetCanEverAffectNavigation(false);
	HISMClone->SetGenerateOverlapEvents(false);
	HISMClone->bWorldPositionOffsetWritesVelocity = false;
	HISMClone->bAutoRebuildTreeOnInstanceChanges = false;
	HISMClone->NumCustomDataFloats = 8;

	HISMRebel = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRebel"));
	HISMRebel->SetupAttachment(HISMContainer);
	HISMRebel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMRebel->SetCollisionObjectType(ECC_GameTraceChannel3);
	HISMRebel->SetCollisionResponseToChannels(response);
	HISMRebel->SetCanEverAffectNavigation(false);
	HISMRebel->SetGenerateOverlapEvents(false);
	HISMRebel->bWorldPositionOffsetWritesVelocity = false;
	HISMRebel->bAutoRebuildTreeOnInstanceChanges = false;
	HISMRebel->NumCustomDataFloats = 11;

	HISMEnemy = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMEnemy"));
	HISMEnemy->SetupAttachment(HISMContainer);
	HISMEnemy->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMEnemy->SetCollisionObjectType(ECC_GameTraceChannel4);
	HISMEnemy->SetCollisionResponseToChannels(response);
	HISMEnemy->SetCanEverAffectNavigation(false);
	HISMEnemy->SetGenerateOverlapEvents(false);
	HISMEnemy->bWorldPositionOffsetWritesVelocity = false;
	HISMEnemy->bAutoRebuildTreeOnInstanceChanges = false;
	HISMEnemy->NumCustomDataFloats = 8;

	TorchNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TorchNiagaraComponent"));
	TorchNiagaraComponent->SetupAttachment(HISMContainer);
	TorchNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	TorchNiagaraComponent->bAutoActivate = false;

	DiseaseNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DiseaseNiagaraComponent"));
	DiseaseNiagaraComponent->SetupAttachment(HISMContainer);
	DiseaseNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	DiseaseNiagaraComponent->bAutoActivate = false;

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetupAttachment(HISMContainer);
	HarvestNiagaraComponent->PrimaryComponentTick.bCanEverTick = false;
	HarvestNiagaraComponent->bAutoActivate = false;
}

void UAIVisualiser::BeginPlay()
{
	Super::BeginPlay();

	// ECC_WorldDynamic, ECC_Pawn, ECC_GameTraceChannel2, ECC_GameTraceChannel3, ECC_GameTraceChannel4: Consider ECR_Block
}

void UAIVisualiser::MainLoop(ACamera* Camera)
{
	for (FPendingChangeStruct pending : PendingChange) {
		if (pending.Instance == 0) {
			int32 instance = pending.HISM->AddInstance(pending.Transform, false);

			FLinearColor colour;
			if (pending.AI->IsA<AEnemy>()) {
				AEnemy* enemy = Cast<AEnemy>(pending.AI);

				colour = enemy->Colour;
				enemy->SpawnComponent->SetWorldLocation(pending.Transform.GetLocation()); // Consider making spawn component like torch niagara component
			}
			else
				colour = FLinearColor(Camera->Grid->Stream.FRandRange(0.0f, 1.0f), Camera->Grid->Stream.FRandRange(0.0f, 1.0f), Camera->Grid->Stream.FRandRange(0.0f, 1.0f));

			pending.HISM->SetCustomDataValue(instance, 0, colour.R);
			pending.HISM->SetCustomDataValue(instance, 1, colour.G);
			pending.HISM->SetCustomDataValue(instance, 2, colour.B);

			ActivateTorches(Camera->Grid->AtmosphereComponent->Calendar.Hour, pending.HISM, instance);
		}
		else
			pending.HISM->RemoveInstance(pending.Instance);
	}

	PendingChange.Empty();

	TArray<FTransform> diseaseTransforms;
	TArray<FTransform> torchTransforms;

	TArray<TArray<ACitizen*>> citizens;
	citizens.Add(Camera->CitizenManager->Citizens);
	citizens.Add(Camera->CitizenManager->Rebels);

	for (int32 i = 0; i < citizens.Num(); i++) {
		for (int32 j = 0; j < citizens[i].Num(); j++) {
			ACitizen* citizen = citizens[i][j];

			citizen->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - citizen->MovementComponent->LastUpdatedTime);

			if (i == 0)
				HISMCitizen->UpdateInstanceTransform(j, citizen->MovementComponent->Transform, true);
			else
				HISMRebel->UpdateInstanceTransform(j, citizen->MovementComponent->Transform, true);

			if (Camera->CitizenManager->Infected.Contains(citizen))
				diseaseTransforms.Add(citizen->MovementComponent->Transform);

			if (TorchNiagaraComponent->IsActive())
				torchTransforms.Add(citizen->MovementComponent->Transform);

			UpdateCitizenVisuals(Camera, citizen, j);
		}
	}

	TArray<TArray<AAI*>> ais;
	ais.Add(Camera->CitizenManager->Clones);
	ais.Add(Camera->CitizenManager->Enemies);

	for (int32 i = 0; i < ais.Num(); i++) {
		for (int32 j = 0; j < ais[i].Num(); j++) {
			AAI* ai = ais[i][j];

			ai->MovementComponent->ComputeMovement(GetWorld()->GetTimeSeconds() - ai->MovementComponent->LastUpdatedTime);
			HISMClone->UpdateInstanceTransform(j, ai->MovementComponent->Transform, true);
		}
	}

	// If torch/disease niagara active, update locations here. Figure out how to make it update only on change (increment number?) Or set spawn rate to transform num and loop through transform.

	HISMCitizen->BuildTreeIfOutdated(false, true);
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

void UAIVisualiser::UpdateCitizenVisuals(ACamera* Camera, ACitizen* Citizen, int32 Instance)
{
	if (Citizen->bGlasses && HISMCitizen->PerInstanceSMCustomData[Instance * 11 + 9] == 0.0f)
		HISMCitizen->SetCustomDataValue(Instance, 9, 1.0f);

	if (Camera->CitizenManager->Injured.Contains(Citizen))
		if (HISMCitizen->PerInstanceSMCustomData[Instance * 11 + 8] == 0.0f)
			HISMCitizen->SetCustomDataValue(Instance, 8, 1.0f);
		else if (HISMCitizen->PerInstanceSMCustomData[Instance * 11 + 8] == 1.0f)
			HISMCitizen->SetCustomDataValue(Instance, 8, 0.0f);
}

void UAIVisualiser::ActivateTorches(int32 Hour, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	int32 value = 0.0f;

	if (settings->GetRenderTorches() && (Hour >= 18 || Hour < 6))
		value = 1.0f;

	if (Instance == INDEX_NONE) {
		for (int32 i = 0; i < HISMCitizen->GetInstanceCount(); i++)
			HISMCitizen->SetCustomDataValue(i, 10, value);

		for (int32 i = 0; i < HISMRebel->GetInstanceCount(); i++)
			HISMRebel->SetCustomDataValue(i, 10, value);

		if (value == 1.0f && !TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Activate();
		else if (TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Deactivate();
	}
	else {
		HISM->SetCustomDataValue(Instance, 10, value);
	}
}
