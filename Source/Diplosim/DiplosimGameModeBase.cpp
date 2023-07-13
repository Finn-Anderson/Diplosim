#include "DiplosimGameModeBase.h"

#include "Kismet/GameplayStatics.h"

#include "Citizen.h"

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

	int32 tax = 0.5 * citizens.Num();
}