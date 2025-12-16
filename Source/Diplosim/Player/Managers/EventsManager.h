#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "EventsManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UEventsManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEventsManager();

	UFUNCTION(BlueprintCallable)
		void CreateEvent(FString FactionName, EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival = false);

	void ExecuteEvent(FString Period, int32 Day, int32 Hour);

	UFUNCTION(BlueprintCallable)
		bool IsAttendingEvent(class ACitizen* Citizen);

	void RemoveFromEvent(class ACitizen* Citizen);

	TMap<FFactionStruct*, TArray<FEventStruct*>> OngoingEvents();

	void GotoEvent(ACitizen* Citizen, FEventStruct* Event, FFactionStruct* Faction = nullptr);

	bool UpcomingProtest(FFactionStruct* Faction);

	void CreateWedding(int32 Hour);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
		TArray<FEventStruct> InitEvents;

	UPROPERTY()
		class ACamera* Camera;

private:
	void StartEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour);

	void EndEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour);
};
