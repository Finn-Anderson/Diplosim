#include "Clouds.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AtmosphereComponent.h"
#include "Map/Grid.h"
#include "Universal/DiplosimUserSettings.h"

UCloudComponent::UCloudComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Height = 4000.0f;
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

	for (int32 i = Clouds.Num() - 1; i > -1; i--) {
		FCloudStruct cloudStruct = Clouds[i];

		FVector location = cloudStruct.Cloud->GetRelativeLocation() + Cast<AGrid>(GetOwner())->AtmosphereComponent->WindRotation.Vector();

		cloudStruct.Cloud->SetRelativeLocation(location);

		double distance = FVector::Dist(location, Cast<AGrid>(GetOwner())->GetActorLocation());

		if (distance > cloudStruct.Distance) {
			cloudStruct.Cloud->Deactivate();

			Clouds.Remove(cloudStruct);
		}
	}
}

void UCloudComponent::Clear()
{
	for (FCloudStruct cloudStruct : Clouds)
		cloudStruct.Cloud->DestroyComponent();

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

	float spawnRate = 0.0f;

	transform.SetRotation((Cast<AGrid>(GetOwner())->AtmosphereComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

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
		cloud->SetVariableLinearColor(TEXT("Color"), FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));
		spawnRate = 400.0f * transform.GetScale3D().X * transform.GetScale3D().Y;
	}

	cloud->SetVariableFloat(TEXT("SpawnRate"), spawnRate);

	cloud->SetBoundsScale(3.0f);

	FCloudStruct cloudStruct;
	cloudStruct.Cloud = cloud;
	cloudStruct.Distance = FVector::Dist(spawnLoc, Cast<AGrid>(GetOwner())->GetActorLocation());

	Clouds.Add(cloudStruct);

	GetWorld()->GetTimerManager().SetTimer(CloudTimer, this, &UCloudComponent::ActivateCloud, 90.0f);
}