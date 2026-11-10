#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DiplosimGameInstance.generated.h"

UCLASS()
class DIPLOSIM_API UDiplosimGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UDiplosimGameInstance();

	UPROPERTY(BlueprintReadOnly, Category="Shutdown")
		bool bShutdown;

protected:
	virtual void Shutdown() override;
};
