#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Orphanage.generated.h"

UCLASS()
class DIPLOSIM_API AOrphanage : public AWork
{
	GENERATED_BODY()
	
public:
	AOrphanage();

	void Enter(class ACitizen* Citizen) override;

	void Leave(class ACitizen* Citizen) override;

	void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	void Kickout(class ACitizen* Citizen);

	void PickChildren(class ACitizen* Citizen);
};
