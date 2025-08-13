#include "Map/AIVisualiser.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/AIMovementComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/Resource.h"
#include "Buildings/Building.h"

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
	HISMCitizen->NumCustomDataFloats = 13;

	HISMClone = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClone"));
	HISMClone->SetupAttachment(HISMContainer);
	HISMClone->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMClone->SetCollisionObjectType(ECC_GameTraceChannel2);
	HISMClone->SetCollisionResponseToChannels(response);
	HISMClone->SetCanEverAffectNavigation(false);
	HISMClone->SetGenerateOverlapEvents(false);
	HISMClone->bWorldPositionOffsetWritesVelocity = false;
	HISMClone->bAutoRebuildTreeOnInstanceChanges = false;
	HISMClone->NumCustomDataFloats = 9;

	HISMRebel = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMRebel"));
	HISMRebel->SetupAttachment(HISMContainer);
	HISMRebel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMRebel->SetCollisionObjectType(ECC_GameTraceChannel3);
	HISMRebel->SetCollisionResponseToChannels(response);
	HISMRebel->SetCanEverAffectNavigation(false);
	HISMRebel->SetGenerateOverlapEvents(false);
	HISMRebel->bWorldPositionOffsetWritesVelocity = false;
	HISMRebel->bAutoRebuildTreeOnInstanceChanges = false;
	HISMRebel->NumCustomDataFloats = 12;

	HISMEnemy = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMEnemy"));
	HISMEnemy->SetupAttachment(HISMContainer);
	HISMEnemy->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMEnemy->SetCollisionObjectType(ECC_GameTraceChannel4);
	HISMEnemy->SetCollisionResponseToChannels(response);
	HISMEnemy->SetCanEverAffectNavigation(false);
	HISMEnemy->SetGenerateOverlapEvents(false);
	HISMEnemy->bWorldPositionOffsetWritesVelocity = false;
	HISMEnemy->bAutoRebuildTreeOnInstanceChanges = false;
	HISMEnemy->NumCustomDataFloats = 9;

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

			if (IsValid(pending.AI->SpawnSystem))
				UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), pending.AI->SpawnSystem, pending.Transform.GetLocation());

			FLinearColor colour;
			if (pending.AI->IsA<AEnemy>()) {
				AEnemy* enemy = Cast<AEnemy>(pending.AI);

				colour = enemy->Colour;
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
	if (Citizen->bGlasses && HISMCitizen->PerInstanceSMCustomData[Instance * 13 + 10] == 0.0f)
		HISMCitizen->SetCustomDataValue(Instance, 10, 1.0f);

	if (Camera->CitizenManager->Injured.Contains(Citizen))
		if (HISMCitizen->PerInstanceSMCustomData[Instance * 13 + 9] == 0.0f)
			HISMCitizen->SetCustomDataValue(Instance, 9, 1.0f);
		else if (HISMCitizen->PerInstanceSMCustomData[Instance * 13 + 9] == 1.0f)
			HISMCitizen->SetCustomDataValue(Instance, 9, 0.0f);
}

void UAIVisualiser::ActivateTorches(int32 Hour, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	int32 value = 0.0f;

	if (settings->GetRenderTorches() && (Hour >= 18 || Hour < 6))
		value = 1.0f;

	if (Instance == INDEX_NONE) {
		for (int32 i = 0; i < HISMCitizen->GetInstanceCount(); i++)
			HISMCitizen->SetCustomDataValue(i, 11, value);

		for (int32 i = 0; i < HISMRebel->GetInstanceCount(); i++)
			HISMRebel->SetCustomDataValue(i, 11, value);

		if (value == 1.0f && !TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Activate();
		else if (TorchNiagaraComponent->IsActive())
			TorchNiagaraComponent->Deactivate();
	}
	else {
		HISM->SetCustomDataValue(Instance, 11, value);
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

	if (cm->Citizens.Contains(AI)) {
		info.Key = HISMCitizen;
		info.Value = cm->Citizens.Find(Cast<ACitizen>(AI));
	}
	else if (cm->Rebels.Contains(AI)) {
		info.Key = HISMRebel;
		info.Value = cm->Rebels.Find(Cast<ACitizen>(AI));
	}
	else if (cm->Clones.Contains(AI)) {
		info.Key = HISMClone;
		info.Value = cm->Clones.Find(AI);
	}
	else {
		info.Key = HISMEnemy;
		info.Value = cm->Enemies.Find(AI);
	}

	return info;
}

FTransform UAIVisualiser::GetAnimationPoint(AAI* AI)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;
	
	FVector position = FVector::Zero();
	FRotator rotation = FRotator::ZeroRotator;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	int32 numCustomData = 9;

	if (info.Key == HISMCitizen)
		numCustomData = 13;
	else if (info.Key == HISMRebel)
		numCustomData = 12;

	position.X = info.Key->PerInstanceSMCustomData[info.Value * numCustomData + 4];
	position.Y = info.Key->PerInstanceSMCustomData[info.Value * numCustomData + 5];
	position.Z = info.Key->PerInstanceSMCustomData[info.Value * numCustomData + 6];
	rotation.Pitch = info.Key->PerInstanceSMCustomData[info.Value * numCustomData + 7];

	FTransform transform;
	transform.SetLocation(position);
	transform.SetRotation(rotation.Quaternion());

	return transform;
}

void UAIVisualiser::SetAnimationPoint(AAI* AI, FTransform Transform)
{
	UCitizenManager* cm = AI->Camera->CitizenManager;

	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = GetAIHISM(AI);

	info.Key->SetCustomDataValue(info.Value, 4, Transform.GetLocation().X);
	info.Key->SetCustomDataValue(info.Value, 5, Transform.GetLocation().Y);
	info.Key->SetCustomDataValue(info.Value, 6, Transform.GetLocation().Z);
	info.Key->SetCustomDataValue(info.Value, 7, Transform.GetRotation().Rotator().Pitch);
}

TArray<AActor*> UAIVisualiser::GetOverlaps(ACamera* Camera, AActor* Actor, float Range)
{
	TArray<AActor*> actors;

	UCitizenManager* cm = Camera->CitizenManager;

	TArray<AActor*> actorsToCheck;
	actorsToCheck.Append(cm->Buildings);
	actorsToCheck.Append(cm->Citizens);
	actorsToCheck.Append(cm->Clones);
	actorsToCheck.Append(cm->Rebels);
	actorsToCheck.Append(cm->Enemies);

	FVector location = Camera->GetTargetLocation(Actor);

	for (AActor* actor : actorsToCheck) {
		FVector loc = cm->Camera->GetTargetLocation(actor);

		if (actor->IsA<ABuilding>())
			Cast<ABuilding>(actor)->BuildingMesh->GetClosestPointOnCollision(location, loc);

		float distance = FVector::Dist(location, loc);

		if (distance <= Range)
			actors.Add(actor);
	}

	for (FResourceHISMStruct resourceStruct : cm->Camera->Grid->TreeStruct) {
		TArray<int32> instances = resourceStruct.Resource->ResourceHISM->GetInstancesOverlappingSphere(location, Range);

		if (!instances.IsEmpty())
			actors.Add(resourceStruct.Resource);
	}

	return actors;
}
