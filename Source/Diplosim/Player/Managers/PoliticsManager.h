#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "PoliticsManager.generated.h"

UENUM()
enum class ERaidPolicy : uint8
{
	Default,
	Home,
	EggTimer
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UPoliticsManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPoliticsManager();

	void PoliticsLoop(FFactionStruct* Faction);

	FPartyStruct* GetMembersParty(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		FString GetCitizenParty(ACitizen* Citizen);

	void SelectNewLeader(FPartyStruct* Party);

	void StartElectionTimer(FFactionStruct* Faction);

	UFUNCTION()
		void Election(FFactionStruct Faction);

	UFUNCTION(BlueprintCallable)
		void Bribe(class ACitizen* Representative, bool bAgree);

	UFUNCTION(BlueprintCallable)
		void ProposeBill(FString FactionName, FLawStruct Bill);

	UFUNCTION()
		void MotionBill(FFactionStruct Faction, FLawStruct Bill);

	bool IsInRange(TArray<int32> Range, int32 Value);

	UFUNCTION(BlueprintCallable)
		int32 GetLawValue(FString FactionName, FString BillType);

	UFUNCTION(BlueprintCallable)
		int32 GetCooldownTimer(FString FactionName, FLawStruct Law);

	UFUNCTION(BlueprintCallable)
		FString GetBillPassChance(FString FactionName, FLawStruct Bill);

	ERaidPolicy GetRaidPolicyStatus(ACitizen* Citizen);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FPartyStruct> InitParties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TArray<FLawStruct> InitLaws;

	UPROPERTY()
		class ACamera* Camera;

	// Rebel
	void ChooseRebellionType(FFactionStruct* Faction);

	void Overthrow(FFactionStruct* Faction);

	void SetupRebel(FFactionStruct* Faction, class ACitizen* Citizen);

	bool IsRebellion(FFactionStruct* Faction);

protected:
	void ReadJSONFile(FString path);

private:
	void SetElectionBillLeans(FFactionStruct* Faction, FLawStruct* Bill);

	void SetupBill(FFactionStruct* Faction);

	void GetVerdict(FFactionStruct* Faction, class ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction);

	void TallyVotes(FFactionStruct* Faction, FLawStruct Bill);
};
