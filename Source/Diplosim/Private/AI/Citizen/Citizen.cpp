#include "AI/Citizen/Citizen.h"

#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Blueprint/UserWidget.h"

#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Special/PowerPlant.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"

ACitizen::ACitizen()
{
	AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudioComponent"));
	AmbientAudioComponent->SetupAttachment(RootComponent);
	AmbientAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientAudioComponent->bCanPlayMultipleInstances = true;

	BuildingComponent = CreateDefaultSubobject<UBuildingComponent>(TEXT("BuildingComponent"));

	BioComponent = CreateDefaultSubobject<UBioComponent>(TEXT("BioComponent"));

	HappinessComponent = CreateDefaultSubobject<UHappinessComponent>(TEXT("HappinessComponent"));

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	bGain = false;

	bHasBeenLeader = false;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ProductivityMultiplier = 1.0f;
	Fertility = 1.0f;
	ReachMultiplier = 1.0f;
	AwarenessMultiplier = 1.0f;
	FoodMultiplier = 1.0f;
	HungerMultiplier = 1.0f;
	EnergyMultiplier = 1.0f;

	bSleep = false;
	IdealHoursSlept = 8;
	HoursSleptToday = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };

	VoicePitch = 1.0f;
	bConversing = false;

	bGlasses = false;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	Camera->TimerManager->CreateTimer("Birthday", this, timeToCompleteDay / 10.0f, "Birthday", {}, true);
}

void ACitizen::CitizenSetup(FFactionStruct* Faction)
{
	Faction->Citizens.Add(this);
	Camera->DiseaseManager->Infectible.Add(this);

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	Camera->TimerManager->CreateTimer("Eat", this, (timeToCompleteDay / 200) * HungerMultiplier, "Eat", {}, true);

	Camera->TimerManager->CreateTimer("Energy", this, (timeToCompleteDay / 100) * EnergyMultiplier, "CheckGainOrLoseEnergy", {}, true);

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(this, params);
	Camera->TimerManager->CreateTimer("ChooseIdleBuilding", this, 60, "ChooseIdleBuilding", params, true);

	if (BioComponent->Mother != nullptr && BioComponent->Mother->BuildingComponent->BuildingAt != nullptr)
		BioComponent->Mother->BuildingComponent->BuildingAt->Enter(this);

	GenerateGenetics(Faction);
	ApplyResearch(Faction);

	float minPitch = 0.8f;
	float maxPitch = 1.2f;

	for (FGeneticsStruct genetic : Genetics) {
		if (genetic.Type != EGeneticsType::Reach)
			continue;

		if (genetic.Grade == EGeneticsGrade::Good) {
			minPitch = 0.6f;
			maxPitch = 1.0f;
		}
		else if (genetic.Grade == EGeneticsGrade::Bad) {
			minPitch = 1.0f;
			maxPitch = 1.4f;
		}
	}

	VoicePitch = Camera->Stream.FRandRange(minPitch, maxPitch);

	AIController->ChooseIdleBuilding(this);
	AIController->DefaultAction();
}

void ACitizen::ClearCitizen()
{
	if (IsValid(BuildingComponent->Employment))
		BuildingComponent->Employment->RemoveCitizen(this);
	else if (IsValid(BuildingComponent->School))
		BuildingComponent->School->RemoveVisitor(BuildingComponent->School->GetOccupant(this), this);

	if (IsValid(BuildingComponent->House)) {
		if (BuildingComponent->House->GetOccupied().Contains(this)) {
			if (BioComponent->Partner != nullptr) {
				BuildingComponent->House->RemoveVisitor(this, BioComponent->Partner.Get());

				int32 index = BuildingComponent->House->GetOccupied().Find(this);

				BuildingComponent->House->Occupied[index].Citizen = BioComponent->Partner.Get();
			}
			else {
				BuildingComponent->House->RemoveCitizen(this);
			}
		}
		else {
			BuildingComponent->House->RemoveVisitor(BuildingComponent->House->GetOccupant(this), this);
		}
	}
	else if (IsValid(BuildingComponent->Orphanage)) {
		BuildingComponent->Orphanage->RemoveVisitor(BuildingComponent->Orphanage->GetOccupant(this), this);
	}

	BioComponent->Disown();
	BioComponent->RemovePartner();

	Camera->EventsManager->RemoveFromEvent(this);
}

