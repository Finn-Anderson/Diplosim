#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "DiplomacyComponent.generated.h"

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

	void InitCultureList();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culture")
		TMap<FString, UTexture2D*> CultureTextureList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culture")
		UMaterial* CultureMaterial;

	UPROPERTY()
		class ACamera* Camera;

private:
	UMaterialInstanceDynamic* GetTextureFromCulture(FString Party, FString Religion);

	FFactionStruct* GetFactionPtr(FFactionStruct Faction);

	int32 GetStrengthScore(FFactionStruct Faction);
};
