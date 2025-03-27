#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "School.generated.h"

UCLASS()
class DIPLOSIM_API ASchool : public AWork
{
	GENERATED_BODY()
	
public:
	ASchool();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 MaxSpace;

	virtual void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	virtual void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	TArray<class ACitizen*> GetStudentsAtSchool();

	void AddProgress();
};
