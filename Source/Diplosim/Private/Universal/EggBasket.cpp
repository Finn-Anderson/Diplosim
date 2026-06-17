#include "Universal/EggBasket.h"

#include "Kismet/GameplayStatics.h"

#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Buildings/Work/Service/Stockpile.h"
#include "Universal/Resource.h"

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
	BasketMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);
	BasketMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BasketMesh->SetCanEverAffectNavigation(true);

	RootComponent = BasketMesh;
}

TSubclassOf<AResource> AEggBasket::PickReward(ACamera* Camera)
{
	TArray<TSubclassOf<AResource>> chosenRewards;

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		TMap<TSubclassOf<ABuilding>, int32> buildings = Camera->ResourceManager->GetBuildings(resource.Type);

		for (auto& element : buildings) {
			TArray<ABuilding*> foundBuildings = Camera->ResourceManager->GetBuildingsOfClass(Camera->ConquestManager->GetFaction(Camera->ColonyName), element.Key);

			if (foundBuildings.IsEmpty())
				continue;

			for (ABuilding* building : foundBuildings) {
				if (building->IsCapacityFull() || (building->IsA<AStockpile>() && !Cast<AStockpile>(building)->DoesStoreResource(resource.Type)))
					continue;

				chosenRewards.Add(resource.Type);

				break;
			}

			break;
		}
	}

	if (chosenRewards.IsEmpty())
		return nullptr;

	int32 index = Camera->Stream.RandRange(0, chosenRewards.Num() - 1);

	return chosenRewards[index];
}

void AEggBasket::RedeemReward()
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TSubclassOf<AResource> reward = PickReward(camera);

	if (reward != nullptr) {
		int32 amount = camera->Stream.RandRange(10, 100);

		camera->ResourceManager->AddUniversalResource(camera->ConquestManager->GetFaction(camera->ColonyName), reward, amount);
	}

	Destroy();
}