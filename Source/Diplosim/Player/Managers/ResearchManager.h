#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "ConquestManager.h"
#include "Components/ActorComponent.h"
#include "ResearchManager.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UResearchManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UResearchManager();

protected:
	void ReadJSONFile(FString Path);

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		TArray<FResearchStruct> InitResearchStruct;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Research")
		UTexture2D* DefaultTexture;

	UPROPERTY()
		class ACamera* Camera;

	UFUNCTION(BlueprintCallable)
		bool IsReseached(FString Name, FString FactionName);

	UFUNCTION(BlueprintCallable)
		bool IsBeingResearched(int32 Index, FString FactionName);

	UFUNCTION(BlueprintCallable)
		int32 GetCurrentResearchIndex(FString FactionName);

	UFUNCTION(BlueprintCallable)
		FResearchStruct GetCurrentResearch(FString FactionName);

	UFUNCTION(BlueprintCallable)
		void GetResearchAmount(int32 Index, FString FactionName, float& Amount, int32& Target);

	UFUNCTION(BlueprintCallable)
		void SetResearch(int32 Index, FString FactionName);

	UFUNCTION(BlueprintCallable)
		void RemoveResearch(int32 Index, FString FactionName);

	UFUNCTION(BlueprintCallable)
		void ReorderResearch(int32 OldIndex, int32 NewIndex, FString FactionName);

	void Research(float Amount, FString FactionName);
};
