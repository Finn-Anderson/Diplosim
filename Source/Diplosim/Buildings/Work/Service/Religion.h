#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "Religion.generated.h"

UCLASS()
class DIPLOSIM_API ABroadcast : public AWork
{
	GENERATED_BODY()
	
public:
	ABroadcast();

	virtual void Enter(class ACitizen* Citizen) override;

	virtual void Leave(class ACitizen* Citizen) override;

	virtual void OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	virtual void OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	UFUNCTION(BlueprintCallable)
		void SetBroadcastType(EParty Party = EParty::Undecided, EReligion Religion = EReligion::Atheist);

	void RemoveInfluencedMaterial(class AHouse* House);

	UPROPERTY()
		TArray<class AHouse*> Houses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
		class UMaterial* InfluencedMaterial;

	// Influences
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence")
		EReligion Belief;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence")
		EParty Allegiance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence")
		bool bHolyPlace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence")
		bool bIsPark;
};
