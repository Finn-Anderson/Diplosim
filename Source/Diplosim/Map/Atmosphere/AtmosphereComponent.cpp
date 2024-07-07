#include "AtmosphereComponent.h"
#include "Engine/DirectionalLight.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "Map/Grid.h"
#include "AI/Citizen.h"

UAtmosphereComponent::UAtmosphereComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(1.0f/24.0f);

	bNight = false;

	Speed = 0.01f;

	Day = 1;
}

void UAtmosphereComponent::BeginPlay()
{
	Super::BeginPlay();

	ChangeWindDirection();
}

void UAtmosphereComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Sun->AddActorLocalRotation(FRotator(0.0f, -Speed, 0.0f));

	Cast<AGrid>(GetOwner())->UpdateSkybox();

	float pitch = FMath::RoundHalfFromZero((Sun->GetActorRotation().Pitch * 100.0f)) / 100.0f;

	if (pitch == 0.0f) {
		TArray<AActor*> citizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

		bNight = !bNight;

		for (AActor* Actor : citizens)
			Cast<ACitizen>(Actor)->SetTorch();

		if (!bNight)
			Day++;
	}
}

void UAtmosphereComponent::ChangeWindDirection()
{
	float yaw = FMath::FRandRange(0.0f, 360.0f);

	WindRotation = FRotator(0.0f, yaw, 0.0f);

	int32 time = FMath::RandRange(180.0f, 600.0f);

	FLatentActionInfo info;
	info.Linkage = 0;
	info.CallbackTarget = this;
	info.ExecutionFunction = "ChangeWindDirection";
	info.UUID = GetUniqueID();
	UKismetSystemLibrary::Delay(GetWorld(), time, info);
}