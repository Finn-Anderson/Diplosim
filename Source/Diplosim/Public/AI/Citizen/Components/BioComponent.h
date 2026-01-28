#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "BioComponent.generated.h"

UENUM()
enum class ESex : uint8
{
	NaN,
	Male,
	Female
};

UENUM()
enum class ESexuality : uint8
{
	NaN,
	Straight,
	Homosexual,
	Bisexual
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UBioComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBioComponent();

	UFUNCTION()
		void Birthday();

	void SetSex(TArray<class ACitizen*> Citizens);

	void SetName();

	void SetSexuality(TArray<class ACitizen*> Citizens);

	void FindPartner(FFactionStruct* Faction);

	void SetPartner(class ACitizen* Citizen);

	void RemoveMarriage();

	void RemovePartner();

	void IncrementHoursTogetherWithPartner();

	void HaveChild();

	TArray<ACitizen*> GetLikedFamily(bool bFactorAge);

	void Disown();

	void Adopt(ACitizen* Child);

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Mother;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Father;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Partner;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 HoursTogetherWithPartner;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		bool bMarried;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		ESex Sex;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		ESexuality Sexuality;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 Age;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 EducationLevel;

	UPROPERTY()
		int32 EducationProgress;

	UPROPERTY()
		int32 PaidForEducationLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TArray<class ACitizen*> Children;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TArray<class ACitizen*> Siblings;

	UPROPERTY()
		bool bAdopted;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float SpeedBeforeOld;

	UPROPERTY(BlueprintReadOnly, Category = "Age")
		float MaxHealthBeforeOld;
};