void ACitizen::ApplyResearch(FFactionStruct* Faction)
{
	for (FResearchStruct& research : Faction->ResearchStruct)
		for (int32 i = 0; i < research.Level; i++)
			for (auto& element : research.Modifiers)
				ApplyToMultiplier(element.Key, element.Value);
}

//
// Economy
//
bool ACitizen::CanAffordEducationLevel()
{
	if (BioComponent->EducationLevel < BioComponent->PaidForEducationLevel)
		return true;
	
	int32 money = 0;
	int32 leftoverMoney = GetLeftoverMoney();

	if (leftoverMoney > 0)
		money += leftoverMoney;

	for (ACitizen* citizen : BioComponent->GetLikedFamily(false)) {
		int32 leftover = citizen->GetLeftoverMoney();

		if (leftover <= 0)
			continue;

		money += leftover;
	}

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	if (money < Camera->PoliticsManager->GetLawValue(faction->Name, "Education Cost"))
		return false;

	if (IsValid(BuildingComponent->School))
		PayForEducationLevels();

	return true;
}

void ACitizen::PayForEducationLevels()
{
	if (BioComponent->EducationLevel < BioComponent->PaidForEducationLevel)
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	TMap<ACitizen*, int32> wallet;
	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Education Cost");

	int32 leftoverMoney = GetLeftoverMoney();

	if (cost > 0) {
		if (leftoverMoney < cost) {
			for (ACitizen* citizen : BioComponent->GetLikedFamily(false)) {
				int32 leftover = citizen->GetLeftoverMoney();

				if (leftover <= 0)
					continue;

				wallet.Add(citizen, leftover);
			}
		}
	}

	for (int32 i = 0; i < cost; i++) {
		if (GetLeftoverMoney() <= 0 && !wallet.IsEmpty()) {
			int32 index = Camera->Stream.RandRange(0, wallet.Num() - 1);

			TArray<ACitizen*> family;
			wallet.GenerateKeyArray(family);

			family[index]->Balance -= 1;

			int32 leftover = *wallet.Find(family[index]) - 1;

			if (leftover == 0)
				wallet.Remove(family[index]);
			else
				wallet.Add(family[index], leftover);
		}
		else {
			Balance -= 1;
		}
	}

	BioComponent->PaidForEducationLevel++;
}

int32 ACitizen::GetLeftoverMoney()
{
	int32 money = Balance;

	if (IsValid(BuildingComponent->House))
		money -= BuildingComponent->House->GetAmount(this);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);
	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Food Cost");

	for (ACitizen* child : BioComponent->Children) {
		int32 maxF = FMath::CeilToInt((100 - child->Hunger) / (25.0f * child->FoodMultiplier));

		int32 modifier = 1;

		if (BioComponent->Partner != nullptr && IsValid(BioComponent->Partner->BuildingComponent->Employment))
			modifier = 2;

		money -= (cost * maxF) / modifier;
	}

	int32 maxF = FMath::CeilToInt((100 - Hunger) / (25.0f * FoodMultiplier));

	money -= cost * maxF;

	return money;
}

//
// Health
//

void ACitizen::Heal(ACitizen* Citizen)
{		
	Camera->DiseaseManager->Cure(Citizen);

	Camera->DiseaseManager->PairCitizenToHealer(Camera->ConquestManager->GetFaction("", this), this);
}

