#include "Map/Clouds.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Map/Grid.h"

AClouds::AClouds()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTickableWhenPaused(true);

	HISMClouds = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMClouds"));
	HISMClouds->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	HISMClouds->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	HISMClouds->SetCanEverAffectNavigation(false);
	HISMClouds->NumCustomDataFloats = 2;
}

void AClouds::BeginPlay()
{
	Super::BeginPlay();

	GetCloudBounds();

	CloudSpawner();
}

void AClouds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = 0; i < HISMClouds->GetInstanceCount(); i++) {
		FTransform transform;

		HISMClouds->GetInstanceTransform(i, transform);

		transform.SetLocation(transform.GetLocation() + FVector(0.0f, HISMClouds->PerInstanceSMCustomData[i * 2], 0.0f));
		HISMClouds->UpdateInstanceTransform(i, transform, false);

		float opacity = 1.0f;
		if (transform.GetLocation().Y >= Y) {
			opacity = HISMClouds->PerInstanceSMCustomData[i * 2 + 1] - 0.01f;

			if (HISMClouds->PerInstanceSMCustomData[i * 2 + 1] <= 0.0f) {
				HISMClouds->RemoveInstance(i);

				continue;
			}
		} else if (HISMClouds->PerInstanceSMCustomData[i * 2 + 1] < 1.0f) {
			opacity = HISMClouds->PerInstanceSMCustomData[i * 2 + 1] + 0.01f;
		}
		HISMClouds->SetCustomDataValue(i, 1, opacity);
	}
}

void AClouds::GetCloudBounds()
{
	TArray<AActor*> foundGrids;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), GridClass, foundGrids);

	AGrid* grid = Cast<AGrid>(foundGrids[0]);

	FTransform minTransform;
	FTransform maxTransform;

	FTileStruct min = grid->Storage[0];
	FTileStruct max = grid->Storage[(grid->Size - 1) * (grid->Size - 1)];

	grid->HISMWater->GetInstanceTransform(min.Instance, minTransform);
	grid->HISMWater->GetInstanceTransform(max.Instance, maxTransform);
	X = FMath::Abs(minTransform.GetLocation().X) + FMath::Abs(maxTransform.GetLocation().X) / 2;

	grid->HISMWater->GetInstanceTransform(min.Instance, minTransform);
	grid->HISMWater->GetInstanceTransform(max.Instance, maxTransform);
	Y = FMath::Abs(minTransform.GetLocation().Y) + FMath::Abs(maxTransform.GetLocation().Y) / 2;
}

void AClouds::CloudSpawner()
{
	int32 chance = FMath::RandRange(1, 30);

	if (chance > 15) {
		FTransform transform;

		float x;
		float y;
		float z;

		x = FMath::FRandRange(-X, X);
		y = -Y;
		z = GetActorLocation().Z + FMath::FRandRange(-200.0f, 200.0f);
		FVector loc = FVector(x, y, z);
		transform.SetLocation(loc);

		x = FMath::FRandRange(1.0f, 20.0f);
		y = FMath::FRandRange(1.0f, 20.0f);
		z = 1.0f;
		transform.SetScale3D(FVector(x, y, z));

		int32 inst = HISMClouds->AddInstance(transform);

		float time = FMath::FRandRange(1.0f, 3.0f);
		HISMClouds->SetCustomDataValue(inst, 0, time);

		HISMClouds->SetCustomDataValue(inst, 1, 0.0f);
	}

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "CloudSpawner";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 1.0f, info);
}