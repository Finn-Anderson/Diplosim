#include "Buildings/Work/Booster.h"

ABooster::ABooster()
{

}

void ABooster::OnRadialOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRadialOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	for (TSubclassOf<AWork> workClass : WorkplacesToBoost) {
		if (OtherActor->IsA(workClass))
			Cast<AWork>(OtherActor)->Boosters.Add(this);

		break;
	}
}

void ABooster::OnRadialOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnRadialOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (OtherActor->IsA<AWork>() && Cast<AWork>(OtherActor)->Boosters.Contains(this))
		Cast<AWork>(OtherActor)->Boosters.Remove(this);
}