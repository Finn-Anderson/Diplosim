#include "Map/Clouds.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Map/Grid.h"

AClouds::AClouds()
{
	PrimaryActorTick.bCanEverTick = true;

	HISMClouds = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClouds"));
	HISMClouds->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMClouds->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	HISMClouds->SetCanEverAffectNavigation(false);
	HISMClouds->NumCustomDataFloats = 2;

	height = 800.0f;
}

void AClouds::BeginPlay()
{
	Super::BeginPlay();

	SetActorLocation(FVector(0.0f, 0.0f, height));
}

void AClouds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = 0; i < HISMClouds->GetInstanceCount(); i++) {
		FTransform transform;

		HISMClouds->GetInstanceTransform(i, transform);

		transform.SetLocation(transform.GetLocation() + FVector(0.0f, HISMClouds->PerInstanceSMCustomData[i * 2], 0.0f));
		HISMClouds->UpdateInstanceTransform(i, transform, false);

		float opacity = 0.5f;
		if (transform.GetLocation().Y >= Y) {
			opacity = HISMClouds->PerInstanceSMCustomData[i * 2 + 1] - 0.01f;

			if (HISMClouds->PerInstanceSMCustomData[i * 2 + 1] <= 0.0f) {
				HISMClouds->RemoveInstance(i);

				continue;
			}
		} else if (HISMClouds->PerInstanceSMCustomData[i * 2 + 1] < 0.5f) {
			opacity = HISMClouds->PerInstanceSMCustomData[i * 2 + 1] + 0.01f;
		}
		HISMClouds->SetCustomDataValue(i, 1, opacity);
	}
}

void AClouds::GetCloudBounds(AGrid* Grid)
{
	FTransform extremeTransform;

	FTileStruct min = Grid->Storage[0];

	Grid->HISMWater->GetInstanceTransform(min.Instance, extremeTransform);
	X = FMath::Abs(extremeTransform.GetLocation().X) / 2;

	Grid->HISMWater->GetInstanceTransform(min.Instance, extremeTransform);
	Y = FMath::Abs(extremeTransform.GetLocation().Y);

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
	z = GetActorLocation().Z + FMath::FRandRange(-200.0f, 200.0f);
	FVector loc = FVector(x, y, z);
	transform.SetLocation(loc);

	x = FMath::FRandRange(1.0f, 5.0f);
	y = FMath::FRandRange(1.0f, 5.0f);
	z = FMath::FRandRange(1.5f, 2.0f);
	transform.SetScale3D(FVector(x, y, z));

	int32 inst = HISMClouds->AddInstance(transform);

	float time = FMath::FRandRange(1.0f, 3.0f);
	HISMClouds->SetCustomDataValue(inst, 0, time);

	HISMClouds->SetCustomDataValue(inst, 1, 0.0f);

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "CloudSpawner";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 15.0f, info);
}