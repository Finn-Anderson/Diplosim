#include "Buildings/Trader.h"

#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
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

	if (Citizen->Carrying.Type != nullptr)
		StoreResource(Citizen);

	if (CheckStored(Citizen, Orders[0].Items))
		SubmitOrder(Citizen);
}

void ATrader::SubmitOrder(class ACitizen* Citizen)
{
	ParticleComponent->Activate();

	Orders.RemoveAt(0);

	if (Orders.Num() > 0)
		CheckStored(Citizen, Orders[0].Items);
}