//
// Food
//
void ACitizen::Eat()
{
	Hunger = FMath::Clamp(Hunger - 1, 0, 100);

	if (Hunger > 25)
		return;
	else if (Hunger == 0)
		HealthComponent->TakeHealth(10, this);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;

	TArray<TSubclassOf<AResource>> foods = Camera->ResourceManager->GetResourcesFromCategory("Food");

	for (TSubclassOf<AResource> food : foods) {
		int32 curAmount = Camera->ResourceManager->GetResourceAmount(faction->Name, food);

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Food Cost");

	int32 maxF = FMath::CeilToInt((100 - Hunger) / (25.0f * FoodMultiplier));
	int32 quantity = FMath::Clamp(totalAmount, 0, maxF);

	TMap<ACitizen*, int32> wallet;

	if (cost > 0 && !IsValid(BuildingComponent->Orphanage) && !faction->Police.Arrested.Contains(this)) {
		if (FMath::Floor(Balance / cost) < quantity) {
			for (ACitizen* citizen : BioComponent->GetLikedFamily(true)) {
				if (citizen->Balance <= 0)
					continue;

				wallet.Add(citizen, citizen->Balance);
			}
		}

		int32 total = 0;

		if (Balance > 0)
			total += Balance;

		for (auto& element : wallet)
			total += element.Value;

		if (FMath::Floor(total / cost) < 1) {
			if (Balance > 0)
				total -= Balance;

			total += Balance;
		}

		quantity = FMath::Min(quantity, FMath::Floor(total / cost));
	}

	for (int32 i = 0; i < quantity; i++) {
		int32 selected = Camera->Stream.RandRange(0, totalAmount - 1);

		for (int32 j = 0; j < foodAmounts.Num(); j++) {
			if (foodAmounts[j] <= selected) {
				selected -= foodAmounts[j];
			}
			else {
				Camera->ResourceManager->TakeUniversalResource(faction, foods[j], 1, 0);

				foodAmounts[j] -= 1;
				totalAmount -= 1;

				if (cost > 0 && !IsValid(BuildingComponent->Orphanage) && !faction->Police.Arrested.Contains(this)) {
					for (int32 k = 0; k < cost; k++) {
						if (Balance <= 0 && !wallet.IsEmpty()) {
							int32 index = Camera->Stream.RandRange(0, wallet.Num() - 1);

							TArray<ACitizen*> family;
							wallet.GenerateKeyArray(family);

							family[index]->Balance -= 1;

							int32 leftover = *wallet.Find(family[index]) - 1;

							if (leftover == 0)
								wallet.Remove(family[index]);
							else
								wallet.Add(family[index], leftover);
						}
						else {
							Balance -= 1;
						}
					}
				}

				break;
			}
		}

		Camera->ResourceManager->AddUniversalResource(faction, Camera->ResourceManager->Money, cost);

		Hunger = FMath::Clamp(Hunger + 25, 0, 100);
	}

	if (Hunger > 25)
		MovementComponent->Transform.SetScale3D(MovementComponent->Transform.GetScale3D() / FVector(0.75f, 0.75f, 1.0f));
	else if (Hunger == 25)
		MovementComponent->Transform.SetScale3D(MovementComponent->Transform.GetScale3D() * FVector(0.75f, 0.75f, 1.0f));
}

//
// Energy
//
void ACitizen::CheckGainOrLoseEnergy()
{
	if (bGain)
		GainEnergy();
	else
		LoseEnergy();
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	MovementComponent->SetMaxSpeed(Energy);

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty() || bWorshipping || (IsValid(BuildingComponent->Employment) && BuildingComponent->Employment->IsWorking(this)) || Camera->ArmyManager->IsCitizenInAnArmy(this))
		return;

	ABuilding* target = nullptr;

	if (IsValid(BuildingComponent->House) || IsValid(BuildingComponent->Orphanage)) {
		if (IsValid(BuildingComponent->House))
			target = BuildingComponent->House;
		else
			target = BuildingComponent->Orphanage;

		if (IsValid(BuildingComponent->Employment) && BuildingComponent->Employment->IsA<AExternalProduction>()) {
			for (auto& element : Cast<AExternalProduction>(BuildingComponent->Employment)->GetValidResources()) {
				for (FWorkerStruct workerStruct : element.Key->WorkerStruct) {
					if (!workerStruct.Citizens.Contains(this))
						continue;

					workerStruct.Citizens.Remove(this);

					break;
				}
			}
		}
	}

	if (IsValid(target) && AIController->MoveRequest.GetGoalActor() != target)
		AIController->AIMoveTo(target);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	if (faction->Rebels.Contains(this)) {
		UAIVisualiser* aiVisualiser = Camera->Grid->AIVisualiser;
		aiVisualiser->RemoveInstance(aiVisualiser->HISMRebel, faction->Rebels.Find(this));
		faction->Rebels.Remove(this);

		faction->Citizens.Add(this);
		aiVisualiser->AddInstance(this, aiVisualiser->HISMCitizen, MovementComponent->Transform);
	}
}

