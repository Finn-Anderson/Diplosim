#include "Map/Clouds.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

#include "Map/Grid.h"

AClouds::AClouds()
{
	PrimaryActorTick.bCanEverTick = true;

	Height = 2000.0f;
}

void AClouds::BeginPlay()
{
	Super::BeginPlay();

	SetActorLocation(FVector(0.0f, 0.0f, Height));
}

void AClouds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = (Clouds.Num() - 1); i > -1; i--) {
		FCloudStruct cloud = Clouds[i];

		FVector location = cloud.CloudComponent->GetRelativeLocation() + FVector(0.0f, cloud.Speed, 0.0f);

		cloud.CloudComponent->SetRelativeLocation(location);

		if (location.Y >= Y) {
			cloud.CloudComponent->Deactivate();

			Clouds.Remove(cloud);
		}
	}
}

void AClouds::GetCloudBounds(AGrid* Grid)
{
	FTransform extremeTransform;

	FTileStruct min = Grid->Storage[0];

	Grid->HISMWater->GetInstanceTransform(min.Instance, extremeTransform);
	X = FMath::Abs(extremeTransform.GetLocation().X);

	Grid->HISMWater->GetInstanceTransform(min.Instance, extremeTransform);
	Y = FMath::Abs(extremeTransform.GetLocation().Y) * 1.5;

	CloudSpawner();
}

void AClouds::CloudSpawner()
{
	FTransform transform;

	float x;
	float y;
	float z;

	x = FMath::FRandRange(-X, X);
	y = -Y;
	z = Height + FMath::FRandRange(-200.0f, 200.0f);
	FVector loc = FVector(x, y, z);
	transform.SetLocation(loc);

	x = FMath::FRandRange(1.0f, 5.0f);
	y = FMath::FRandRange(1.0f, 5.0f);
	z = FMath::FRandRange(1.5f, 2.0f);
	transform.SetScale3D(FVector(x, y, z));

	UNiagaraComponent* cloudComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CloudSystem, transform.GetLocation(), FRotator(0.0f), transform.GetScale3D());
	cloudComp->CastShadow = true;

	FCloudStruct cloudStruct;
	cloudStruct.Speed = FMath::FRandRange(1.0f, 3.0f);
	cloudStruct.CloudComponent = cloudComp;

	Clouds.Add(cloudStruct);

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "CloudSpawner";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 15.0f, info);
}