#include "Map/WindComponent.h"

#include "Kismet/KismetSystemLibrary.h"

UWindComponent::UWindComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWindComponent::BeginPlay()
{
	Super::BeginPlay();

	ChangeWindDirection();
}

void UWindComponent::ChangeWindDirection()
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