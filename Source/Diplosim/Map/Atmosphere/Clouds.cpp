#include "Clouds.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

#include "AtmosphereComponent.h"
#include "Map/Grid.h"
#include "Universal/DiplosimUserSettings.h"
#include "Buildings/Building.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "Universal/EggBasket.h"

UCloudComponent::UCloudComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CloudMesh = nullptr;

	CloudSystem = nullptr;
	Settings = nullptr;

	Height = 4000.0f;

	bSnow = false;
}

void UCloudComponent::BeginPlay()
{
	Super::BeginPlay();

	Settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	Settings->Clouds = this;
}

void UCloudComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.0001f)
		return;

	for (int32 i = Clouds.Num() - 1; i > -1; i--) {
		FCloudStruct cloudStruct = Clouds[i];

		FVector location = cloudStruct.HISMCloud->GetRelativeLocation() + (Cast<AGrid>(GetOwner())->AtmosphereComponent->WindRotation.Vector());

		float bobAmount = FMath::Sin(GetWorld()->GetTimeSeconds() / 2.0f) / 10.0f;
		location.Z += bobAmount;

		cloudStruct.HISMCloud->SetRelativeLocation(location);

		if (cloudStruct.Precipitation != nullptr)
			cloudStruct.Precipitation->SetVariableVec3(TEXT("CloudLocation"), cloudStruct.HISMCloud->GetRelativeLocation());

		UMaterialInstanceDynamic* material = Cast<UMaterialInstanceDynamic>(cloudStruct.HISMCloud->GetMaterial(0));
		float opacity = 0.0f;
		FHashedMaterialParameterInfo info;
		info.Name = FScriptName("Opacity");
		material->GetScalarParameterValue(info, opacity);

		if (!cloudStruct.bHide && opacity != 1.0f)
			material->SetScalarParameterValue("Opacity", FMath::Clamp(opacity + 0.005f, 0.0f, 1.0f));
		else if (cloudStruct.bHide)
			material->SetScalarParameterValue("Opacity", FMath::Clamp(opacity - 0.005f, 0.0f, 1.0f));

		double distance = FVector::Dist(location, Cast<AGrid>(GetOwner())->GetActorLocation());

		if (distance > cloudStruct.Distance) {
			if (cloudStruct.Precipitation != nullptr)
				cloudStruct.Precipitation->Deactivate();

			Clouds[i].bHide = true;

			if (opacity == 0.0f) {
				cloudStruct.HISMCloud->DestroyComponent();

				Clouds.Remove(cloudStruct);
			}
		}
	}
}

void UCloudComponent::Clear()
{
	for (FCloudStruct cloudStruct : Clouds) {
		if (cloudStruct.Precipitation != nullptr)
			cloudStruct.Precipitation->DestroyComponent();

		cloudStruct.HISMCloud->DestroyComponent();
	}

	Clouds.Empty();

	GetWorld()->GetTimerManager().ClearTimer(CloudTimer);
}

