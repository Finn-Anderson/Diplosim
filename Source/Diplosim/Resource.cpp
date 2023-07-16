#include "Resource.h"

#include "Math/UnrealMathUtility.h"

#include "Grid.h"
#include "Tile.h"

AResource::AResource()
{
	PrimaryActorTick.bCanEverTick = false;

	if (ResourceMeshList.Num() > 0) {
		int32 num = FMath::RandRange(0, ResourceMeshList.Num() - 1);

		ResourceMesh = ResourceMeshList[num];
		ResourceMesh->bCastDynamicShadow = true;
		ResourceMesh->CastShadow = true;
	}

	Quantity = 100000;

	MaxYield = 5;

	isRock = true;
}

int32 AResource::GetYield()
{
	Yield = GenerateYield();

	YieldStatus();

	return Yield;
}

int32 AResource::GenerateYield()
{
	int32 yield = FMath::RandRange(1, MaxYield);

	return yield;
}

void AResource::YieldStatus() 
{
	if (isRock) {
		Quantity -= Yield;

		if (Quantity <= 0) {
			[&] {
				for (int32 y = 0; y < Grid->Size; y++) {
					for (int32 x = 0; x < Grid->Size; x++) {
						if (Grid->Storage[x][y] == this) {
							Destroy();

							Grid->GenerateTile(Grid->Hill, 0, x, y);

							return;
						}
					}
				}
			}();

			Destroy();
		}
	}
	else {
		SetActorHiddenInGame(true);

		FTimerHandle growTimer;
		GetWorldTimerManager().SetTimer(growTimer, this, &AResource::Grow, 300.0f, true);
	}
}

void AResource::SetQuantity(int32 Value)
{
	Quantity = Value;
}

void AResource::Grow()
{
	SetActorHiddenInGame(false);
}