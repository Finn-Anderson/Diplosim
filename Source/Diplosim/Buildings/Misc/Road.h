#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Road.generated.h"

USTRUCT(BlueprintType)
struct FRoadStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		class UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		int32 Connections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		bool bStraight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		bool bBridge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		int32 Tier;

	FRoadStruct()
	{
		Mesh = nullptr;
		Connections = 0;
		bStraight = false;
		bBridge = false;
		Tier = -1;
	}

	bool operator==(const FRoadStruct& other) const
	{
		return (other.Connections == Connections && other.bStraight == bStraight && other.bBridge == bBridge && other.Tier == Tier) || (other.bBridge == bBridge && bBridge == true && other.Tier == Tier);
	}
};

UCLASS()
class DIPLOSIM_API ARoad : public ABuilding
{
	GENERATED_BODY()
	
public:
	ARoad();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
		void OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
		void OnRoadOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnRoadOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		class UBoxComponent* BoxCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		TArray<FRoadStruct> RoadMeshes;

	void RegenerateMesh();

	void SetTier(int32 Value) override;
};
