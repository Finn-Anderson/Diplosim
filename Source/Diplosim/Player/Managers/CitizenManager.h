// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

UENUM()
enum class EGrade : uint8
{
	Mild,
	Moderate,
	Severe
};

USTRUCT(BlueprintType)
struct FDiseaseStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		EGrade Grade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		int32 Spreadability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		int32 DeathLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		bool bContained;

	FDiseaseStruct()
	{
		Name = "";
		Grade = EGrade::Mild;
		Spreadability = 0;
		bContained = false;
	}

	bool operator==(const FDiseaseStruct& other) const
	{
		return (other.Name == Name);
	}
};


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
		
	// Disease
	void SpawnDisease();

	int32 DiseaseTimer;

	int32 DiseaseTimerTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		TArray<FDiseaseStruct> Diseases;
};
