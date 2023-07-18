#include "BuildComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Camera.h"
#include "Building.h"
#include "Grid.h"
#include "Tile.h"
#include "Ground.h"
#include "Resource.h"
#include "Water.h"

UBuildComponent::UBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	BlueprintMaterial = CreateDefaultSubobject<UMaterial>(TEXT("BlueprintMaterial"));

	GridStatus = true;
	Building = nullptr;

	IsBlocked = true;

	//Camera = Cast<ACamera>(GetOwner());
}

void UBuildComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
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

		if (Building == nullptr) {
			Building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, location, Rotation);

			OGMaterial = UMaterialInstanceDynamic::Create(Building->BuildingMesh->GetMaterial(0), NULL);
		}
		else {
			Building->SetActorLocation(location);
		}

		if (IsBlocked) {
			Building->BuildingMesh->SetMaterial(0, BlockedMaterial);
		}
		else {
			Building->BuildingMesh->SetMaterial(0, BlueprintMaterial);
		}
	}

	if (Building != nullptr) {
		FVector min = Building->GetActorLocation();

		FVector max;
		FVector boxExtent;
		Building->GetActorBounds(false, max, boxExtent);

		int32 x[2] = {min.X, max.X};
		int32 y[2] = {min.Y, max.Y};
		int32 z = min.Z;

		int32 count = 0;

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				FVector end = FVector(x[j], y[i], z - 10.0f);
				FVector corner = FVector(x[j], y[i], z);

				if (GetWorld()->LineTraceSingleByChannel(hit, corner, end, ECollisionChannel::ECC_GameTraceChannel1)) {
					AActor* actor = hit.GetActor();
					if (!actor->IsA<AGrid>()) {
						if (actor->IsA<AResource>() || (actor->IsA<ATile>() && !actor->IsA<AWater>())) {
							count += 1;
						}
					}
				}
				else {
					GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("Some debug message!"));

					return;
				}
			}
		}

		if (count == 4) {
			IsBlocked = false;
		}
		else {
			IsBlocked = true;
		}
	}
}

void UBuildComponent::SetGridStatus()
{
	GridStatus = !GridStatus;
}

void UBuildComponent::HideTree(AResource* tree)
{
	if (tree->GetClass() == TreeClass) {
		tree->SetActorHiddenInGame(!tree->IsHidden());

		if (tree->IsHidden()) {
			Trees.Add(tree);
		}
		else {
			Trees.Remove(tree);
		}
	}
}

void UBuildComponent::Build()
{
	SetComponentTickEnabled(!IsComponentTickEnabled());

	if (Building != nullptr) {
		Building->Destroy();

		Building = nullptr;
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
	if (Building == nullptr || IsBlocked || !Building->BuildCost())
		return;

	for (int i = 0; i < Trees.Num(); i++) {
		Trees[i]->Destroy();

		Trees.RemoveAt(i);
	}

	Building->BuildingMesh->SetMaterial(0, OGMaterial);

	Building->Blueprint = false;

	Building = nullptr;

	Build();

	if (Camera->start) {
		Camera->start = false;
	}
}