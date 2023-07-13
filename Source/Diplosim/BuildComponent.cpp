#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "Camera.h"
#include "Grid.h"
#include "Building.h"
#include "Tile.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	GridStatus = true;
	Building = nullptr;

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

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
	{
		ATile* tile = Cast<ATile>(hit.GetActor());

		if (tile->GetType() != EType::Water) {
			FVector location;

			if (GridStatus) {
				location = tile->GetActorLocation();

				FVector origin;
				FVector boxExtent;
				tile->GetActorBounds(false, origin, boxExtent);

				location.Z += boxExtent.Z + origin.Z;
			}
			else {
				location = hit.Location;
			}

			if (Building == nullptr) {
				Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);
			}
			else {
				Building->SetActorLocation(location);
			}

			Building->SetActorHiddenInGame(false);
		}
		else {
			Building->SetActorHiddenInGame(true);
		}
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

void UBuildComponent::Build()
{
	SetComponentTickEnabled(!IsComponentTickEnabled());

	if (Building != nullptr) {
		Building->SetActorHiddenInGame(IsComponentTickEnabled());
	}
}

void UBuildComponent::RotateBuilding()
{
	if (Building != nullptr) {
		int32 yaw = Rotation.Yaw;
		if (GridStatus) {
			if (yaw % 90 == 0) {
				yaw += 90;
			}
			else {
				yaw += 90 / 2;
				yaw -= yaw % 90;
			}
		}
		else {
			APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

			bool keyR = PController->IsInputKeyDown(EKeys::R);

			if (keyR) {
				yaw += 2;

				FTimerHandle timerHandle;
				GetWorld()->GetTimerManager().SetTimer(timerHandle, this, &UBuildComponent::RotateBuilding, 0.01f, false);
			}
		}

		if (yaw == 360) {
			yaw = 0;
		}

		Rotation.Yaw = yaw;

		Building->SetActorRotation(Rotation);
	}
}

void UBuildComponent::Place()
{
	if (Building != nullptr && !Building->IsHidden() && !Building->IsBlocked()) {
		Build();

		Building = nullptr;
	}
}