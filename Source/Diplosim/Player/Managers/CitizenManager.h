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

	FDiseaseStruct()
	{
		Name = "";
		Grade = EGrade::Mild;
		Spreadability = 0;
		Level = 0;
		DeathLevel = 120;
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

	void PickCitizenToHeal(ACitizen* Healer);

	int32 DiseaseTimer;

	int32 DiseaseTimerTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		TArray<FDiseaseStruct> Diseases;

	UPROPERTY()
		TArray<ACitizen*> Healing;
};
