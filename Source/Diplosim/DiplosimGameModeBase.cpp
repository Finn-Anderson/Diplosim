#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "Building.h"

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle taxTimer;
	GetWorld()->GetTimerManager().SetTimer(taxTimer, this, &ADiplosimGameModeBase::Tax, 600.0f, true);
}

void ADiplosimGameModeBase::Tax()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	int32 tax = 0;

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);
		ABuilding* e = Cast<ABuilding>(c->Employment);

		float rate;
		if (e->EcoStatus == EEconomy::Poor) {
			rate = 2;
		}
		else if (e->EcoStatus == EEconomy::Modest) {
			rate = 5;
		}
		else {
			rate = 10;
		}

		tax += rate;
	}
	
	// Add to resource manager
}