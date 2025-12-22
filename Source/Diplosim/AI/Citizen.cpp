#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"

#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "AIMovementComponent.h"
#include "BuildingComponent.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/House.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/Misc/Special/PowerPlant.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Map/Resources/Mineral.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Components/BuildComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"

ACitizen::ACitizen()
{
	AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudioComponent"));
	AmbientAudioComponent->SetupAttachment(RootComponent);
	AmbientAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientAudioComponent->bCanPlayMultipleInstances = true;

	BuildingComponent = CreateDefaultSubobject<UBuildingComponent>(TEXT("BuildingComponent"));

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	bGain = false;

	bHasBeenLeader = false;

	bHolliday = false;
	MassStatus = EAttendStatus::Neutral;
	FestivalStatus = EAttendStatus::Neutral;

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

	SpeedBeforeOld = 100.0f;
	MaxHealthBeforeOld = 100.0f;

	bConversing = false;
	ConversationHappiness = 0;

	VoicePitch = 1.0f;

	bGlasses = false;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	Camera->TimerManager->CreateTimer("Birthday", this, timeToCompleteDay / 10.0f, "Birthday", {}, true);

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

	if (BioStruct.Mother != nullptr && BioStruct.Mother->BuildingComponent->BuildingAt != nullptr)
		BioStruct.Mother->BuildingComponent->BuildingAt->Enter(this);

	GenerateGenetics(Faction);
	ApplyResearch(Faction);

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
			if (BioStruct.Partner != nullptr) {
				BuildingComponent->House->RemoveVisitor(this, Cast<ACitizen>(BioStruct.Partner));

				int32 index = BuildingComponent->House->GetOccupied().Find(this);

				BuildingComponent->House->Occupied[index].Occupant = Cast<ACitizen>(BioStruct.Partner);
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

	if (BioStruct.Partner != nullptr) {
		BioStruct.Partner->BioStruct.Partner = nullptr;
		BioStruct.Partner = nullptr;
	}

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
	if (BioStruct.EducationLevel < BioStruct.PaidForEducationLevel)
		return true;
	
	int32 money = 0;
	int32 leftoverMoney = GetLeftoverMoney();

	if (leftoverMoney > 0)
		money += leftoverMoney;

	for (ACitizen* citizen : GetLikedFamily(false)) {
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
	if (BioStruct.EducationLevel < BioStruct.PaidForEducationLevel)
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	TMap<ACitizen*, int32> wallet;
	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Education Cost");

	int32 leftoverMoney = GetLeftoverMoney();

	if (cost > 0) {
		if (leftoverMoney < cost) {
			for (ACitizen* citizen : GetLikedFamily(false)) {
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

	BioStruct.PaidForEducationLevel++;
}

int32 ACitizen::GetLeftoverMoney()
{
	int32 money = Balance;

	if (IsValid(BuildingComponent->House))
		money -= BuildingComponent->House->Rent;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);
	int32 cost = Camera->PoliticsManager->GetLawValue(faction->Name, "Food Cost");

	for (ACitizen* child : BioStruct.Children) {
		int32 maxF = FMath::CeilToInt((100 - child->Hunger) / (25.0f * child->FoodMultiplier));

		int32 modifier = 1;

		if (BioStruct.Partner != nullptr && IsValid(BioStruct.Partner->BuildingComponent->Employment))
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
			for (ACitizen* citizen : GetLikedFamily(true)) {
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
// Bio
//
void ACitizen::Birthday()
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	BioStruct.Age++;

	if (BioStruct.Age == 5)
		GivePersonalityTrait(BioStruct.Mother.Get());
	else if (BioStruct.Age == 10)
		GivePersonalityTrait(BioStruct.Father.Get());

	BuildingComponent->RemoveFromHouse();

	if (BioStruct.Age > 50) {
		float ratio = FMath::Clamp(FMath::Pow(FMath::LogX(50.0f, BioStruct.Age - 50.0f), 3.0f), 0.0f, 1.0f);
		float odds = ratio * 90.0f;

		HealthComponent->MaxHealth = MaxHealthBeforeOld / (ratio * 5.0f);
		HealthComponent->AddHealth(0);

		MovementComponent->InitialSpeed = SpeedBeforeOld / (ratio * 2.0f);

		int32 chance = Camera->Stream.RandRange(1, 100);

		if (chance < odds)
			HealthComponent->TakeHealth(HealthComponent->MaxHealth, this);
	}
	else if (BioStruct.Age == 50) {
		MaxHealthBeforeOld = HealthComponent->MaxHealth;
		SpeedBeforeOld = MovementComponent->InitialSpeed;
	}

	if (BioStruct.Age <= 18) {
		HealthComponent->MaxHealth += 5 * HealthComponent->HealthMultiplier;
		HealthComponent->AddHealth(5 * HealthComponent->HealthMultiplier);

		float multiply = 1.0f;
		if (Hunger <= 25)
			multiply = 0.75f;

		float scale = (BioStruct.Age * 0.04f) + 0.28f;

		MovementComponent->Transform.SetScale3D(FVector(scale, scale, scale) * FVector(multiply, multiply, 1.0f) * ReachMultiplier);
	}
	else if (BioStruct.Partner != nullptr && BioStruct.Sex == ESex::Female)
		AsyncTask(ENamedThreads::GameThread, [this]() { HaveChild(); });

	if (faction == nullptr)
		return;

	if (BioStruct.Age == 18) {
		SetSexuality(faction->Citizens);

		SetReligion(faction);
	}

	if (BioStruct.Age >= 18 && BioStruct.Partner == nullptr)
		FindPartner(faction);

	BuildingComponent->RemoveOnReachingWorkAge(faction);

	if (BioStruct.Age >= Camera->PoliticsManager->GetLawValue(faction->Name, "Vote Age"))
		SetPoliticalLeanings();
}

void ACitizen::SetSex(TArray<ACitizen*> Citizens)
{
	int32 choice = Camera->Stream.RandRange(1, 100);

	float male = 0.0f;
	float total = 0.0f;

	for (AActor* actor : Citizens) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen == this)
			continue;

		if (citizen->BioStruct.Sex == ESex::Male)
			male += 1.0f;

		total += 1.0f;
	}

	float mPerc = 50.0f;

	if (total > 0)
		mPerc = (male / total) * 100.0f;

	if (choice > mPerc)
		BioStruct.Sex = ESex::Male;
	else
		BioStruct.Sex = ESex::Female;

	SetName();
}

void ACitizen::SetName()
{
	FString names;
		
	if (BioStruct.Sex == ESex::Male)
		FFileHelper::LoadFileToString(names, *(FPaths::ProjectDir() + "/Content/Custom/Citizen/MaleNames.txt"));
	else
		FFileHelper::LoadFileToString(names, *(FPaths::ProjectDir() + "/Content/Custom/Citizen/FemaleNames.txt"));

	TArray<FString> parsed;
	names.ParseIntoArray(parsed, TEXT(","));

	int32 index = Camera->Stream.RandRange(0, parsed.Num() - 1);

	BioStruct.Name = parsed[index];
}

void ACitizen::SetSexuality(TArray<ACitizen*> Citizens)
{
	if (Citizens.Num() <= 10) {
		BioStruct.Sexuality = ESexuality::Straight;

		return;
	}

	int32 gays = 0;

	Citizens.Remove(this);

	for (ACitizen* citizen : Citizens)
		if (citizen->BioStruct.Sexuality != ESexuality::NaN && citizen->BioStruct.Sexuality != ESexuality::Straight)
			gays++;

	float percentageGay = gays / (float)Citizens.Num();

	if (percentageGay > 0.1f)
		return;

	int32 chance = Camera->Stream.RandRange(1, 100);

	if (chance > 90) {
		if (chance > 95) {
			BioStruct.Sexuality = ESexuality::Homosexual;

			Fertility *= 0.5f;
		}
		else {
			BioStruct.Sexuality = ESexuality::Bisexual;
		}
	}
	else {
		BioStruct.Sexuality = ESexuality::Straight;
	}
}

void ACitizen::FindPartner(FFactionStruct* Faction)
{
	ACitizen* citizen = nullptr;
	int32 curCount = 1;

	for (ACitizen* c : Faction->Citizens) {
		if (!IsValid(c) || c == this || c->HealthComponent->GetHealth() == 0 || c->IsPendingKillPending() || c->BioStruct.Partner != nullptr || c->BioStruct.Age < 18)
			continue;

		int32 value = Camera->PoliticsManager->GetLawValue(Faction->Name, "Same-Sex Laws");

		if (((BioStruct.Sexuality == ESexuality::Straight || value == 0) && c->BioStruct.Sex == BioStruct.Sex) || (BioStruct.Sexuality != ESexuality::Straight && value != 0 && c->BioStruct.Sex != BioStruct.Sex))
			continue;

		int32 count = 0;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
			for (FPersonality* p : Camera->CitizenManager->GetCitizensPersonalities(c)) {
				if (personality->Trait == p->Trait)
					count += 2;
				else if (personality->Likes.Contains(p->Trait))
					count++;
				else if (personality->Dislikes.Contains(p->Trait))
					count--;
			}
		}

		if (count < 1)
			continue;

		if (citizen == nullptr) {
			citizen = c;
			curCount = count;

			continue;
		}

		double magnitude = AIController->GetClosestActor(50.0f, MovementComponent->Transform.GetLocation(), citizen->MovementComponent->Transform.GetLocation(), c->MovementComponent->Transform.GetLocation(), true, curCount, count);

		if (magnitude <= 0.0f)
			continue;

		citizen = c;
	}

	if (citizen != nullptr) {
		SetPartner(citizen);

		citizen->SetPartner(this);

		BuildingComponent->SelectPreferredPartnersHouse(this, citizen);
	}
}

void ACitizen::SetPartner(ACitizen* Citizen)
{
	BioStruct.Partner = Citizen;
	BioStruct.HoursTogetherWithPartner = 0;
}

void ACitizen::RemoveMarriage()
{
	if (BioStruct.Partner == nullptr)
		return;

	BioStruct.bMarried = false;
	DivorceHappiness = -20;

	BioStruct.Partner->BioStruct.bMarried = false;
	BioStruct.Partner->DivorceHappiness = -20;
}

void ACitizen::RemovePartner()
{
	if (BioStruct.Partner == nullptr)
		return;

	RemoveMarriage();

	BioStruct.Partner->BioStruct.Partner = nullptr;
	BioStruct.Partner = nullptr;
}

void ACitizen::IncrementHoursTogetherWithPartner()
{
	if (BioStruct.Partner == nullptr)
		return;

	BioStruct.HoursTogetherWithPartner++;
}

void ACitizen::HaveChild()
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	if (!IsValid(BuildingComponent->House) || BioStruct.Children.Num() >= Camera->PoliticsManager->GetLawValue(faction->Name, "Child Policy"))
		return;

	ACitizen* occupant = nullptr;

	if (IsValid(BuildingComponent->House)) {
		occupant = BuildingComponent->House->GetOccupant(this);

		if (BuildingComponent->House->GetVisitors(occupant).Num() == BuildingComponent->House->Space)
			return;
	}
	
	float chance = Camera->Stream.FRandRange(0.0f, 100.0f) * BioStruct.Partner->Fertility * Fertility;
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance < passMark)
		return;

	FVector location = MovementComponent->Transform.GetLocation() + MovementComponent->Transform.GetRotation().Vector() * 10.0f;

	if (BuildingComponent->BuildingAt != nullptr)
		location = BuildingComponent->EnterLocation;

	FActorSpawnParameters params;
	params.bNoFail = true;

	FTransform transform;
	transform.SetLocation(location);
	transform.SetRotation(GetActorRotation().Quaternion());
	transform.SetScale3D(FVector(0.28f));

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), FVector::Zero(), FRotator::ZeroRotator, params);
	Camera->Grid->AIVisualiser->AddInstance(citizen, Camera->Grid->AIVisualiser->HISMCitizen, transform);

	citizen->BioStruct.Mother = this;
	citizen->BioStruct.Father = BioStruct.Partner;

	citizen->SetSex(faction->Citizens);
	citizen->CitizenSetup(faction);

	if (IsValid(occupant))
		citizen->BuildingComponent->House->AddVisitor(occupant, citizen);

	Camera->NotifyLog("Good", citizen->BioStruct.Name + " is born", faction->Name);

	for (ACitizen* child : BioStruct.Children) {
		citizen->BioStruct.Siblings.Add(child);
		child->BioStruct.Siblings.Add(citizen);
	}

	BioStruct.Children.Add(citizen);
	BioStruct.Partner->BioStruct.Children.Add(citizen);
}