void ACitizen::GainEnergy()
{
	Energy = FMath::Clamp(Energy + 1, 0, 100);

	HealthComponent->AddHealth(1);

	if (Energy >= 100 && !bSleep)
		AIController->DefaultAction();
}

//
// Resources
//
void ACitizen::StartHarvestTimer(AResource* Resource)
{
	float time = Camera->Stream.RandRange(6.0f, 10.0f);
	time /= (FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed) * GetProductivity());

	MovementComponent->SetAnimation(EAnim::Melee, true);

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(Resource, params);
	Camera->TimerManager->CreateTimer("Harvest", this, time, "HarvestResource", params, false, true);

	AIController->StopMovement();
}

void ACitizen::HarvestResource(AResource* Resource)
{
	MovementComponent->SetAnimation(EAnim::Still);
	
	AResource* resource = Resource->GetHarvestedResource();

	Camera->DiseaseManager->Injure(this, 99);

	LoseEnergy();

	int32 instance = AIController->MoveRequest.GetGoalInstance();
	int32 amount = FMath::Clamp(Resource->GetYield(this, instance), 0, 10 * GetProductivity());

	if (!Camera->ResourceManager->GetResources(BuildingComponent->Employment).Contains(resource->GetClass())) {
		FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(this);

		if (!AIController->CanMoveTo(faction.EggTimer->GetActorLocation()))
			AIController->DefaultAction();
		else
			Carry(resource, amount, faction.EggTimer);
	}
	else
		Carry(resource, amount, BuildingComponent->Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

	LoseEnergy();

	if (Location == nullptr)
		AIController->StopMovement();
	else
		AIController->AIMoveTo(Location, FVector::Zero(), AIController->MoveRequest.GetGoalInstance());
}

//
// Politics
//
void ACitizen::SetPoliticalLeanings()
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	if (faction->Police.Arrested.Contains(this))
		return;

	TArray<FString> partyList;

	TEnumAsByte<ESway>* sway = nullptr;

	FPartyStruct* party = Camera->PoliticsManager->GetMembersParty(this);

	if (party != nullptr)
		sway = party->Members.Find(this);

	if (sway != nullptr && sway->GetValue() == ESway::Radical)
		return;

	if (faction->Politics.Representatives.Contains(this))
		for (int32 i = 0; i < sway->GetIntValue(); i++)
			partyList.Add(party->Party);

	if (IsValid(BuildingComponent->House)) {
		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA<ABooster>() || (building->GetCapacity() > 0 && building->GetOccupied().IsEmpty()))
				continue;

			ABooster* booster = Cast<ABooster>(building);

			if (!booster->GetAffectedBuildings().Contains(BuildingComponent->House))
				continue;

			for (auto& element : booster->BuildingsToBoost) {
				FPartyStruct p;
				p.Party = element.Value;

				if (faction->Politics.Parties.Contains(p))
					partyList.Add(element.Value);
			}
		}
	}

	int32 itterate = FMath::Floor(HappinessComponent->GetHappiness() / 10.0f) - 5;

	if (itterate < 0)
		for (int32 i = 0; i < FMath::Abs(itterate); i++)
			partyList.Add("Shell Breakers");

	for (FPartyStruct p : faction->Politics.Parties) {
		int32 count = 0;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
			for (FString trait : p.Agreeable) {
				if (personality->Trait == trait)
					count += 2;
				else if (personality->Likes.Contains(trait))
					count++;
				else if (personality->Dislikes.Contains(trait))
					count--;
			}
		}

		if (count > 0)
			for (int32 i = 0; i < count; i++)
				partyList.Add(p.Party);
	}

	if (partyList.IsEmpty())
		return;
	
	int32 index = Camera->Stream.RandRange(0, partyList.Num() - 1);

	bool bLog = false;

	if (party != nullptr && party->Party == partyList[index]) {
		int32 mark = Camera->Stream.RandRange(0, 100);
		int32 pass = 75;

		if (sway->GetValue() == ESway::Strong)
			pass = 95;

		if (mark > pass) {
			if (sway->GetValue() == ESway::Strong)
				party->Members.Emplace(this, ESway::Radical);
			else
				party->Members.Emplace(this, ESway::Strong);

			bLog = true;
		}
	}
	else {
		if (sway != nullptr && sway->GetValue() == ESway::Strong) {
			party->Members.Emplace(this, ESway::Moderate);
		}
		else {
			if (party != nullptr)
				party->Members.Remove(this);

			FPartyStruct partyStruct;
			partyStruct.Party = partyList[index];

			int32 i = faction->Politics.Parties.Find(partyStruct);
			party = &faction->Politics.Parties[i];

			party->Members.Add(this, ESway::Moderate);
			sway = party->Members.Find(this);

			if (party->Party == "Shell Breakers" && Camera->PoliticsManager->IsRebellion(faction))
				Camera->PoliticsManager->SetupRebel(faction, this);

			if (Camera->InfoUIInstance->IsInViewport())
				Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Party, party->Party);
		}

		bLog = true;
	}

	if (bLog)
		Camera->NotifyLog("Neutral", BioComponent->Name + " is now a " + UEnum::GetValueAsString(sway->GetValue()) + " " + party->Party, faction->Name);
}

