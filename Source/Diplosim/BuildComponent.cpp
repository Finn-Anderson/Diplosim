#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Camera.h"
#include "Grid.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	GridStatus = true;

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(Camera);
	queryParams.AddIgnoredActor(Camera->Grid);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECC_Visibility, queryParams))
	{
		Actor = hit.GetActor();
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

void UBuildComponent::Build()
{
	SetComponentTickEnabled(!IsComponentTickEnabled());
}

void UBuildComponent::Place()
{
	if (!Actor->IsA<ABuilding>()) {
		FVector location = Actor->GetActorLocation();

		FVector origin;
		FVector boxExtent;
		Actor->GetActorBounds(false, origin, boxExtent);

		location.Z += (boxExtent.Z / 2);
	}
}