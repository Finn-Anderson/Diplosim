#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "AISpawner.generated.h"

UCLASS()
class DIPLOSIM_API AAISpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AAISpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* SpawnerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
		class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
		TSubclassOf<class AAI> AIClass;

	UPROPERTY()
		class ACamera* Camera;

	UPROPERTY()
		int32 IncrementSpawned;

	void ClearedNest(FFactionStruct* Faction);

protected:
	virtual void BeginPlay() override;

private:	
	void SpawnAI();


};
