#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "ConquestManager.generated.h"

USTRUCT(BlueprintType)
struct FFactionHappinessStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		FString Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Happiness")
		TMap<FString, int32> Modifiers;

	int32 ProposalTimer;

	FFactionHappinessStruct()
	{
		ClearValues();
		ProposalTimer = 0;
	}

	void ClearValues()
	{
		Modifiers.Empty();
	}
	
	bool Contains(FString Key)
	{
		return Modifiers.Contains(Key);
	}

	int32 GetValue(FString Key)
	{
		return *Modifiers.Find(Key);
	}

	void SetValue(FString Key, int32 Value)
	{
		Modifiers.Add(Key, Value);
	}

	void RemoveValue(FString Key)
	{
		Modifiers.Remove(Key);
	}

	void Decay(FString Key) {
		int32 value = GetValue(Key);

		if (value < 0)
			value++;
		else
			value--;

		if (value != 0)
			SetValue(Key, value);
		else
			RemoveValue(Key);
	}

	bool operator==(const FFactionHappinessStruct& other) const
	{
		return (other.Owner == Owner);
	}
};

USTRUCT(BlueprintType)
struct FFactionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> AtWar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FString> Allies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		TArray<FFactionHappinessStruct> Happiness;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
		int32 WarFatigue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString PartyInPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString LargestReligion;

	FFactionStruct()
	{
		Name = "";
		WarFatigue = 0;
		PartyInPower = "";
		LargestReligion = "";
	}

	bool operator==(const FFactionStruct& other) const
	{
		return (other.Name == Name);
	}
};

USTRUCT(BlueprintType)
struct FCultureImageStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Type;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		UTexture2D* Texture;

	FCultureImageStruct()
	{
		Type = "";
		Texture = nullptr;
	}

	bool operator==(const FCultureImageStruct& other) const
	{
		return (other.Type == Type);
	}
};

USTRUCT(BlueprintType)
struct FRaidStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raiding")
		FString Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raiding")
		TMap<class ACitizen*, int32> Raiders;

	FRaidStruct() 
	{
		Owner = "";
	}

	bool operator==(const FRaidStruct& other) const
	{
		return (other.Owner == Owner);
	}
};

USTRUCT(BlueprintType)
struct FGiftStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
		TSubclassOf<class AResource> Resource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift")
		int32 Amount;

	FGiftStruct()
	{
		Resource = nullptr;
		Amount = 0;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UConquestManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UConquestManager();

protected:
	virtual void BeginPlay() override;

public:	
	void CreateFactions();

	int32 GetFactionIndexFromOwner(FString Owner);

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetFactionFromOwner(FString Owner);

	UFUNCTION(BlueprintCallable)
		UTexture2D* GetTextureFromCulture(FString Type);

	void ComputeAI();

	UFUNCTION(BlueprintCallable)
		bool CanTravel(class ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FFactionStruct GetCitizenFaction(class ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
		int32 EnemiesNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culture Texture List")
		TArray<FCultureImageStruct> CultureTextureList;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> Factions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factions")
		TArray<FFactionStruct> FactionsToRemove;

	FCriticalSection ConquestLock;

	// Diplomacy
	void SetFactionCulture(FFactionStruct Faction);

	UFUNCTION(BlueprintCallable)
		int32 GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		int32 GetHappinessValue(FFactionHappinessStruct Happiness);

	void SetFactionsHappiness(FFactionStruct Faction);

	void EvaluateDiplomacy(FFactionStruct Faction);

	TTuple<bool, bool> IsWarWinnable(FFactionStruct Faction, FFactionStruct& Target);

	UFUNCTION(BlueprintCallable)
		void Peace(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void Ally(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void BreakAlliance(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void DeclareWar(FFactionStruct Faction1, FFactionStruct Faction2);

	UFUNCTION(BlueprintCallable)
		void Insult(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		void Praise(FFactionStruct Faction, FFactionStruct Target);

	UFUNCTION(BlueprintCallable)
		void Gift(FFactionStruct Faction, TArray<FGiftStruct> Gifts);

	// AI
	void EvaluateAI(FFactionStruct Faction);

	// UI
	UFUNCTION(BlueprintCallable)
		void DisplayConquestNotification(FString Message, FString Owner, bool bChoice);
};
