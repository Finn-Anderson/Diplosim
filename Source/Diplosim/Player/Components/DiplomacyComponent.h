#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "DiplomacyComponent.generated.h"

USTRUCT(BlueprintType)
struct FCultureImageStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		FString Religion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
		UTexture2D* Texture;

	FCultureImageStruct()
	{
		Party = "";
		Religion = "";
		Texture = nullptr;
	}

	bool operator==(const FCultureImageStruct& other) const
	{
		return (other.Party == Party && other.Religion == Religion);
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
class DIPLOSIM_API UDiplomacyComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDiplomacyComponent();

	void SetFactionFlagColour(FFactionStruct* Faction);

	void SetFactionCulture(FFactionStruct* Faction);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culture Texture List")
		TArray<FCultureImageStruct> CultureTextureList;

	UPROPERTY()
		class ACamera* Camera;

private:
	UTexture2D* GetTextureFromCulture(FString Party, FString Religion);

	FFactionStruct* GetFactionPtr(FFactionStruct Faction);

	int32 GetStrengthScore(FFactionStruct Faction);
};