TArray<ACitizen*> ACitizen::GetLikedFamily(bool bFactorAge)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	TArray<ACitizen*> family;

	if (BioStruct.Mother != nullptr && BioStruct.Mother->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(BioStruct.Mother));

	if (BioStruct.Father != nullptr && BioStruct.Father->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(BioStruct.Father));

	if (BioStruct.Partner != nullptr && BioStruct.Partner->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(BioStruct.Partner));

	for (ACitizen* child : BioStruct.Children)
		if (IsValid(child) && child->HealthComponent->GetHealth() != 0)
			family.Add(child);

	for (ACitizen* sibling : BioStruct.Siblings)
		if (IsValid(sibling) && sibling->HealthComponent->GetHealth() != 0)
			family.Add(sibling);

	if (bFactorAge && BioStruct.Age < Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age"))
		return family;

	for (int32 i = (family.Num() - 1); i > -1; i--) {
		ACitizen* citizen = family[i];
		int32 likeness = 0;

		Camera->CitizenManager->PersonalityComparison(this, citizen, likeness);

		if (likeness < 0)
			family.RemoveAt(i);
	}

	return family;
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

	int32 itterate = FMath::Floor(GetHappiness() / 10.0f) - 5;

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
		}

		bLog = true;
	}

	if (bLog)
		Camera->NotifyLog("Neutral", BioStruct.Name + " is now a " + UEnum::GetValueAsString(sway->GetValue()) + " " + party->Party, faction->Name);
}

