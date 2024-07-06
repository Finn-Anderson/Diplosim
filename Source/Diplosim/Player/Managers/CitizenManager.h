// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UCitizenManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCitizenManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartTimers();

	UPROPERTY()
		TArray<class ACitizen*> Citizens;
};
