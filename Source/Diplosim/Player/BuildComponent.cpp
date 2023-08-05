#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

#include "Camera.h"
#include "Buildings/Building.h"
#include "Map/Grid.h"
#include "Map/Water.h"
#include "Map/Vegetation.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	GridStatus = true;
	Building = nullptr;

	IsBlocked = true;
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (BuildingClass != nullptr) {
		FVector mouseLoc, mouseDirection;
		APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

		FHitResult hit(ForceInit);

		FVector endTrace = mouseLoc + (mouseDirection * 10000);

		if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_GameTraceChannel1))
		{
			AActor* tile = hit.GetActor();

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

			if (Building == nullptr || Building->GetClass() != BuildingClass) {
				Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);
			}
			else {
				Building->SetActorLocation(location);
			}
		}

		if (Building != nullptr) {
			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(Building);
			queryParams.AddIgnoredActor(Camera->Grid);

			FVector origin;
			FVector boxExtent;
			Building->GetActorBounds(false, origin, boxExtent);

			FVector min = (origin - boxExtent) + FVector(1.0f, 1.0f, 0.0f);
			FVector max = (origin + boxExtent) - FVector(1.0f, 1.0f, 0.0f);

			int32 x[2] = { min.X, max.X };
			int32 y[2] = { min.Y, max.Y };
			int32 z = min.Z;

			int32 count = 0;

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 2; j++) {
					FVector end = FVector(x[j], y[i], z - 10.0f);
					FVector corner = FVector(x[j], y[i], z + 10.0f);

					if (GetWorld()->LineTraceSingleByChannel(hit, corner, end, ECC_Visibility, queryParams)) {
						AActor* actor = hit.GetActor();

						if (!actor->IsA<AWater>() && !actor->IsA<ABuilding>()) {
							count += 1;
						}
					}
				}
			}

			if (count == 4) {
				IsBlocked = false;
			}
			else {
				IsBlocked = true;
			}

			if (IsBlocked || !Building->CheckBuildCost()) {
				Building->BuildingMesh->SetOverlayMaterial(BlockedMaterial);
			}
			else {
				Building->BuildingMesh->SetOverlayMaterial(BlueprintMaterial);
			}
		}
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

void UBuildComponent::HideTree(class AVegetation* vegetation)
{
	vegetation->SetActorHiddenInGame(!vegetation->IsHidden());

	if (vegetation->IsHidden()) {
		Vegetation.Add(vegetation);
	}
	else {
		Vegetation.Remove(vegetation);
	}
}

void UBuildComponent::Build()
{
	SetComponentTickEnabled(!IsComponentTickEnabled());

	if (Building != nullptr) {
		Building->Destroy();

		Building = nullptr;
		BuildingClass = nullptr;
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
	if (Building == nullptr || IsBlocked || !Building->CheckBuildCost())
		return;

	for (int i = 0; i < Vegetation.Num(); i++) {
		Vegetation[i]->Destroy(true);
	}

	Vegetation.Empty();

	Building->BuildingMesh->SetOverlayMaterial(nullptr);

	Building->Build();

	Building = nullptr;

	BuildingClass = nullptr;

	Build();

	if (Camera->start) {
		Camera->start = false;
		Camera->BuildUIInstance->AddToViewport();
	}
}