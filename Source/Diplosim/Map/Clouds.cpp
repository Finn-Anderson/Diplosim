#include "Map/Clouds.h"

#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"

#include "WindComponent.h"
#include "Grid.h"
#include "DiplosimUserSettings.h"

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

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Clouds = this;

	for (UNiagaraComponent* comp : cloudComps) {
		if (!settings->GetRenderClouds())
			comp->SetHiddenInGame(true);

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

		FVector location = cloud.CloudComponent->GetRelativeLocation() + Grid->WindComponent->WindRotation.Vector();

		cloud.CloudComponent->SetRelativeLocation(location);

		double distance = FVector::Dist(location, FVector(0.0f, 0.0f, 0.0f));

		if (distance > 20000.0f) {
			cloud.CloudComponent->Deactivate();

			if (distance > 40000.0f) {
				cloud.Active = false;

				Clouds[i] = cloud;
			}
		}
	}
}

void AClouds::GetCloudBounds(AGrid* GridPtr)
{
	Grid = GridPtr;

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

	float x = FMath::FRandRange(1.0f, 5.0f);
	float y = FMath::FRandRange(1.0f, 5.0f);
	float z = FMath::FRandRange(1.0f, 2.0f);
	transform.SetScale3D(FVector(x, y, z));

	float spawnRate = 0.0f;

	transform.SetRotation((Grid->WindComponent->WindRotation + FRotator(0.0f, 180.0f, 0.0f)).Quaternion());

	FVector spawnLoc = transform.GetRotation().Vector() * 20000.0f;
	spawnLoc.Z = Height + FMath::FRandRange(-200.0f, 200.0f);
	transform.SetLocation(spawnLoc);

	cloud.CloudComponent->SetWorldScale3D(transform.GetScale3D());
	cloud.CloudComponent->SetWorldLocation(transform.GetLocation());
	cloud.CloudComponent->SetWorldRotation(transform.GetRotation());

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