#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "CitizenManager.generated.h"

USTRUCT(BlueprintType)
struct FReligionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		FString Faith;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FString> Agreeable;

	FReligionStruct()
	{
		Faith = "";
	}

	bool operator==(const FReligionStruct& other) const
	{
		return (other.Faith == Faith);
	}
};

USTRUCT(BlueprintType)
struct FPersonality
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		FString Trait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FString> Likes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FString> Dislikes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TMap<FString, float> Affects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		float Aggressiveness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<class ACitizen*> Citizens;

	FPersonality()
	{
		Trait = "";
		Aggressiveness = 1.0f;
	}

	bool operator==(const FPersonality& other) const
	{
		return (other.Trait == Trait);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UCitizenManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCitizenManager();

	void CitizenGeneralLoop();

	void CalculateGoalInteractions();

	void CalculateConversationInteractions();

	UPROPERTY()
		class ACamera* Camera;

	// House
	UFUNCTION(BlueprintCallable)
		void UpdateRent(FString FactionName, TSubclassOf<class AHouse> HouseType, int32 NewRent);

	// Death
	void ClearCitizen(ACitizen* Citizen);

	// Work
	void CheckWorkStatus(int32 Hour);

	// Citizen
	void CheckUpkeepCosts();

	void CheckCitizenStatus(int32 Hour);

	void IssuePensions(int32 Hour);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pensions")
		int32 IssuePensionHour;

	// Conversations
	USoundBase* GetConversationSound(ACitizen* Citizen);

	void StartConversation(FFactionStruct* Faction, class ACitizen* Citizen1, class ACitizen* Citizen2, bool bInterrogation);

	UFUNCTION()
		void Interact(FFactionStruct Faction, class ACitizen* Citizen1, class ACitizen* Citizen2);

	// Genetics

	UFUNCTION(BlueprintCallable)
		void Pray(FString FactionName);

	UFUNCTION(BlueprintCallable)
		int32 GetPrayCost(FString FactionName);

	UFUNCTION(BlueprintCallable)
		void Sacrifice(FString FactionName);

	UFUNCTION()
		void IncrementPray(FFactionStruct Faction, FString Type, int32 Increment);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sacrifice")
		class UNiagaraSystem* SacrificeSystem;

	// Religion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TArray<FReligionStruct> Religions;

	// Personality
	TArray<FPersonality*> GetCitizensPersonalities(class ACitizen* Citizen);

	float GetAggressiveness(class ACitizen* Citizen);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness, float& Citizen1Aggressiveness, float& Citizen2Aggressiveness);

	void PersonalityComparison(class ACitizen* Citizen1, class ACitizen* Citizen2, int32& Likeness);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality")
		TArray<FPersonality> Personalities;

protected:
	void ReadJSONFile(FString path);

private:
	void SetPrayTimer(FFactionStruct Faction, FString Type);

	FCriticalSection CitizenGeneralLoopLock;

	FCriticalSection GoalInteractionsLock;
	FCriticalSection ConversationInteractionsLock;
};