//
// Religion
//
void ACitizen::SetReligion(FFactionStruct* Faction)
{
	TArray<FString> religionList;

	if (BioComponent->Father != nullptr) {
		Spirituality.FathersFaith = BioComponent->Father->Spirituality.Faith;

		religionList.Add(Spirituality.FathersFaith);
	}

	if (BioComponent->Mother != nullptr) {
		Spirituality.MothersFaith = BioComponent->Mother->Spirituality.Faith;

		religionList.Add(Spirituality.MothersFaith);
	}

	if (BioComponent->Partner != nullptr)
		religionList.Add(BioComponent->Partner->Spirituality.Faith);

	religionList.Add(Spirituality.Faith);

	if (IsValid(BuildingComponent->House)) {
		for (ABuilding* building : Faction->Buildings) {
			if (!building->IsA<ABooster>())
				continue;

			ABooster* booster = Cast<ABooster>(building);

			if (!booster->GetAffectedBuildings().Contains(BuildingComponent->House) && BuildingComponent->Employment != booster)
				continue;

			for (auto& element : booster->BuildingsToBoost) {
				FReligionStruct religion;
				religion.Faith = element.Value;

				if (Camera->CitizenManager->Religions.Contains(religion))
					religionList.Add(element.Value);
			}
		}
	}

	for (FReligionStruct religion : Camera->CitizenManager->Religions) {
		int32 count = 0;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
			for (FString trait : religion.Agreeable) {
				if (personality->Trait == trait)
					count += 2;
				else if (personality->Likes.Contains(trait))
					count++;
				else if (personality->Dislikes.Contains(trait))
					count--;
			}
		}

		if (count > 0)
			for (int32 i = 0; i < count; i++)
				religionList.Add(religion.Faith);
	}

	int32 index = Camera->Stream.RandRange(0, religionList.Num() - 1);

	Spirituality.Faith = religionList[index];

	Camera->NotifyLog("Neutral", BioComponent->Name + " set their faith as " + Spirituality.Faith, Faction->Name);

	if (Camera->InfoUIInstance->IsInViewport())
		Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Religion, Spirituality.Faith);
}

