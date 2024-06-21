#include "Buildings/Trader.h"

#include "NiagaraComponent.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"

#include "AI/Citizen.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

ATrader::ATrader()
{

}

void ATrader::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (!GetOccupied().Contains(Citizen) || Orders.IsEmpty())
		return;

	if (Orders[0].bCancelled)
		ReturnResource(Citizen);
	else if (CheckStored(Citizen, Orders[0].Items))
		GetWorldTimerManager().SetTimer(WaitTimer, FTimerDelegate::CreateUObject(this, &ATrader::SubmitOrder, Citizen), Orders[0].Wait + 0.01f, false);
}

void ATrader::SubmitOrder(class ACitizen* Citizen)
{
	if (GetCitizensAtBuilding().IsEmpty())
		return;

	ParticleComponent->Activate();

	int32 money = 0;

	for (FItemStruct item : Orders[0].Items) {
		FValueStruct value;
		value.Resource = item.Resource;

		int32 index = ResourceValues.Find(value);

		money += ResourceValues[index].Value * item.Amount;
	}

	Camera->ResourceManagerComponent->AddUniversalResource(Money, money);

	Orders[0].OrderWidget->RemoveFromParent();

	bool bRefresh = false;

	if (Orders[0].bRepeat && !Orders[0].bCancelled) {
		for (int32 i = 0; i < Orders[0].Items.Num(); i++)
			Orders[0].Items[i].Stored = 0;

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
			CheckStored(Citizen, Orders[0].Items);
	}
}