//
// Religion
//
void ACitizen::SetReligion(FFactionStruct* Faction)
{
	TArray<FString> religionList;

	if (BioStruct.Father != nullptr) {
		Spirituality.FathersFaith = BioStruct.Father->Spirituality.Faith;

		religionList.Add(Spirituality.FathersFaith);
	}

	if (BioStruct.Mother != nullptr) {
		Spirituality.MothersFaith = BioStruct.Mother->Spirituality.Faith;

		religionList.Add(Spirituality.MothersFaith);
	}

	if (BioStruct.Partner != nullptr)
		religionList.Add(BioStruct.Partner->Spirituality.Faith);

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

	Camera->NotifyLog("Neutral", BioStruct.Name + " set their faith as " + Spirituality.Faith, Faction->Name);
}

//
// Happiness
//
void ACitizen::SetAttendStatus(EAttendStatus Status, bool bMass)
{
	if (bMass)
		MassStatus = Status;
	else
		FestivalStatus = Status;

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(EAttendStatus::Neutral, params);
	Camera->TimerManager->SetParameter(bMass, params);
	Camera->TimerManager->CreateTimer("Mass", this, timeToCompleteDay * 2, "SetAttendStatus", params, false);
}

void ACitizen::SetHolliday(bool bStatus)
{
	bHolliday = bStatus;
}