//
// Genetics
//
void ACitizen::GenerateGenetics(FFactionStruct* Faction)
{
	TArray<EGeneticsGrade> grades;

	for (FGeneticsStruct &genetic : Genetics) {
		if (BioComponent->Father != nullptr) {
			int32 index = BioComponent->Father->Genetics.Find(genetic);

			grades.Add(BioComponent->Father->Genetics[index].Grade);
		}

		if (BioComponent->Mother != nullptr) {
			int32 index = BioComponent->Mother->Genetics.Find(genetic);

			grades.Add(BioComponent->Mother->Genetics[index].Grade);
		}

		if (grades.IsEmpty()) {
			grades.Add(EGeneticsGrade::Bad);
			grades.Add(EGeneticsGrade::Neutral);
			grades.Add(EGeneticsGrade::Good);
		}

		int32 choice = Camera->Stream.RandRange(0, grades.Num() - 1);

		genetic.Grade = grades[choice];

		int32 mutate = Camera->Stream.RandRange(1, 100);

		int32 chance = 100 - (Faction->PrayStruct.Bad * 5) - (Faction->PrayStruct.Good * 5);

		if (mutate >= chance)
			continue;

		grades.Empty();

		grades.Add(EGeneticsGrade::Neutral);

		for (int32 i = 0; i <= Faction->PrayStruct.Good; i++)
			grades.Add(EGeneticsGrade::Good);

		for (int32 i = 0; i <= Faction->PrayStruct.Bad; i++)
			grades.Add(EGeneticsGrade::Bad);

		grades.Remove(genetic.Grade);

		choice = Camera->Stream.RandRange(0, grades.Num() - 1);

		genetic.Grade = grades[choice];
	}

	for (FGeneticsStruct& genetic : Genetics)
		ApplyGeneticAffect(genetic);
}

void ACitizen::ApplyGeneticAffect(FGeneticsStruct Genetic)
{
	if (Genetic.Type == EGeneticsType::Speed) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Speed", 1.25f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Speed", 0.75f);
	}
	else if (Genetic.Type == EGeneticsType::Shell) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Health", 1.25f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Health", 0.75f);
	}
	else if (Genetic.Type == EGeneticsType::Reach) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Reach", 1.15f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Reach", 0.85f);
	}
	else if (Genetic.Type == EGeneticsType::Awareness) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Awareness", 1.25f);
		else if (Genetic.Grade == EGeneticsGrade::Bad) {
			ApplyToMultiplier("Awareness", 0.75f);

			bGlasses = true;
		}
	}
	else if (Genetic.Type == EGeneticsType::Productivity) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Productivity", 1.15f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Productivity", 0.85f);
	}
	else {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ApplyToMultiplier("Fertility", 1.15f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Fertility", 0.85f);
	}
}

void ACitizen::Snore(bool bCreate)
{
	float time = Camera->Stream.FRandRange(2.0f, 10.0f);

	if (bCreate) {
		FGeneticsStruct geneticToFind;
		geneticToFind.Type = EGeneticsType::Speed;

		int32 index = Genetics.Find(geneticToFind);

		if (Genetics[index].Grade != EGeneticsGrade::Bad)
			return;

		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(false, params);
		Camera->TimerManager->CreateTimer("Snore", this, time, "Snore", params, true);
	}
	else {
		int32 index = Camera->Stream.RandRange(0, Snores.Num() - 1);

		Camera->PlayAmbientSound(AmbientAudioComponent, Snores[index]);

		Camera->TimerManager->UpdateTimerLength("Snore", this, time);
	}
}

