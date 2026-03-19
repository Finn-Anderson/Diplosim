#pragma once

#include "CoreMinimal.h"
#include "Buildings/Building.h"
#include "Festival.generated.h"

USTRUCT(BlueprintType)
struct FFestivalStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Festival")
		class UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roads")
		class UNiagaraSystem* ParticleSystem;

	FFestivalStruct()
	{
		Mesh = nullptr;
		ParticleSystem = nullptr;
	}
};

UCLASS()
class DIPLOSIM_API AFestival : public ABuilding
{
	GENERATED_BODY()
	
public:
	AFestival();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* FestivalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Festival")
		bool bCanHostFestival;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Festival")
		TArray<FFestivalStruct> FestivalStruct;

	void Tick(float DeltaTime) override;

	void StartFestival(bool bFireFestival);

	void StopFestival();

	void OnBuilt() override;

protected:
	virtual void StoreSocketLocations() override;
};
