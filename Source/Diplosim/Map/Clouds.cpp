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
		UNiagaraComponent* cloud = Clouds[i];

		FVector location = cloud->GetRelativeLocation() + Grid->WindComponent->WindRotation.Vector();

		cloud->SetRelativeLocation(location);

		double distance = FVector::Dist(location, Grid->GetActorLocation());

		if (distance > 21000.0f) {
			cloud->Deactivate();

			Clouds.Remove(cloud);
		}
	}
}

void AClouds::Clear()
{
	for (UNiagaraComponent* cloud : Clouds)
		cloud->DestroyComponent();

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
		transform.SetLocation(spawnLoc);

		UNiagaraComponent* cloud = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CloudSystem, transform.GetLocation(), transform.GetRotation().Rotator(), transform.GetScale3D());

		int32 chance = FMath::RandRange(1, 100);

		if (chance > 75) {
			cloud->SetVariableLinearColor(TEXT("Color"), FLinearColor(0.1f, 0.1f, 0.1f));
			spawnRate = 400.0f * transform.GetScale3D().X * transform.GetScale3D().Y;
		}

		cloud->SetVariableFloat(TEXT("SpawnRate"), spawnRate);

		cloud->SetBoundsScale(3.0f);

		Clouds.Add(cloud);
	}

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ActivateCloud";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 90.0f, info);
}