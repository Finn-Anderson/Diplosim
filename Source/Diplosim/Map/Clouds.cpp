#include "Map/Clouds.h"

#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "WindComponent.h"
#include "Grid.h"
#include "DiplosimUserSettings.h"

AClouds::AClouds()
{
	PrimaryActorTick.bCanEverTick = true;

	Height = 4000.0f;

	Grid = nullptr;
}

void AClouds::BeginPlay()
{
	Super::BeginPlay();

	Settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	Settings->Clouds = this;

	SetActorLocation(FVector(0.0f, 0.0f, Height));
}

void AClouds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = Clouds.Num() - 1; i > -1; i--) {
		FCloudStruct cloudStruct = Clouds[i];

		FVector location = cloudStruct.Cloud->GetRelativeLocation() + Grid->WindComponent->WindRotation.Vector();

		cloudStruct.Cloud->SetRelativeLocation(location);

		double distance = FVector::Dist(location, Grid->GetActorLocation());

		if (distance > cloudStruct.Distance) {
			cloudStruct.Cloud->Deactivate();

			Clouds.Remove(cloudStruct);
		}
	}
}

void AClouds::Clear()
{
	for (FCloudStruct cloudStruct : Clouds)
		cloudStruct.Cloud->DestroyComponent();

	Clouds.Empty();
}

void AClouds::ActivateCloud()
{
	if (Settings->GetRenderClouds()) {
		FTransform transform;

		float x = FMath::FRandRange(1.0f, 10.0f);
		float y = FMath::FRandRange(1.0f, 10.0f);
		float z = FMath::FRandRange(1.0f, 4.0f);
		transform.SetScale3D(FVector(x, y, z));

		float spawnRate = 0.0f;

		transform.SetRotation((Grid->WindComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

		FVector spawnLoc = transform.GetRotation().Vector() * 20000.0f;
		spawnLoc.Z = Height + FMath::FRandRange(-200.0f, 200.0f);

		FVector limit = (transform.GetRotation().Rotator() + FRotator(0.0f, 90.0f, 0.0f)).Vector();
		float vary = FMath::FRandRange(-20000.0f, 20000.0f);
		limit *= vary;

		spawnLoc += limit;

		transform.SetLocation(spawnLoc);

		UNiagaraComponent* cloud = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CloudSystem, transform.GetLocation(), transform.GetRotation().Rotator(), transform.GetScale3D());

		int32 chance = FMath::RandRange(1, 100);

		if (chance > 75) {
			cloud->SetVariableLinearColor(TEXT("Color"), FLinearColor(0.1f, 0.1f, 0.1f));
			spawnRate = 400.0f * transform.GetScale3D().X * transform.GetScale3D().Y;
		}

		cloud->SetVariableFloat(TEXT("SpawnRate"), spawnRate);

		cloud->SetBoundsScale(3.0f);

		FCloudStruct cloudStruct;
		cloudStruct.Cloud = cloud;
		cloudStruct.Distance = FVector::Dist(spawnLoc, Grid->GetActorLocation());

		Clouds.Add(cloudStruct);
	}

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ActivateCloud";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 90.0f, info);
}