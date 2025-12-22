#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "BuildingComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UBuildingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuildingComponent();

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AHouse* House;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AWork* Employment;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class ASchool* School;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AOrphanage* Orphanage;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class ABuilding* BuildingAt;

	UPROPERTY()
		FVector EnterLocation;

	// Find Job, House and Education
	void FindEducation(class ASchool* Education, int32 TimeToCompleteDay);

	void FindJob(class AWork* Job, int32 TimeToCompleteDay);

	void FindHouse(class AHouse* NewHouse, int32 TimeToCompleteDay, TArray<ACitizen*> Roommates);

	void SetJobHouseEducation(int32 TimeToCompleteDay, TArray<ACitizen*> Roommates);

	TArray<ACitizen*> GetRoomates();

	float GetAcquiredTime(int32 Index);

	void SetAcquiredTime(int32 Index, float Time);

	bool CanFindAnything(int32 TimeToCompleteDay, FFactionStruct* Faction);

	bool HasRecentlyAcquired(int32 Index, int32 TimeToCompleteDay);

	void RemoveOnReachingWorkAge(FFactionStruct* Faction);

	UPROPERTY()
		TArray<float> TimeOfAcquirement;

	UPROPERTY()
		TArray<class ABuilding*> AllocatedBuildings;

	UPROPERTY()
		ACitizen* FoundHouseOccupant;

	// Work
	bool CanWork(class ABuilding* WorkBuilding);

	bool WillWork();

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursWorkedMin;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		int32 IdealHoursWorkedMax;

	UPROPERTY(BlueprintReadOnly, Category = "Hours")
		TMap<int32, float> HoursWorked;

	// Family
	void RemoveFromHouse();

	void SelectPreferredPartnersHouse(ACitizen* Citizen, ACitizen* Partner);
};
