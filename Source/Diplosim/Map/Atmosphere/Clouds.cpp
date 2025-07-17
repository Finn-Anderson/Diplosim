#include "Clouds.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeTryLock.h"

#include "AtmosphereComponent.h"
#include "NaturalDisasterComponent.h"
#include "Map/Grid.h"
#include "Universal/DiplosimUserSettings.h"
#include "Buildings/Building.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "Universal/EggBasket.h"

UCloudComponent::UCloudComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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

void UCloudComponent::TickCloud(float DeltaTime)
{
	for (int32 i = Clouds.Num() - 1; i > -1; i--) {
		FCloudStruct cloudStruct = Clouds[i];

		FVector location = cloudStruct.HISMCloud->GetRelativeLocation() + Grid->AtmosphereComponent->WindRotation.Vector() * 100.0f * DeltaTime;

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
			material->SetScalarParameterValue("Opacity", FMath::Clamp(opacity + DeltaTime, 0.0f, 1.0f));
		else if (cloudStruct.bHide)
			material->SetScalarParameterValue("Opacity", FMath::Clamp(opacity - DeltaTime, 0.0f, 1.0f));

		double distance = FVector::Dist(location, Grid->GetActorLocation());

		if (distance > cloudStruct.Distance) {
			if (cloudStruct.Precipitation != nullptr)
				cloudStruct.Precipitation->Deactivate();

			Clouds[i].bHide = true;

			if (opacity == 0.0f) {
				cloudStruct.HISMCloud->DestroyComponent();

				Clouds.Remove(cloudStruct);
			}
		}
		else if (cloudStruct.lightningFrequency != 0.0f) {
			cloudStruct.lightningTimer += DeltaTime;

			if (cloudStruct.lightningTimer > cloudStruct.lightningFrequency) {
				int32 inst = Grid->Stream.RandRange(0, cloudStruct.HISMCloud->GetInstanceCount() - 1);
				
				FTransform transform;
				cloudStruct.HISMCloud->GetInstanceTransform(inst, transform);

				auto bound = FMath::FloorToInt32(FMath::Sqrt((double)Grid->Size));

				int32 x = Grid->Stream.RandRange(-3, 3) + transform.GetLocation().X / 100.0f + bound / 2;
				int32 y = Grid->Stream.RandRange(-3, 3) + transform.GetLocation().X / 100.0f + bound / 2;

				FVector endLocation = FVector(transform.GetLocation().X + Grid->Stream.RandRange(-300.0f, 300.0f), transform.GetLocation().Y + Grid->Stream.RandRange(-300.0f, 300.0f), 0.0f);

				if (x < bound && y < bound)
					endLocation = Grid->GetTransform(&Grid->Storage[x][y]).GetLocation() + FVector(Grid->Stream.RandRange(-50.0f, 50.0f), Grid->Stream.RandRange(-50.0f, 50.0f), 0.0f);

				UNiagaraComponent* lightning = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LightningSystem, transform.GetLocation(), FRotator(0.0f), FVector(1.0f), true, false);
				lightning->SetVariableVec3(TEXT("StartLocation"), cloudStruct.HISMCloud->GetRelativeLocation() + transform.GetLocation());
				lightning->SetVariableVec3(TEXT("EndLocation"), endLocation);
				lightning->Activate();

				TArray<FHitResult> hits;

				FCollisionQueryParams params;
				params.AddIgnoredActor(Grid);

				if (GetWorld()->SweepMultiByChannel(hits, endLocation, endLocation + FVector(0.0f, 0.0f, 300.0f), FQuat(0.0f), ECC_Visibility, FCollisionShape::MakeBox(FVector(50.0f, 50.0f, 100.0f)), params)) {
					for (FHitResult hit : hits) {
						if (Grid->AtmosphereComponent->NaturalDisasterComponent->IsProtected(hit.GetActor()->GetActorLocation()))
							continue;

						Grid->AtmosphereComponent->SetOnFire(hit.GetActor(), hit.Item);
					}
				}

				cloudStruct.lightningTimer = 0.0f;
			}
		}
	}

	Async(EAsyncExecution::Thread, [this, DeltaTime]() {
		FScopeTryLock lock(&RainLock);
		if (!lock.IsLocked())
			return;

		for (int32 i = ProcessRainEffect.Num() - 1; i > -1; i--) {
			SetRainMaterialEffect(ProcessRainEffect[i].Value, ProcessRainEffect[i].Actor, ProcessRainEffect[i].HISM, ProcessRainEffect[i].Instance);

			ProcessRainEffect.RemoveAt(i);
		}

		SetGradualWetness();
	});
}