void UCloudComponent::ActivateCloud()
{
	if (!Settings->GetRenderClouds())
		return;

	FTransform transform;

	float x = FMath::FRandRange(1.0f, 10.0f);
	float y = FMath::FRandRange(1.0f, 10.0f);
	float z = FMath::FRandRange(1.0f, 4.0f);
	transform.SetScale3D(FVector(x, y, z));

	transform.SetRotation((Cast<AGrid>(GetOwner())->AtmosphereComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

	FVector spawnLoc = transform.GetRotation().Vector() * 20000.0f;
	spawnLoc.Z = Height + FMath::FRandRange(-200.0f, 200.0f);

	FVector limit = (transform.GetRotation().Rotator() + FRotator(0.0f, 90.0f, 0.0f)).Vector();
	float vary = FMath::FRandRange(-20000.0f, 20000.0f);
	limit *= vary;

	spawnLoc += limit;

	transform.SetLocation(spawnLoc);

	int32 chance = FMath::RandRange(1, 100);

	if (!Settings->GetRain())
		chance = 1;

	FCloudStruct cloudStruct = CreateCloud(transform, chance);
	cloudStruct.Distance = FVector::Dist(spawnLoc, Cast<AGrid>(GetOwner())->GetActorLocation());

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(cloudStruct.HISMCloud->GetMaterial(0), this);
	material->SetScalarParameterValue("Opacity", 0.0f);

	if (chance > 75)
		material->SetVectorParameterValue("Colour", FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));

	cloudStruct.HISMCloud->SetMaterial(0, material);

	Clouds.Add(cloudStruct);

	GetWorld()->GetTimerManager().SetTimer(CloudTimer, this, &UCloudComponent::ActivateCloud, 90.0f);
}

FCloudStruct UCloudComponent::CreateCloud(FTransform Transform, int32 Chance)
{
	UHierarchicalInstancedStaticMeshComponent* cloud = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass());
	cloud->SetStaticMesh(CloudMesh);
	cloud->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	cloud->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	cloud->SetAffectDistanceFieldLighting(false);
	cloud->SetRelativeLocation(Transform.GetLocation());
	cloud->SetRelativeRotation(Transform.GetRotation());
	cloud->SetupAttachment(GetOwner()->GetRootComponent());
	cloud->RegisterComponent();

	FCloudStruct cloudStruct;
	cloudStruct.HISMCloud = cloud;

	TArray<FVector> locations;

	for (int32 i = 0; i < 200; i++) {
		FTransform t = Transform;

		float x = FMath::FRandRange(-800.0f, 800.0f) * t.GetScale3D().X;
		float y = FMath::FRandRange(-800.0f, 800.0f) * t.GetScale3D().Y;
		float z = FMath::FRandRange(-256.0f, 256.0f) * t.GetScale3D().Z;
		t.SetLocation(FVector(x, y, z));

		float diff = FMath::FRandRange(20.0f, 40.0f);
		t.SetScale3D(t.GetScale3D() * diff);

		int32 inst = cloud->AddInstance(t);

		cloud->GetInstanceTransform(inst, t, true);
		t.SetLocation(t.GetLocation() - Transform.GetLocation());

		float bounds = cloud->GetStaticMesh().Get()->GetBounds().GetBox().GetSize().X / 2.0f;

		if (Chance > 75)
			locations.Append(SetPrecipitationLocations(t, bounds));
	}

	if (Chance <= 75)
		return cloudStruct;

	UNiagaraComponent* precipitation = UNiagaraFunctionLibrary::SpawnSystemAttached(CloudSystem, cloud, "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);
	precipitation->SetVariableObject(TEXT("Callback"), GetOwner());
	precipitation->SetVariableVec3(TEXT("CloudLocation"), Transform.GetLocation());

	float spawnRate = 0.0f;

	float gravity = -980.0f;
	float lifetime = (cloud->GetRelativeLocation().Z - Height + 200.0f) / 800.0f + 3.0f;

	if (Chance > 75) {
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(precipitation, TEXT("SpawnLocations"), locations);
		spawnRate = 400.0f * Transform.GetScale3D().X * Transform.GetScale3D().Y * FMath::FRandRange(0.5f, 1.5f);
	}

	if (bSnow) {
		precipitation->SetVariableFloat(TEXT("SnowSpawnRate"), spawnRate);
		gravity /= 4;
		lifetime *= 4;
	}
	else {
		precipitation->SetVariableFloat(TEXT("RainSpawnRate"), spawnRate);
	}

	precipitation->SetVariableVec3(TEXT("Gravity"), FVector(0.0f, 0.0f, gravity));
	precipitation->SetVariableFloat(TEXT("Life"), lifetime);

	precipitation->SetBoundsScale(3.0f);

	precipitation->Activate();

	cloudStruct.Precipitation = precipitation;

	return cloudStruct;
}

TArray<FVector> UCloudComponent::SetPrecipitationLocations(FTransform Transform, float Bounds)
{
	TArray<FVector> locations;

	float x = Bounds * Transform.GetScale3D().X - Transform.GetScale3D().X;
	float y = Bounds * Transform.GetScale3D().Y - Transform.GetScale3D().Y;

	for (int32 i = 0; i < 9; i++) {
		for (int32 j = 0; j < 9; j++) {
			float xCoord = (x - (x / 4.0f * i)) * FMath::Cos(FMath::DegreesToRadians(Transform.GetRotation().Rotator().Yaw)) - (y - (y / 4.0f * j)) * FMath::Sin(FMath::DegreesToRadians(Transform.GetRotation().Rotator().Yaw));
			float yCoord = (x - (x / 4.0f * i)) * FMath::Sin(FMath::DegreesToRadians(Transform.GetRotation().Rotator().Yaw)) + (y - (y / 4.0f * j)) * FMath::Cos(FMath::DegreesToRadians(Transform.GetRotation().Rotator().Yaw));

			FVector vector = FVector(Transform.GetLocation().X + xCoord, Transform.GetLocation().Y + yCoord, Transform.GetLocation().Z);

			locations.Add(vector);
		}
	}

	return locations;
}

void UCloudComponent::UpdateSpawnedClouds()
{
	for (FCloudStruct cloudStruct : Clouds) {
		if (cloudStruct.Precipitation == nullptr)
			continue;

		int32 spawnRate = 400.0f * cloudStruct.Precipitation->GetRelativeScale3D().X * cloudStruct.Precipitation->GetRelativeScale3D().Y;

		if (bSnow) {
			cloudStruct.Precipitation->SetVariableFloat(TEXT("SnowSpawnRate"), spawnRate);
			cloudStruct.Precipitation->SetVariableFloat(TEXT("RainSpawnRate"), 0.0f);
		}
		else {
			cloudStruct.Precipitation->SetVariableFloat(TEXT("SnowSpawnRate"), 0.0f);
			cloudStruct.Precipitation->SetVariableFloat(TEXT("RainSpawnRate"), spawnRate);
		}
	}
}

void UCloudComponent::RainCollisionHandler(FVector CollisionLocation)
{
	FHitResult hit(ForceInit);

	if (GetWorld()->SweepSingleByChannel(hit, CollisionLocation, FVector::Zero(), GetOwner()->GetActorQuat(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeBox(FVector(15.0f, 15.0f, 15.0f))))
	{
		if (hit.GetActor()->IsA<ABuilding>() || hit.GetActor()->IsA<AAI>() || hit.GetActor()->IsA<AEggBasket>())
			SetRainMaterialEffect(1.0f, hit.GetActor());
		else if (hit.Component != nullptr && hit.Component->IsA<UHierarchicalInstancedStaticMeshComponent>() && hit.Component != Cast<AGrid>(GetOwner())->HISMRiver)
			SetRainMaterialEffect(1.0f, nullptr, Cast<UHierarchicalInstancedStaticMeshComponent>(hit.Component), hit.Item);
	}
}

void UCloudComponent::SetRainMaterialEffect(float Value, AActor* Actor, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	if (Value == 1.0f) {
		ACamera* camera = Cast<AGrid>(GetOwner())->Camera;

		FTimerStruct timer;

		FString id = "Wet";

		if (Actor != nullptr)
			id += FString::FromInt(Actor->GetUniqueID());
		else
			id += FString::FromInt(HISM->GetUniqueID()) + FString::FromInt(Instance);

		timer.CreateTimer(id, GetOwner(), 30, FTimerDelegate::CreateUObject(this, &UCloudComponent::SetRainMaterialEffect, 0.0f, Actor, HISM, Instance), false);

		if (camera->CitizenManager->FindTimer(id, GetOwner()) != nullptr) {
			camera->CitizenManager->ResetTimer(id, GetOwner());

			return;
		}
		else {
			camera->CitizenManager->Timers.Add(timer);
		}
	}

	if (Actor != nullptr) {
		if (Actor->IsA<AAI>()) {
			SetMaterialWetness(Cast<AAI>(Actor)->Mesh->GetMaterial(0), Value, nullptr, Cast<AAI>(Actor)->Mesh);

			if (Actor->IsA<ACitizen>())
				SetMaterialWetness(Cast<ACitizen>(Actor)->HatMesh->GetMaterial(0), Value, Cast<ACitizen>(Actor)->HatMesh, nullptr);
		}
		else {
			UStaticMeshComponent* meshComp = nullptr;

			if (Actor->IsA<ABuilding>())
				meshComp = Cast<ABuilding>(Actor)->BuildingMesh;
			else
				meshComp = Cast<AEggBasket>(Actor)->BasketMesh;

			SetMaterialWetness(meshComp->GetMaterial(0), Value, meshComp, nullptr);
		}
	}
	else {
		HISM->SetCustomDataValue(Instance, 0, Value);
	}
}

void UCloudComponent::SetMaterialWetness(UMaterialInterface* MaterialInterface, float Value, UStaticMeshComponent* StaticMesh, USkeletalMeshComponent* SkeletalMesh)
{
	if (!IsValid(MaterialInterface))
		return;
	
	UMaterialInstanceDynamic* material = nullptr;

	if (MaterialInterface->IsA<UMaterialInstanceDynamic>())
		material = Cast<UMaterialInstanceDynamic>(MaterialInterface);
	else
		material = UMaterialInstanceDynamic::Create(MaterialInterface, this);

	material->SetScalarParameterValue("WetAlpha", Value);

	if (!MaterialInterface->IsA<UMaterialInstanceDynamic>()) {
		if (StaticMesh != nullptr)
			StaticMesh->SetMaterial(0, material);
		else
			SkeletalMesh->SetMaterial(0, material);
	}
}