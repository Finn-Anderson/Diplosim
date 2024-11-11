#include "Buildings/Work/Service/Trader.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/DiplosimAIController.h"

ATrader::ATrader()
{
	bAuto = false;
}

void ATrader::BeginPlay()
{
	Super::BeginPlay();

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		if (resource.Type == Money)
			continue;

		FMinStruct minStruct;
		minStruct.Resource = resource.Type;

		AutoMinCap.Add(minStruct);
	}
}

void ATrader::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen))
		return;

	AutoGenerateOrder();

	if (Orders.IsEmpty())
		return;

	if (Orders[0].bCancelled)
		ReturnResource(Citizen);
	else if (CheckStored(Citizen, Orders[0].SellingItems)) {
		if (Orders[0].Wait == 0)
			SubmitOrder(Citizen);
		else {
			FTimerStruct timer;
			timer.CreateTimer("Order", this, Orders[0].Wait, FTimerDelegate::CreateUObject(this, &ATrader::SubmitOrder, Citizen), false);

			Camera->CitizenManager->Timers.Add(timer);
		}
	}
}

void ATrader::SubmitOrder(class ACitizen* Citizen)
{
	if (GetCitizensAtBuilding().IsEmpty())
		return;

	ParticleComponent->Activate();

	UGameplayStatics::PlayWorldCameraShake(GetWorld(), CannonShake, GetActorLocation(), 0.0f, 1000.0f, 1.0f);

	int32 money = 0;

	UResourceManager* rm = Camera->ResourceManager;

	for (FItemStruct item : Orders[0].SellingItems) {
		FResourceStruct resource;
		resource.Type = item.Resource;

		int32 index = rm->ResourceList.Find(resource);

		rm->ResourceList[index].Stored += item.Amount;

		money += rm->ResourceList[index].Value * item.Amount;
	}

	for (FItemStruct item : Orders[0].BuyingItems) {
		FResourceStruct resource;
		resource.Type = item.Resource;

		int32 index = rm->ResourceList.Find(resource);

		rm->AddUniversalResource(item.Resource, item.Amount);

		rm->ResourceList[index].Stored -= item.Amount;

		money -= rm->ResourceList[index].Value * item.Amount;
	}

	if (money < 0)
		rm->TakeUniversalResource(Money, -money, -1000000);
	else
		rm->AddUniversalResource(Money, money);

	Orders[0].OrderWidget->RemoveFromParent();

	bool bRefresh = false;

	if (Orders[0].bRepeat && !Orders[0].bCancelled) {
		for (int32 i = 0; i < Orders[0].SellingItems.Num(); i++)
			Orders[0].SellingItems[i].Stored = 0;

		SetNewOrder(Orders[0]);

		if (Camera->WidgetComponent->IsAttachedTo(GetRootComponent()))
			bRefresh = true;
	}

	Orders.RemoveAt(0);

	if (bRefresh)
		Camera->DisplayInteract(this);

	if (Orders.Num() > 0) {
		if (Orders[0].bCancelled)
			ReturnResource(Citizen);
		else
			CheckStored(Citizen, Orders[0].SellingItems);
	}
	else
		AutoGenerateOrder();
}

void ATrader::ReturnResource(class ACitizen* Citizen)
{
	for (int32 i = 0; i < Orders[0].SellingItems.Num(); i++) {
		if (Orders[0].SellingItems[i].Stored == 0)
			continue;

		TArray<TSubclassOf<class ABuilding>> buildings = Camera->ResourceManager->GetBuildings(Orders[0].SellingItems[i].Resource);

		ABuilding* target = nullptr;

		int32 amount = FMath::Clamp(Orders[0].SellingItems[i].Stored, 0, 10);

		for (int32 j = 0; j < buildings.Num(); j++) {
			TArray<AActor*> foundBuildings;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), buildings[j], foundBuildings);

			for (int32 k = 0; k < foundBuildings.Num(); k++) {
				ABuilding* building = Cast<ABuilding>(foundBuildings[k]);

				FItemStruct itemStruct;
				itemStruct.Resource = Orders[0].SellingItems[i].Resource;

				int32 index = building->Storage.Find(itemStruct);

				if (building->Storage[index].Amount + amount > building->StorageCap)
					continue;

				if (target == nullptr) {
					target = building;

					continue;
				}

				double magnitude = Citizen->AIController->GetClosestActor(Citizen->GetActorLocation(), target->GetActorLocation(), building->GetActorLocation());

				if (magnitude <= 0.0f)
					continue;

				target = building;
			}
		}

		Orders[0].SellingItems[i].Stored -= amount;

		if (target != nullptr) {
			Citizen->Carry(Orders[0].SellingItems[i].Resource->GetDefaultObject<AResource>(), amount, target);

			return;
		}
		else
			Orders[0].SellingItems[i].Stored = 0;
	}

	Orders[0].OrderWidget->RemoveFromParent();

	Orders.RemoveAt(0);

	if (Orders.Num() > 0) {
		if (Orders[0].bCancelled)
			ReturnResource(Citizen);
		else
			CheckStored(Citizen, Orders[0].SellingItems);
	}
}

void ATrader::SetNewOrder(FQueueStruct Order)
{
	Orders.Add(Order);

	UResourceManager* rm = Camera->ResourceManager;

	for (FItemStruct items : Order.SellingItems)
		rm->AddCommittedResource(items.Resource, items.Amount);

	if (Orders.Num() != 1)
		return;

	TArray<ACitizen*> citizens = GetCitizensAtBuilding();
	bool bInstantOrder = false;

	for (ACitizen* citizen : citizens)
		bInstantOrder = CheckStored(citizen, Orders[0].SellingItems);

	if (bInstantOrder)
		SubmitOrder(GetOccupied()[0]);
}

void ATrader::SetOrderWidget(int32 index, UWidget* Widget)
{
	Orders[index].OrderWidget = Widget;
}

void ATrader::SetOrderCancelled(int32 index, bool bCancel)
{
	Orders[index].bCancelled = bCancel;

	UResourceManager* rm = Camera->ResourceManager;

	for (FItemStruct items : Orders[index].SellingItems)
		rm->TakeCommittedResource(items.Resource, items.Amount - items.Stored);

	for (ACitizen* Citizen : GetOccupied())
		Citizen->AIController->AIMoveTo(this);
}

void ATrader::SetAutoMode()
{
	bAuto = !bAuto;
}

bool ATrader::GetAutoMode()
{
	return bAuto;
}

void ATrader::SetMinCapPerResource(TArray<FMinStruct> MinCap)
{
	AutoMinCap = MinCap;
}

void ATrader::AutoGenerateOrder()
{
	if (!Orders.IsEmpty() || !bAuto)
		return;

	FQueueStruct order;

	for (FMinStruct minStruct : AutoMinCap) {
		int32 Amount = Camera->ResourceManager->GetResourceAmount(minStruct.Resource);

		if (!minStruct.bSell || Amount <= minStruct.Min)
			continue;

		FItemStruct item;
		item.Resource = minStruct.Resource;
		item.Amount = Amount - minStruct.Min;

		order.SellingItems.Add(item);
	}

	SetNewOrder(order);
}