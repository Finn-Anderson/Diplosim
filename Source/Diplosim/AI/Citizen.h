#pragma once

#include "CoreMinimal.h"
#include "AI/AI.h"
#include "Citizen.generated.h"

USTRUCT()
struct FCarryStruct
{
	GENERATED_USTRUCT_BODY()

	class AResource* Type;

	int32 Amount;

	FCarryStruct()
	{
		Type = nullptr;
		Amount = 0;
	}
};

USTRUCT(BlueprintType)
struct FBuildingStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class AHouse* House;

	UPROPERTY(BlueprintReadOnly, Category = "Building")
		class ABuilding* Employment;

	class ABuilding* BuildingAt;

	FVector EnterLocation;

	FBuildingStruct()
	{
		House = nullptr;
		Employment = nullptr;
		BuildingAt = nullptr;
	}
};

UENUM()
enum class ESex : uint8
{
	NaN,
	Male,
	Female
};

USTRUCT(BlueprintType)
struct FBioStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Mother;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Father;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TWeakObjectPtr<class ACitizen> Partner;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		ESex Sex;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		int32 Age;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		TArray<class ACitizen*> Children;

	FBioStruct()
	{
		Mother = nullptr;
		Father = nullptr;
		Partner = nullptr;
		Sex = ESex::NaN;
		Age = 0;
		Name = "Citizen";
	}
};

UENUM()
enum class EParty : uint8
{
	Undecided,
	Religious,
	Militarist,
	Industrialist,
	Environmentalist
};

UENUM()
enum ESway : uint8
{
	NaN,
	Moderate,
	Strong,
	Radical
};

USTRUCT(BlueprintType)
struct FPartyStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		EParty Party;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TEnumAsByte<ESway> Leaning;

	FPartyStruct()
	{
		Party = EParty::Undecided;
		Leaning = ESway::NaN;
	}

	bool operator==(const FPartyStruct& other) const
	{
		return (other.Party == Party) && (other.Leaning == Leaning);
	}
};

USTRUCT(BlueprintType)
struct FPoliticalStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Politics")
		FPartyStruct Ideology;

	FPartyStruct FathersIdeology;

	FPartyStruct MothersIdeology;
};

UENUM()
enum class EReligion : uint8
{
	Undecided,
	Egg,
	Chicken,
	Fox
};

USTRUCT(BlueprintType)
struct FReligionStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		EReligion Religion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Religion")
		TEnumAsByte<ESway> Leaning;

	FReligionStruct()
	{
		Religion = EReligion::Undecided;
		Leaning = ESway::NaN;
	}

	bool operator==(const FReligionStruct& other) const
	{
		return (other.Religion == Religion) && (other.Leaning == Leaning);
	}
};

USTRUCT(BlueprintType)
struct FSpiritualStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FReligionStruct Faith;

	FReligionStruct FathersFaith;

	FReligionStruct MothersFaith;

	bool bBoost;
};

USTRUCT()
struct FCollidingStruct
{
	GENERATED_USTRUCT_BODY()

	AActor* Actor;

	int32 Instance;

	FCollidingStruct()
	{
		Actor = nullptr;
		Instance = -1;
	}

	bool operator==(const FCollidingStruct& other) const
	{
		return (other.Actor == Actor) && (other.Instance == Instance);
	}
};

UCLASS()
class DIPLOSIM_API ACitizen : public AAI
{
	GENERATED_BODY()

public:
	ACitizen();

protected:
	virtual void BeginPlay() override;

public:
	virtual void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
		
	virtual void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	// Cosmetics
	UFUNCTION(BlueprintCallable)
		void SetTorch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UStaticMeshComponent* HatMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class USkeletalMeshComponent* TorchMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmetics")
		class UNiagaraComponent* TorchNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease")
		class UNiagaraComponent* DiseaseNiagaraComponent;

	// Work
	bool CanWork(class ABuilding* ReligiousBuilding);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Money")
		int32 Balance;


	// Buildings
	UPROPERTY(BlueprintReadOnly, Category = "Buildings")
		FBuildingStruct Building;

	UPROPERTY()
		TArray<FCollidingStruct> StillColliding;


	// Resources
	void StartHarvestTimer(class AResource* Resource, int32 Instance);

	void HarvestResource(class AResource* Resource, int32 Instance);

	void Carry(class AResource* Resource, int32 Amount, AActor* Location);

	UPROPERTY()
		FCarryStruct Carrying;


	// Food
	void Eat();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
		TArray<TSubclassOf<class AResource>> Food;

	UPROPERTY(BlueprintReadOnly, Category = "Food")
		int32 Hunger;

	UPROPERTY()
		int32 HungerTimer;


	// Energy
	void CheckGainOrLoseEnergy();

	void LoseEnergy();

	void GainEnergy();

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
		int32 Energy;

	UPROPERTY()
		int32 EnergyTimer;

	UPROPERTY()
		bool bGain;

	UPROPERTY()
	float InitialSpeed;


	// Bio
	void Birthday();

	void SetSex();

	void SetName();

	void FindPartner();

	void SetPartner(ACitizen* Citizen);

	void HaveChild();

	UPROPERTY(BlueprintReadOnly, Category = "Bio")
		FBioStruct BioStruct;

	UPROPERTY()
		int32 AgeTimer;

	// Politics
	UPROPERTY(BlueprintReadOnly, Category = "Politics")
		FPoliticalStruct Politics;

	void SetPolticalLeanings();

	// Religion
	UPROPERTY(BlueprintReadOnly, Category = "Religion")
		FSpiritualStruct Spirituality;

	void SetReligionLeanings();

	// Disease
	UPROPERTY()
		TArray<struct FDiseaseStruct> CaughtDiseases;
};