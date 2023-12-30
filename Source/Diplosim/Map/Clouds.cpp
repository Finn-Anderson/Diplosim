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

	CloudComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CloudComponent"));
	CloudComponent->SetCastShadow(true);

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

	FVector location = CloudComponent->GetRelativeLocation() + FVector(0.0f, Speed, 0.0f);

	CloudComponent->SetRelativeLocation(location);

	if (location.Y >= Y + CloudComponent->GetRelativeScale3D().Y * 400.0f) {
		CloudComponent->Deactivate();

		if (location.Y >= Y + CloudComponent->GetRelativeScale3D().Y * 400.0f + 800.0f) {
			ResetCloud();
		}
	}
}

void AClouds::GetCloudBounds(AGrid* Grid)
{
	X, Y = Grid->Size * 100.0f / 2.0f;

	ResetCloud();
}

void AClouds::ResetCloud()
{
	float x;
	float y;
	float z;

	x = FMath::FRandRange(1.0f, 10.0f);
	y = FMath::FRandRange(1.0f, 10.0f);
	z = FMath::FRandRange(1.0f, 3.0f);
	CloudComponent->SetWorldScale3D(FVector(x, y, z));

	x = FMath::FRandRange(-X, X);
	y = -Y - 400.0f * CloudComponent->GetRelativeScale3D().Y;
	z = Height + FMath::FRandRange(-200.0f, 200.0f);
	CloudComponent->SetWorldScale3D(FVector(x, y, z));

	Speed = FMath::FRandRange(1.0f, 3.0f);

	CloudComponent->Activate();
}