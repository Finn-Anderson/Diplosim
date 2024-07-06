#include "EggBasket.h"

#include "Kismet/GameplayStatics.h"

#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Buildings/Building.h"

AEggBasket::AEggBasket()
{
	PrimaryActorTick.bCanEverTick = false;

	BasketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasketMesh"));
	BasketMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BasketMesh->SetCanEverAffectNavigation(true);
}

FRewardStruct AEggBasket::PickReward(ACamera* Camera)
{
	TArray<FRewardStruct> chosenRewards;

	for (FRewardStruct reward : Rewards) {
		TArray<TSubclassOf<ABuilding>> buildings = Camera->ResourceManager->GetBuildings(reward.Resource);

		for (TSubclassOf<ABuilding> building : buildings) {
			TArray<AActor*> actors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), building, actors);

			if (actors.IsEmpty())
				continue;

			chosenRewards.Add(reward);

			break;
		}
	}

	int32 index = FMath::RandRange(0, chosenRewards.Num() - 1);

	return chosenRewards[index];
}

void AEggBasket::RedeemReward()
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	FRewardStruct reward = PickReward(camera);

	int32 amount = FMath::RandRange(reward.Min, reward.Max);

	camera->ResourceManager->AddUniversalResource(reward.Resource, amount);
	
	Grid->ResourceTiles.Add(Tile);

	Destroy();
}