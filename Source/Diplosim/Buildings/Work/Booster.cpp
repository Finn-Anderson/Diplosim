#include "Buildings/Work/Booster.h"

ABooster::ABooster()
{

}

void ABooster::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	for (TSubclassOf<AWork> workClass : WorkplacesToBoost) {
		if (OtherActor->IsA(workClass))
			Cast<AWork>(OtherActor)->bBoost = true;

		break;
	}
}

void ABooster::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	for (TSubclassOf<AWork> workClass : WorkplacesToBoost) {
		if (OtherActor->IsA(workClass))
			Cast<AWork>(OtherActor)->bBoost = false;

		break;
	}
}