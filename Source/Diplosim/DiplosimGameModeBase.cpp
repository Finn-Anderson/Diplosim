#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"
#include "Building.h"
#include "Camera.h"
//#include "ResourceManager.h"

void ADiplosimGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle taxTimer;
	GetWorld()->GetTimerManager().SetTimer(taxTimer, this, &ADiplosimGameModeBase::Tax, 600.0f, true);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
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
	
	//Camera->ResourceManager->ChangeResource(TEXT("Money"), tax);
}