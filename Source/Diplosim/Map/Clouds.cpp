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

	Height = 3000.0f;
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

	for (UNiagaraComponent* cloud : Clouds) {
		FVector location = cloud->GetRelativeLocation() + Grid->WindComponent->WindRotation.Vector();

		cloud->SetRelativeLocation(location);

		double distance = FVector::Dist(location, FVector(0.0f, 0.0f, 0.0f));

		if (distance > 20000.0f)
			cloud->Deactivate();
	}
}

void AClouds::GetCloudBounds(AGrid* GridPtr)
{
	Grid = GridPtr;

	for (UNiagaraComponent* cloud : Clouds)
		cloud->DestroyComponent();

	Clouds.Empty();

	ActivateCloud();
}

void AClouds::ActivateCloud()
{
	if (Settings->GetRenderClouds()) {
		FTransform transform;

		float x = FMath::FRandRange(1.0f, 5.0f);
		float y = FMath::FRandRange(1.0f, 5.0f);
		float z = FMath::FRandRange(1.0f, 2.0f);
		transform.SetScale3D(FVector(x, y, z));

		float spawnRate = 0.0f;

		transform.SetRotation((Grid->WindComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

		FVector spawnLoc = transform.GetRotation().Vector() * 19750.0f;
		spawnLoc.Z = Height + FMath::FRandRange(-200.0f, 200.0f);
		transform.SetLocation(spawnLoc);

		UNiagaraComponent* cloud = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CloudSystem, transform.GetLocation(), transform.GetRotation().Rotator(), transform.GetScale3D());

		int32 chance = FMath::RandRange(1, 100);

		if (chance > 75) {
			cloud->SetVariableLinearColor(TEXT("Color"), FLinearColor(0.1f, 0.1f, 0.1f));
			spawnRate = 400.0f * transform.GetScale3D().X * transform.GetScale3D().Y;
		}

		cloud->SetVariableFloat(TEXT("SpawnRate"), spawnRate);

		Clouds.Add(cloud);
	}

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ActivateCloud";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 90.0f, info);
}