void ACitizen::SetDecayHappiness(int32* HappinessToDecay, int32 Amount, int32 Min, int32 Max)
{
	int32 value = FMath::Clamp(*HappinessToDecay + Amount, Min, Max);

	HappinessToDecay = &value;
}

void ACitizen::DecayHappiness()
{
	TArray<int32*> happinessToDecay = {&ConversationHappiness, &FamilyDeathHappiness, &WitnessedDeathHappiness, &DivorceHappiness};

	for (int32* happiness : happinessToDecay) {
		if (*happiness < 0)
			happiness++;
		else if (*happiness > 0)
			happiness--;
	}
}

int32 ACitizen::GetHappiness()
{
	int32 value = 50;

	TArray<int32> values;
	Happiness.Modifiers.GenerateValueArray(values);

	for (int32 v : values)
		value += v;

	return value;
}

void ACitizen::SetHappiness()
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", this);

	Happiness.ClearValues();

	if (!IsValid(BuildingComponent->House))
		Happiness.SetValue("Homeless", -20);
	else {
		Happiness.SetValue("Housed", 10);

		int32 satisfaction = BuildingComponent->House->GetSatisfactionLevel();
		int32 level = (BuildingComponent->House->GetSatisfactionLevel() / 5 - 10) * 2;

		if (level > 0)
			level *= 0.75f;

		FString message = "";

		if (satisfaction < 25)
			message = "Awful Housing Satisfaction";
		else if (satisfaction < 50)
			message = "Substandard Housing Satisfaction";
		else if (satisfaction > 50)
			message = "Good Housing Satisfaction";
		else if (satisfaction > 75)
			message = "Very High Housing Satisfaction";

		Happiness.SetValue(message, level);

		bool bIsCruel = false;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this))
			if (personality->Trait == "Cruel")
				bIsCruel = true;

		bool isThereAnyBoosters = false;
		bool bNearbyFaith = false;
		bool bPropaganda = true;
		bool bIsPark = false;
		
		for (ABuilding* building : faction->Buildings) {
			if (!building->IsA<ABooster>())
				continue;

			ABooster* booster = Cast<ABooster>(building);

			if (!booster->GetAffectedBuildings().Contains(BuildingComponent->House))
				continue;

			isThereAnyBoosters = true;

			if (booster->DoesPromoteFavouringValues(this))
				bPropaganda = false;

			for (auto& element : booster->BuildingsToBoost) {

				if (element.Value == Spirituality.Faith && booster->bHolyPlace)
					bNearbyFaith = true;

				if (element.Value == "Happiness")
					bIsPark = true;
			}
		}

		if (Spirituality.Faith != "Atheist") {
			if (!bNearbyFaith)
				Happiness.SetValue("No Working Holy Place Nearby", -25);
			else
				Happiness.SetValue("Holy Place Nearby", 15);
		}

		if (bIsPark) {
			if (bIsCruel)
				Happiness.SetValue("Park Nearby", -5);
			else
				Happiness.SetValue("Park Nearby", 5);
		}
		else if (!bIsCruel)
			Happiness.SetValue("No Nearby Park", -10);

		if (isThereAnyBoosters) {
			if (bPropaganda)
				Happiness.SetValue("Nearby propaganda tower", -25);
			else
				Happiness.SetValue("Nearby eggtastic tower", 15);
		}
	}

	if (BioStruct.Age >= Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age")) {
		if (BuildingComponent->Employment == nullptr)
			Happiness.SetValue("Unemployed", -10);
		else
			Happiness.SetValue("Employed", 5);

		int32 hours = BuildingComponent->HoursWorked.Num();

		if (hours < BuildingComponent->IdealHoursWorkedMin || hours > BuildingComponent->IdealHoursWorkedMax)
			Happiness.SetValue("Inadequate Hours Worked", -15);
		else
			Happiness.SetValue("Ideal Hours Worked", 10);

		if (Balance < 5)
			Happiness.SetValue("Poor", -25);
		else if (Balance >= 5 && Balance < 15)
			Happiness.SetValue("Well Off", 10);
		else
			Happiness.SetValue("Rich", 20);

		if (IsValid(BuildingComponent->Employment) && GetWorld()->GetTimeSeconds() >= BuildingComponent->GetAcquiredTime(1) + 300.0f) {
			int32 count = 0;

			for (ACitizen* citizen : BuildingComponent->Employment->GetOccupied()) {
				if (citizen == this)
					continue;

				for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
					for (FPersonality* p : Camera->CitizenManager->GetCitizensPersonalities(citizen)) {
						if (personality->Trait == p->Trait)
							count += 2;
						else if (personality->Likes.Contains(p->Trait))
							count++;
						else if (personality->Dislikes.Contains(p->Trait))
							count--;
					}
				}
			}

			Happiness.SetValue("Work Happiness", count * 5);
		}
	}

	if (BioStruct.Sexuality == ESexuality::Homosexual || BioStruct.Sexuality == ESexuality::Bisexual) {
		int32 lawValue = Camera->PoliticsManager->GetLawValue(faction->Name, "Same-Sex Laws");
		int32 sameSexHappiness = -20 + 10 * (lawValue + FMath::Floor(lawValue / 2));

		if (BioStruct.Sexuality == ESexuality::Bisexual) {
			if (sameSexHappiness < 0)
				sameSexHappiness += 5;
			else
				sameSexHappiness -= 5;
		}

		Happiness.SetValue("Same-Sex Laws", sameSexHappiness);
	}

	if (Hunger < 20)
		Happiness.SetValue("Hungry", -30);
	else if (Hunger > 70)
		Happiness.SetValue("Well Fed", 10);

	if (Energy < 20)
		Happiness.SetValue("Tired", -15);
	else if (Energy > 70)
		Happiness.SetValue("Rested", 10);

	if (HoursSleptToday.Num() < IdealHoursSlept - 2)
		Happiness.SetValue("Disturbed Sleep", -15);
	else if (HoursSleptToday.Num() >= IdealHoursSlept)
		Happiness.SetValue("Slept Like A Baby", 10);

	if (MassStatus == EAttendStatus::Missed)
		Happiness.SetValue("Missed Mass", -25);
	else if (MassStatus == EAttendStatus::Attended)
		Happiness.SetValue("Attended Mass", 15);

	if (FestivalStatus == EAttendStatus::Missed)
		Happiness.SetValue("Missed Festival", -5);
	else if (FestivalStatus == EAttendStatus::Attended)
		Happiness.SetValue("Attended Festival", 15);

	if (bHolliday)
		Happiness.SetValue("Holliday", 10);

	if (ConversationHappiness < 0)
		Happiness.SetValue("Recent arguments", ConversationHappiness);
	else if (ConversationHappiness > 0)
		Happiness.SetValue("Recent conversations", ConversationHappiness);

	FString party = Camera->PoliticsManager->GetCitizenParty(this);

	if (party != "Undecided") {
		int32 lawTally = 0;

		for (FLawStruct law : faction->Politics.Laws) {
			if (law.BillType == "Abolish")
				continue;

			int32 count = 0;

			FLeanStruct partyLean;
			partyLean.Party = party;

			int32 index = law.Lean.Find(partyLean);

			if (index == INDEX_NONE)
				continue;

			if (!law.Lean[index].ForRange.IsEmpty() && Camera->PoliticsManager->IsInRange(law.Lean[index].ForRange, law.Value))
				count++;
			else if (!law.Lean[index].AgainstRange.IsEmpty() && Camera->PoliticsManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
				count--;

			for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
				FLeanStruct personalityLean;
				personalityLean.Personality = personality->Trait;

				index = law.Lean.Find(personalityLean);

				if (index == INDEX_NONE)
					continue;

				if (!law.Lean[index].ForRange.IsEmpty() && Camera->PoliticsManager->IsInRange(law.Lean[index].ForRange, law.Value))
					count++;
				else if (!law.Lean[index].AgainstRange.IsEmpty() && Camera->PoliticsManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
					count--;
			}

			if (count < 0)
				lawTally--;
			else
				lawTally++;
		}

		if (lawTally < -1)
			Happiness.SetValue("Not Represented", -20);
		else if (lawTally > 1)
			Happiness.SetValue("Represented", 15);
	}

	if (FamilyDeathHappiness != 0)
		Happiness.SetValue("Recent family death", FamilyDeathHappiness);

	if (WitnessedDeathHappiness != 0)
		Happiness.SetValue("Witnessed death", WitnessedDeathHappiness);

	if (DivorceHappiness != 0)
		Happiness.SetValue("Recently divorced", DivorceHappiness);

	if (Camera->ConquestManager->GetCitizenFaction(this).WarFatigue >= 120)
		Happiness.SetValue("High War Fatigue", -15);

	if (GetHappiness() < 35 && !faction->Police.Arrested.Contains(this)) {
		if (SadTimer == 0)
			Camera->NotifyLog("Bad", BioStruct.Name + " is sad", faction->Name);

		SadTimer++;
	}
	else {
		SadTimer = 0;
	}

	if (SadTimer == 300 && !Camera->EventsManager->UpcomingProtest(faction)) {
		int32 startHour = Camera->Stream.RandRange(6, 9);
		int32 endHour = Camera->Stream.RandRange(12, 18);

		TArray<int32> hours;
		for (int32 i = startHour; i < endHour; i++)
			hours.Add(i);

		Camera->EventsManager->CreateEvent(faction->Name, EEventType::Protest, nullptr, nullptr, "", 0, hours, false, {});
	}
}

//
// Genetics
//
void ACitizen::GenerateGenetics(FFactionStruct* Faction)
{
	TArray<EGeneticsGrade> grades;

	for (FGeneticsStruct &genetic : Genetics) {
		if (BioStruct.Father != nullptr) {
			int32 index = BioStruct.Father->Genetics.Find(genetic);

			grades.Add(BioStruct.Father->Genetics[index].Grade);
		}

		if (BioStruct.Mother != nullptr) {
			int32 index = BioStruct.Mother->Genetics.Find(genetic);

			grades.Add(BioStruct.Mother->Genetics[index].Grade);
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

		HealthComponent->MaxHealth = FMath::Clamp(10 + (5 * BioStruct.Age), 0, 100) * HealthComponent->HealthMultiplier;

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
	float scale = (FMath::Min(BioStruct.Age, 18) * 0.04f) + 0.28f;

	float productivity = (ProductivityMultiplier * (1 + BioStruct.EducationLevel * 0.1)) * scale * speed;

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