//
// Personality
//
void ACitizen::GivePersonalityTrait(ACitizen* Parent)
{
	if (!IsValid(Camera)) {
		APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		Camera = PController->GetPawn<ACamera>();
	}

	TArray<FPersonality*> parentsPersonalities = Camera->CitizenManager->GetCitizensPersonalities(Parent);
	TArray<FPersonality> personalities;

	int32 chance = Camera->Stream.RandRange(0, 100);

	if (chance >= 45 * parentsPersonalities.Num())
		personalities = Camera->CitizenManager->Personalities;
	else
		for (FPersonality* personality : parentsPersonalities)
			personalities.Add(*personality);

	for (int32 i = personalities.Num() - 1; i > -1; i--) {
		if (!personalities[i].Citizens.Contains(this))
			continue;

		for (int32 j = personalities.Num() - 1; j > -1; j--) {
			if (!personalities[i].Dislikes.Contains(personalities[j].Trait) || (personalities[i].Trait == "Cruel" && personalities[j].Trait != "Kind"))
				continue;

			if (j < i)
				i--;

			personalities.RemoveAt(j);
		}

		personalities.RemoveAt(i);
	}

	int32 index = Camera->Stream.RandRange(0, personalities.Num() - 1);

	int32 i = Camera->CitizenManager->Personalities.Find(personalities[index]);

	Camera->CitizenManager->Personalities[i].Citizens.Add(this);

	ApplyTraitAffect(Camera->CitizenManager->Personalities[i].Affects);

	if (Camera->InfoUIInstance->IsInViewport())
		Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Personality, Camera->CitizenManager->Personalities[i].Trait);
}

void ACitizen::ApplyTraitAffect(TMap<FString, float> Affects)
{
	for (auto& element : Affects)
		ApplyToMultiplier(element.Key, element.Value);
}

//
// Multipliers
//
void ACitizen::ApplyToMultiplier(FString Affect, float Amount)
{
	Amount = Amount - 1.0f;

	int32 timeToCompleteDay = 0; 
	if (Affect == "Hunger" || Affect == "Energy")
		timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	
	if (Affect == "Damage") {
		AttackComponent->DamageMultiplier += Amount;
	}
	else if (Affect == "Speed") {
		MovementComponent->SpeedMultiplier += Amount;
	}
	else if (Affect == "Health") {
		HealthComponent->HealthMultiplier += Amount;

		HealthComponent->MaxHealth = FMath::Clamp(10 + (5 * BioComponent->Age), 0, 100) * HealthComponent->HealthMultiplier;

		if (HealthComponent->Health > HealthComponent->MaxHealth)
			HealthComponent->Health = HealthComponent->MaxHealth;
	}
	else if (Affect == "Fertility") {
		Fertility += Amount;
	}
	else if (Affect == "Productivity") {
		ProductivityMultiplier += Amount;
	}
	else if (Affect == "Reach") {
		ReachMultiplier += Amount;

		MovementComponent->Transform.SetScale3D(MovementComponent->Transform.GetScale3D() * ReachMultiplier);

		Range = InitialRange * AwarenessMultiplier * ReachMultiplier;
	}
	else if (Affect == "Awareness") {
		AwarenessMultiplier += Amount;

		Range = InitialRange * AwarenessMultiplier * ReachMultiplier;
	}
	else if (Affect == "Food") {
		FoodMultiplier += Amount;
	}
	else if (Affect == "Hunger") {
		HungerMultiplier += Amount;

		Camera->TimerManager->UpdateTimerLength("Eat", this, (timeToCompleteDay / 200) * HungerMultiplier);
	}
	else if (Affect == "Energy") {
		EnergyMultiplier += Amount;

		Camera->TimerManager->UpdateTimerLength("Energy", this, (timeToCompleteDay / 100) * EnergyMultiplier);
	}
	else if (Affect == "Ideal Hours Slept") {
		IdealHoursSlept = Amount;
	}
	else if (Affect == "Ideal Work Hours") {
		BuildingComponent->IdealHoursWorkedMin = Amount;
		BuildingComponent->IdealHoursWorkedMax = Amount + 8;
	}
}

float ACitizen::GetProductivity()
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	float speed = FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed);
	float scale = (FMath::Min(BioComponent->Age, 18) * 0.04f) + 0.28f;

	float productivity = (ProductivityMultiplier * (1 + BioComponent->EducationLevel * 0.1)) * scale * speed;

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA<APowerPlant>())
			continue;

		productivity *= (1.0f + 0.05f * building->GetCitizensAtBuilding().Num());

		break;
	}

	if (Camera->Grid->AtmosphereComponent->bRedSun && !Camera->Grid->AtmosphereComponent->NaturalDisasterComponent->IsProtected(MovementComponent->Transform.GetLocation()))
		productivity *= 0.5f;

	return productivity;
}