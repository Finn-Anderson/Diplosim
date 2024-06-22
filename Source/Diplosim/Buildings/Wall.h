#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work.h"
#include "Wall.generated.h"

USTRUCT()
struct FSocketStruct
{
	GENERATED_USTRUCT_BODY()

	FName Name;

	FVector SocketLocation;

	FVector EntranceLocation;

	class ACitizen* Citizen;

	FSocketStruct()
	{
		Name = "";
		SocketLocation = FVector(0.0f, 0.0f, 0.0f);
		EntranceLocation = FVector(0.0f, 0.0f, 0.0f);
		Citizen = nullptr;
	}
};

UCLASS()
class DIPLOSIM_API AWall : public AWork
{
	GENERATED_BODY()
	
public:
	AWall();

	void StoreSocketLocations();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
		TSubclassOf<class AProjectile> BuildingProjectileClass;

	TArray<FSocketStruct> SocketList;
};
