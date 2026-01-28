#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "HappinessComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UHappinessComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHappinessComponent();

	UFUNCTION()
		void SetAttendStatus(EAttendStatus Status, bool bMass);

	void SetDecayingHappiness(int32* HappinessToDecay, int32 Amount, int32 Min = -24, int32 Max = 24);

	void DecayHappiness();

	UFUNCTION(BlueprintCallable)
		int32 GetHappiness();

	void SetHappiness();

	UPROPERTY(BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	UPROPERTY(BlueprintReadOnly, Category = "Festival")
		EAttendStatus FestivalStatus;

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		EAttendStatus MassStatus;

	UPROPERTY()
		int32 ConversationHappiness;

	UPROPERTY()
		int32 FamilyDeathHappiness;

	UPROPERTY()
		int32 WitnessedDeathHappiness;

	UPROPERTY()
		int32 DivorceHappiness;

	UPROPERTY()
		int32 SadTimer;

private:
	void SetHousingHappiness(class ACitizen* Citizen, FFactionStruct* Faction);

	void SetBoosterHappiness(class ACitizen* Citizen, FFactionStruct* Faction);

	void SetWorkHappiness(class ACitizen* Citizen, FFactionStruct* Faction);

	void SetPoliticsHappiness(class ACitizen* Citizen, FFactionStruct* Faction);

	void SetSexualityHappiness(class ACitizen* Citizen, FFactionStruct* Faction);

	void CheckSadnessTimer(class ACitizen* Citizen, FFactionStruct* Faction);
};