#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Building.h"
#include "House.generated.h"

USTRUCT(BlueprintType)
struct FRentStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rent")
		class ACitizen* Occupant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upkeep")
		int32 Rent;

	FRentStruct()
	{
		Occupant = nullptr;
		Rent = 0;
	}

	bool operator==(const FRentStruct& other) const
	{
		return (other.Occupant == Occupant);
	}
};

UCLASS()
class DIPLOSIM_API AHouse : public ABuilding
{
	GENERATED_BODY()
	
public:
	AHouse();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House")
		int32 BaseRent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House")
		TArray<FRentStruct> RentStruct;

	int32 GetSatisfactionLevel(int32 Rent);

	void CollectRent(FFactionStruct* Faction, class ACitizen* Citizen);

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	// Citizens
	virtual bool AddCitizen(class ACitizen* Citizen) override;

	virtual bool RemoveCitizen(class ACitizen* Citizen) override;

	virtual void AddVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	virtual void RemoveVisitor(class ACitizen* Occupant, class ACitizen* Visitor) override;

	// Rent
	FRentStruct* GetBestAvailableRoom();

	void SetRent(ACitizen* Citizen);

	void RemoveRent(ACitizen* Citizen);

	int32 GetRent(ACitizen* Citizen);

	UFUNCTION(BlueprintCallable)
		void UpdateRent(int32 Index, int32 NewRent);
};