void UCloudComponent::Clear()
{
	for (FCloudStruct cloudStruct : Clouds) {
		if (cloudStruct.Precipitation != nullptr)
			cloudStruct.Precipitation->DestroyComponent();

		cloudStruct.HISMCloud->DestroyComponent();
	}

	Clouds.Empty();

	Grid->Camera->CitizenManager->RemoveTimer("Cloud", Grid);
}

void UCloudComponent::StartCloudTimer()
{
	Grid->Camera->CitizenManager->CreateTimer("Cloud", Grid, 90.0f, FTimerDelegate::CreateUObject(this, &UCloudComponent::ActivateCloud), true, true);
}

void UCloudComponent::ActivateCloud()
{
	if (!Settings->GetRenderClouds())
		return;

	FTransform transform;

	float x = Grid->Stream.FRandRange(1.0f, 10.0f);
	float y = Grid->Stream.FRandRange(1.0f, 10.0f);
	float z = Grid->Stream.FRandRange(1.0f, 4.0f);
	transform.SetScale3D(FVector(x, y, z));

	transform.SetRotation((Grid->AtmosphereComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

	FVector spawnLoc = transform.GetRotation().Vector() * 20000.0f;
	spawnLoc.Z = Height + Grid->Stream.FRandRange(-200.0f, 200.0f);

	FVector limit = (transform.GetRotation().Rotator() + FRotator(0.0f, 90.0f, 0.0f)).Vector();
	float vary = Grid->Stream.FRandRange(-20000.0f, 20000.0f);
	limit *= vary;

	spawnLoc += limit;

	transform.SetLocation(spawnLoc);

	int32 chance = Grid->Stream.RandRange(1, 100);

	if (!Settings->GetRain())
		chance = 1;

	chance = 100;

	FCloudStruct cloudStruct = CreateCloud(transform, chance);
	cloudStruct.Distance = FVector::Dist(spawnLoc, Grid->GetActorLocation());

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(cloudStruct.HISMCloud->GetMaterial(0), this);
	material->SetScalarParameterValue("Opacity", 0.0f);

	if (chance > 75)
		material->SetVectorParameterValue("Colour", FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));

	cloudStruct.HISMCloud->SetMaterial(0, material);

	Clouds.Add(cloudStruct); 
}

FCloudStruct UCloudComponent::CreateCloud(FTransform Transform, int32 Chance)
{
	UHierarchicalInstancedStaticMeshComponent* cloud = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass());
	cloud->SetStaticMesh(CloudMesh);
	cloud->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	cloud->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	cloud->SetAffectDistanceFieldLighting(false);
	cloud->SetRelativeLocation(Transform.GetLocation());
	cloud->SetRelativeRotation(Transform.GetRotation());
	cloud->SetupAttachment(Grid->GetRootComponent());
	cloud->PrimaryComponentTick.bCanEverTick = false;
	cloud->RegisterComponent();

	FCloudStruct cloudStruct;
	cloudStruct.HISMCloud = cloud;

	TArray<FVector> locations;

	for (int32 i = 0; i < 200; i++) {
		FTransform t = Transform;

		float x = Grid->Stream.FRandRange(-800.0f, 800.0f) * t.GetScale3D().X;
		float y = Grid->Stream.FRandRange(-800.0f, 800.0f) * t.GetScale3D().Y;
		float z = Grid->Stream.FRandRange(-256.0f, 256.0f) * t.GetScale3D().Z;
		t.SetLocation(FVector(x, y, z));

		float diff = Grid->Stream.FRandRange(20.0f, 40.0f);
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
	precipitation->SetVariableObject(TEXT("Callback"), Grid);
	precipitation->SetVariableVec3(TEXT("CloudLocation"), Transform.GetLocation());

	float spawnRate = 0.0f;

	float gravity = -980.0f;
	float lifetime = (cloud->GetRelativeLocation().Z - Height + 200.0f) / 800.0f + 3.0f;

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(precipitation, TEXT("SpawnLocations"), locations);
	spawnRate = 400.0f * Transform.GetScale3D().X * Transform.GetScale3D().Y * Grid->Stream.FRandRange(0.5f, 1.5f);

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

	if (NaturalDisasterComponent->ShouldCreateDisaster()) {
		cloudStruct.lightningFrequency = Grid->Stream.FRandRange(0.5f, 10.0f) / NaturalDisasterComponent->Intensity;

		NaturalDisasterComponent->ResetDisasterChance();
	}

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
	if (CollisionLocation.Z < 0.0f)
		return;
	
	FHitResult hit(ForceInit);

	if (GetWorld()->LineTraceSingleByChannel(hit, CollisionLocation, CollisionLocation - FVector(0.0f, 0.0f, 100.0f), ECollisionChannel::ECC_Visibility))
	{
		FWetnessStruct rainDrop;
		rainDrop.Value = 1.0f;

		if (hit.GetActor()->IsA<ABuilding>() || hit.GetActor()->IsA<AAI>() || hit.GetActor()->IsA<AEggBasket>()) {
			rainDrop.Actor = hit.GetActor();
		}
		else if (hit.Component != nullptr && hit.Component->IsA<UHierarchicalInstancedStaticMeshComponent>() && hit.Component != Grid->HISMRiver) {
			rainDrop.HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.Component);
			rainDrop.Instance = hit.Item;
		}

		ProcessRainEffect.Add(rainDrop);
	}
}

