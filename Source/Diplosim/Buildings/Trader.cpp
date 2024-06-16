#include "Buildings/Trader.h"

#include "NiagaraComponent.h"
#include "Components/Widget.h"

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
		SubmitOrder(Citizen);
}

void ATrader::SubmitOrder(class ACitizen* Citizen)
{
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

	Orders.RemoveAt(0);

	if (Orders.Num() > 0) {
		if (Orders[0].bCancelled)
			ReturnResource(Citizen);
		else
			CheckStored(Citizen, Orders[0].Items);
	}
}