#include "Map/Clouds.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Map/Grid.h"

AClouds::AClouds()
{
	PrimaryActorTick.bCanEverTick = true;

	CloudComponent1 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent1"));
	CloudComponent1->SetCastShadow(true);
	CloudComponent1->bAutoActivate = false;

	CloudComponent2 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent2"));
	CloudComponent2->SetCastShadow(true);
	CloudComponent2->bAutoActivate = false;

	CloudComponent3 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent3"));
	CloudComponent3->SetCastShadow(true);
	CloudComponent3->bAutoActivate = false;

	CloudComponent4 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent4"));
	CloudComponent4->SetCastShadow(true);
	CloudComponent4->bAutoActivate = false;

	CloudComponent5 = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent5"));
	CloudComponent5->SetCastShadow(true);
	CloudComponent5->bAutoActivate = false;

	Height = 3000.0f;
}

void AClouds::BeginPlay()
{
	Super::BeginPlay();

	TArray<UNiagaraComponent*> cloudComps = { CloudComponent1, CloudComponent2, CloudComponent3, CloudComponent4, CloudComponent5 };

	for (UNiagaraComponent* comp : cloudComps) {
		FCloudStruct cloud;
		cloud.CloudComponent = comp;

		Clouds.Add(cloud);
	}

	SetActorLocation(FVector(0.0f, 0.0f, Height));
}

void AClouds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = 0; i < Clouds.Num(); i++) {
		FCloudStruct cloud = Clouds[i];

		if (!cloud.Active)
			continue;

		FVector location = cloud.CloudComponent->GetRelativeLocation() + FVector(0.0f, 1.0f, 0.0f);

		cloud.CloudComponent->SetRelativeLocation(location);

		if (location.Y >= XY - cloud.CloudComponent->GetRelativeScale3D().Y * 800.0f) {
			cloud.CloudComponent->Deactivate();

			if (location.Y >= XY * 2) {
				cloud.Active = false;

				Clouds[i] = cloud;
			}
		}
	}
}

void AClouds::GetCloudBounds(int32 GridSize)
{
	XY = GridSize;

	ActivateCloud();
}

void AClouds::ActivateCloud()
{
	TArray<FCloudStruct> validClouds;

	for (FCloudStruct cloud : Clouds) {
		if (cloud.Active)
			continue;

		validClouds.Add(cloud);
	}

	if (validClouds.IsEmpty())
		return; 
	
	int32 index = FMath::RandRange(0, validClouds.Num() - 1);
	FCloudStruct cloud = validClouds[index];

	FTransform transform;

	float x;
	float y;
	float z;

	x = FMath::FRandRange(1.0f, 5.0f);
	y = FMath::FRandRange(1.0f, 5.0f);
	z = FMath::FRandRange(1.0f, 2.0f);
	transform.SetScale3D(FVector(x, y, z));

	float spawnRate = 0.0f;

	x = FMath::FRandRange(-XY, XY);
	y = -XY * 2;
	z = Height + FMath::FRandRange(-200.0f, 200.0f);
	transform.SetLocation(FVector(x, y, z));

	cloud.CloudComponent->SetWorldScale3D(transform.GetScale3D());
	cloud.CloudComponent->SetWorldLocation(transform.GetLocation());

	int32 chance = FMath::RandRange(1, 100);

	if (chance > 75) {
		cloud.CloudComponent->SetVariableLinearColor(TEXT("Color"), FLinearColor(0.1f, 0.1f, 0.1f));
		spawnRate = 400.0f * transform.GetScale3D().X * transform.GetScale3D().Y;
	}

	cloud.CloudComponent->SetVariableFloat(TEXT("SpawnRate"), spawnRate);
	cloud.CloudComponent->BoundsScale = 10.0f;

	cloud.CloudComponent->Activate();
	cloud.Active = true;

	Clouds.Find(validClouds[index], index);

	Clouds[index] = cloud;

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ActivateCloud";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), 90.0f, info);
}