void UCloudComponent::SetRainMaterialEffect(float Value, AActor* Actor, UHierarchicalInstancedStaticMeshComponent* HISM, int32 Instance)
{
	if ((!IsValid(Actor) && !IsValid(HISM)))
		return;

	if (Value == 1.0f) {
		FString id = "Wet";

		if (IsValid(Actor))
			id += FString::FromInt(Actor->GetUniqueID());
		else
			id += FString::FromInt(HISM->GetUniqueID()) + FString::FromInt(Instance);

		if (Grid->Camera->CitizenManager->FindTimer(id, Grid) != nullptr) {
			Grid->Camera->CitizenManager->ResetTimer(id, Grid);

			return;
		}
		else {
			Grid->Camera->CitizenManager->CreateTimer(id, Grid, 30, FTimerDelegate::CreateUObject(this, &UCloudComponent::SetRainMaterialEffect, 0.0f, Actor, HISM, Instance), false, true);
		}
	}

	float increment = 0.02;

	if (Value == 0.0f)
		increment *= -1;

	Value = (Value - 1.0f) * -1.0f;

	FWetnessStruct wetness;
	wetness.Create(Value, Actor, HISM, Instance, increment);

	WetnessStruct.Add(wetness);
}

void UCloudComponent::SetGradualWetness()
{
	for (int32 i = (WetnessStruct.Num() - 1); i > -1; i--) {
		WetnessStruct[i].Value += WetnessStruct[i].Increment;

		if (WetnessStruct[i].Actor != nullptr) {
			if (WetnessStruct[i].Actor->IsA<AAI>()) {
				Cast<AAI>(WetnessStruct[i].Actor)->Mesh->SetCustomPrimitiveDataFloat(0, WetnessStruct[i].Value);

				if (WetnessStruct[i].Actor->IsA<ACitizen>() && IsValid(Cast<ACitizen>(WetnessStruct[i].Actor)->HatMesh))
					Cast<ACitizen>(WetnessStruct[i].Actor)->HatMesh->SetCustomPrimitiveDataFloat(0, WetnessStruct[i].Value);
			}
			else {
				UStaticMeshComponent* meshComp = nullptr;

				if (WetnessStruct[i].Actor->IsA<ABuilding>())
					meshComp = Cast<ABuilding>(WetnessStruct[i].Actor)->BuildingMesh;
				else
					meshComp = Cast<AEggBasket>(WetnessStruct[i].Actor)->BasketMesh;

				meshComp->SetCustomPrimitiveDataFloat(0, WetnessStruct[i].Value);
			}
		}
		else {
			WetnessStruct[i].HISM->SetCustomDataValue(WetnessStruct[i].Instance, 0, WetnessStruct[i].Value);
		}

		if (WetnessStruct[i].Value <= 0.0f || WetnessStruct[i].Value >= 1.0f)
			WetnessStruct.RemoveAt(i);
	}
}