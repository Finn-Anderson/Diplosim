#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
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
	HISMCitizen->NumCustomDataFloats = 12;

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
	HarvestNiagaraComponent->bAutoActivate = true;
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

			pending.HISM->SetCustomDataValue(instance, 1, colour.R);
			pending.HISM->SetCustomDataValue(instance, 2, colour.G);
			pending.HISM->SetCustomDataValue(instance, 3, colour.B);

			ActivateTorches(Camera->Grid->AtmosphereComponent->Calendar.Hour, pending.HISM, instance);
		}
		else
			pending.HISM->RemoveInstance(pending.Instance);
	}

	PendingChange.Empty();

	TArray<FVector> diseaseLocations;
	TArray<FVector> torchLocations;

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
				diseaseLocations.Add(citizen->MovementComponent->Transform.GetLocation());

			if (TorchNiagaraComponent->IsActive())
				torchLocations.Add(citizen->MovementComponent->Transform.GetLocation());

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

	if (!torchLocations.IsEmpty()) {
		torchLocations.Append(UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(TorchNiagaraComponent, "Locations"));
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TorchNiagaraComponent, "Locations", torchLocations);
	}

	if (!diseaseLocations.IsEmpty()) {
		diseaseLocations.Append(UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(DiseaseNiagaraComponent, "Locations"));
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(DiseaseNiagaraComponent, "Locations", diseaseLocations);
	}

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
	if (Citizen->bGlasses && HISMCitizen->PerInstanceSMCustomData[Instance * 12 + 9] == 0.0f)
		HISMCitizen->SetCustomDataValue(Instance, 9, 1.0f);

	if (Camera->CitizenManager->Injured.Contains(Citizen))
		if (HISMCitizen->PerInstanceSMCustomData[Instance * 12 + 8] == 0.0f)
			HISMCitizen->SetCustomDataValue(Instance, 8, 1.0f);
		else if (HISMCitizen->PerInstanceSMCustomData[Instance * 12 + 8] == 1.0f)
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

void UAIVisualiser::AddHarvestVisual(FVector Location, FLinearColor Colour)
{
	TArray<FVector> locations = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector(HarvestNiagaraComponent, "Locations");
	locations.Add(Location);

	TArray<FLinearColor> colours = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayColor(HarvestNiagaraComponent, "Colours");
	colours.Add(Colour);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(HarvestNiagaraComponent, "Locations", locations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(HarvestNiagaraComponent, "Colours", colours);
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;
	
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;
	int32 instance = -1;

	if (cm->Citizens.Contains(AI)) {
		instance = cm->Citizens.Find(Cast<ACitizen>(AI));

		position.X = HISMCitizen->PerInstanceSMCustomData[instance * 12 + 4];
		position.Y = HISMCitizen->PerInstanceSMCustomData[instance * 12 + 5];
		position.Z = HISMCitizen->PerInstanceSMCustomData[instance * 12 + 6];
		rotation.Pitch = HISMCitizen->PerInstanceSMCustomData[instance * 12 + 7];
	}
	else if (cm->Rebels.Contains(AI)) {
		instance = cm->Rebels.Find(Cast<ACitizen>(AI));

		position.X = HISMRebel->PerInstanceSMCustomData[instance * 11 + 4];
		position.Y = HISMRebel->PerInstanceSMCustomData[instance * 11 + 5];
		position.Z = HISMRebel->PerInstanceSMCustomData[instance * 11 + 6];
		rotation.Pitch = HISMRebel->PerInstanceSMCustomData[instance * 11 + 7];
	}
	else if (cm->Clones.Contains(AI)) {
		instance = cm->Clones.Find(AI);

		position.X = HISMClone->PerInstanceSMCustomData[instance * 8 + 4];
		position.Y = HISMClone->PerInstanceSMCustomData[instance * 8 + 5];
		position.Z = HISMClone->PerInstanceSMCustomData[instance * 8 + 6];
		rotation.Pitch = HISMClone->PerInstanceSMCustomData[instance * 8 + 7];
	}
	else {
		instance = cm->Enemies.Find(AI);

		position.X = HISMEnemy->PerInstanceSMCustomData[instance * 8 + 4];
		position.Y = HISMEnemy->PerInstanceSMCustomData[instance * 8 + 5];
		position.Z = HISMEnemy->PerInstanceSMCustomData[instance * 8 + 6];
		rotation.Pitch = HISMEnemy->PerInstanceSMCustomData[instance * 8 + 7];
	}

	FTransform transform;
	transform.SetLocation(position);
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::SetAnimationPoint(AAI* AI, FTransform Transform)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;
	int32 instance = -1.0f;

	if (cm->Citizens.Contains(AI)) {
		instance = cm->Citizens.Find(Cast<ACitizen>(AI));

		HISMCitizen->SetCustomDataValue(instance, 4, Transform.GetLocation().X);
		HISMCitizen->SetCustomDataValue(instance, 5, Transform.GetLocation().Y);
		HISMCitizen->SetCustomDataValue(instance, 6, Transform.GetLocation().Z);
		HISMCitizen->SetCustomDataValue(instance, 7, Transform.GetRotation().Rotator().Pitch);
	}
	else if (cm->Rebels.Contains(AI)) {
		instance = cm->Rebels.Find(Cast<ACitizen>(AI));

		HISMRebel->SetCustomDataValue(instance, 4, Transform.GetLocation().X);
		HISMRebel->SetCustomDataValue(instance, 5, Transform.GetLocation().Y);
		HISMRebel->SetCustomDataValue(instance, 6, Transform.GetLocation().Z);
		HISMRebel->SetCustomDataValue(instance, 7, Transform.GetRotation().Rotator().Pitch);
	}
	else if (cm->Clones.Contains(AI)) {
		instance = cm->Clones.Find(AI);

		HISMClone->SetCustomDataValue(instance, 4, Transform.GetLocation().X);
		HISMClone->SetCustomDataValue(instance, 5, Transform.GetLocation().Y);
		HISMClone->SetCustomDataValue(instance, 6, Transform.GetLocation().Z);
		HISMClone->SetCustomDataValue(instance, 7, Transform.GetRotation().Rotator().Pitch);
	}
	else {
		instance = cm->Enemies.Find(AI);

		HISMEnemy->SetCustomDataValue(instance, 4, Transform.GetLocation().X);
		HISMEnemy->SetCustomDataValue(instance, 5, Transform.GetLocation().Y);
		HISMEnemy->SetCustomDataValue(instance, 6, Transform.GetLocation().Z);
		HISMEnemy->SetCustomDataValue(instance, 7, Transform.GetRotation().Rotator().Pitch);
	}
}
