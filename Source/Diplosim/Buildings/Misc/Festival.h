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

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* InnerSpinMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* InnerIntermediateSpinMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* IntermediateSpinMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* OuterIntermediateSpinMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* OuterSpinMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		class UStaticMeshComponent* FestivalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
		class UBoxComponent* BoxAreaAffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Festival")
		bool bCanHostFestival;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Festival")
		TArray<FFestivalStruct> FestivalStruct;

	void Tick(float DeltaTime) override;

	UFUNCTION()
		void OnCitizenOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnCitizenOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnBuilt() override;

	void StartFestival(bool bFireFestival);

	void StopFestival();

	TArray<FName> GetSpinSockets();

	void AttachToSpinMesh(class ACitizen* Citizen, FName Socket);
};
