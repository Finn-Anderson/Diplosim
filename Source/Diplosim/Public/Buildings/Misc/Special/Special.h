#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Special.generated.h"


UCLASS()
class DIPLOSIM_API ASpecial : public AInternalProduction
{
	GENERATED_BODY()
	
public:
	ASpecial();

	virtual void Rebuild(FString NewFactionName = "") override;

	virtual void OnBuilt() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
		UStaticMesh* ReplacementMesh;
};
