#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"

#include "Universal/Resource.h"
#include "Universal/HealthComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Components/BuildComponent.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/House.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "AIMovementComponent.h"
#include "Map/Resources/Mineral.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = true;
	SetTickableWhenPaused(true);

	Capsule->SetCapsuleSize(9.0f, 11.5f);
	Capsule->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2);

	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.5f));
	Mesh->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.28f));

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
	HatMesh->SetWorldScale3D(FVector(0.1f, 0.1f, 0.1f));
	HatMesh->SetCollisionProfileName("NoCollision", false);
	HatMesh->SetComponentTickEnabled(false);
	HatMesh->SetupAttachment(Mesh, "HatSocket");

	TorchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TorchMesh"));
	TorchMesh->SetCollisionProfileName("NoCollision", false);
	TorchMesh->SetupAttachment(Mesh, "TorchSocket");
	TorchMesh->SetHiddenInGame(true);
	TorchMesh->PrimaryComponentTick.bCanEverTick = false;

	TorchNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TorchNiagaraComponent"));
	TorchNiagaraComponent->SetupAttachment(TorchMesh, "ParticleSocket");
	TorchNiagaraComponent->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.12f));
	TorchNiagaraComponent->bAutoActivate = false;

	HarvestNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HarvestNiagaraComponent"));
	HarvestNiagaraComponent->SetupAttachment(Mesh);
	HarvestNiagaraComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 17.0f));
	HarvestNiagaraComponent->bAutoActivate = false;

	DiseaseNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DiseaseNiagaraComponent"));
	DiseaseNiagaraComponent->SetupAttachment(Mesh);
	DiseaseNiagaraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
	DiseaseNiagaraComponent->bAutoActivate = false;

	PopupComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PopupComponent"));
	PopupComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PopupComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	PopupComponent->SetHiddenInGame(true);
	PopupComponent->SetComponentTickEnabled(false);
	PopupComponent->SetGenerateOverlapEvents(false);
	PopupComponent->SetupAttachment(RootComponent);

	AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudioComponent"));
	AmbientAudioComponent->SetupAttachment(RootComponent);
	AmbientAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientAudioComponent->bCanPlayMultipleInstances = true;
	AmbientAudioComponent->PitchModulationMin = 0.9f;
	AmbientAudioComponent->PitchModulationMax = 1.1f;

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	TimeOfAcquirement.Init(-1000.0f, 3);

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

	IdealHoursWorkedMin = 4;
	IdealHoursWorkedMax = 12;

	SpeedBeforeOld = 100.0f;
	MaxHealthBeforeOld = 100.0f;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	for (FResearchStruct research : Camera->ResearchManager->ResearchStruct)
		if (research.bResearched)
			for (auto& element : research.Modifiers)
				ApplyToMultiplier(element.Key, element.Value);

	Camera->CitizenManager->Citizens.Add(this);
	Camera->CitizenManager->Infectible.Add(this);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;

	timer.CreateTimer("Eat", this, (timeToCompleteDay / 200) * HungerMultiplier, FTimerDelegate::CreateUObject(this, &ACitizen::Eat), true, true);
	Camera->CitizenManager->Timers.Add(timer);

	timer.CreateTimer("Energy", this, (timeToCompleteDay / 100) * EnergyMultiplier, FTimerDelegate::CreateUObject(this, &ACitizen::CheckGainOrLoseEnergy), true);
	Camera->CitizenManager->Timers.Add(timer);

	timer.CreateTimer("Birthday", this, timeToCompleteDay / 10, FTimerDelegate::CreateUObject(this, &ACitizen::Birthday), true);
	Camera->CitizenManager->Timers.Add(timer);

	SetSex();
	SetName();

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	Mesh->SetMaterial(0, material);

	if (BioStruct.Mother != nullptr && BioStruct.Mother->Building.BuildingAt != nullptr)
		BioStruct.Mother->Building.BuildingAt->Enter(this);

	AIController->ChooseIdleBuilding(this);
	AIController->DefaultAction();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	if (settings->GetRenderTorches())
		SetTorch(Camera->Grid->AtmosphereComponent->Calendar.Hour);

	GenerateGenetics();
}

//
// Find Job, House and Education
//
void ACitizen::FindEducation(class ASchool* Education, int32 TimeToCompleteDay)
{
	if (!IsValid(AllocatedBuildings[0]))
		SetAcquiredTime(0, -1000.0f);
	
	if (GetWorld()->GetTimeSeconds() < GetAcquiredTime(0) + TimeToCompleteDay || BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::WorkAge) || BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::EducationAge) || BioStruct.EducationLevel == 5 || !CanAffordEducationLevel() || Education->GetOccupied().IsEmpty())
		return;

	ABuilding* chosenSchool = AllocatedBuildings[0];

	if (!IsValid(chosenSchool)) {
		AllocatedBuildings[0] = Education;
	}
	else {
		FVector location = GetActorLocation();

		if (IsValid(AllocatedBuildings[2]))
			location = AllocatedBuildings[2]->GetActorLocation();

		double magnitude = AIController->GetClosestActor(400.0f, location, chosenSchool->GetActorLocation(), Education->GetActorLocation(), true, 2, 1);

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[0] = Education;
	}
}

void ACitizen::FindJob(class AWork* Job, int32 TimeToCompleteDay)
{
	if (!IsValid(AllocatedBuildings[1]))
		SetAcquiredTime(1, -1000.0f);

	if (GetWorld()->GetTimeSeconds() < GetAcquiredTime(1) + TimeToCompleteDay || Job->GetCapacity() == Job->GetOccupied().Num() || !CanWork(Job) || !WillWork())
		return;

	AWork* chosenWorkplace = Cast<AWork>(AllocatedBuildings[1]);

	if (!IsValid(chosenWorkplace)) {
		AllocatedBuildings[1] = Job;
	}
	else {
		int32 diff = Job->Wage - chosenWorkplace->Wage;

		int32* happiness = Happiness.Modifiers.Find("Work Happiness");

		if (happiness != nullptr)
			diff -= *happiness / 5;

		FVector location = GetActorLocation();

		if (IsValid(AllocatedBuildings[1]))
			location = AllocatedBuildings[1]->GetActorLocation();

		int32 currentValue = 1;
		int32 newValue = 1;

		if (diff < 0)
			currentValue += FMath::Abs(diff);
		else if (diff > 0)
			newValue += diff;

		auto magnitude = AIController->GetClosestActor(400.0f, location, chosenWorkplace->GetActorLocation(), Job->GetActorLocation(), true, currentValue, newValue);

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[1] = Job;
	}
}

void ACitizen::FindHouse(class AHouse* House, int32 TimeToCompleteDay)
{
	if (!IsValid(AllocatedBuildings[2]))
		SetAcquiredTime(2, -1000.0f);

	if (GetWorld()->GetTimeSeconds() < GetAcquiredTime(2) + TimeToCompleteDay || House->GetCapacity() == House->GetOccupied().Num() || Balance < House->Rent && (!IsValid(Building.Employment) || Building.Employment->Wage < House->Rent))
		return;

	AHouse* chosenHouse = Cast<AHouse>(AllocatedBuildings[2]);

	if (!IsValid(chosenHouse)) {
		AllocatedBuildings[2] = House;
	}
	else {
		int32 curLeftoverMoney = 0;

		if (IsValid(Building.Employment))
			curLeftoverMoney = Building.Employment->Wage;
		else
			curLeftoverMoney = Balance;

		int32 spaceRequired = 0;

		if (IsValid(Building.House) && Building.House->GetOccupied().Contains(this)) {
			TArray<ACitizen*> visitors = Building.House->GetVisitors(this);

			for (ACitizen* citizen : visitors)
				curLeftoverMoney += Balance;

			spaceRequired = visitors.Num();
		}

		if (House->Space < spaceRequired)
			return;

		int32 newLeftoverMoney = (curLeftoverMoney - House->Rent) * 50;

		curLeftoverMoney -= chosenHouse->Rent;
		curLeftoverMoney *= 50;

		int32 currentValue = FMath::Max(chosenHouse->GetQuality() + curLeftoverMoney, 0);
		int32 newValue = FMath::Max(House->GetQuality() + newLeftoverMoney, 0);

		FVector workLocation = GetActorLocation();

		if (IsValid(Building.Employment))
			workLocation = Building.Employment->GetActorLocation();

		double magnitude = AIController->GetClosestActor(400.0f, workLocation, chosenHouse->GetActorLocation(), House->GetActorLocation(), true, currentValue, newValue);

		if (BioStruct.Partner->IsValidLowLevelFast() && IsValid(BioStruct.Partner->Building.Employment)) {
			FVector partnerWorkLoc = BioStruct.Partner->Building.Employment->GetActorLocation();

			double m = AIController->GetClosestActor(400.0f, partnerWorkLoc, chosenHouse->GetActorLocation(), House->GetActorLocation(), true, currentValue, newValue);

			magnitude += m;
		}

		if (magnitude <= 0.0f)
			return;

		AllocatedBuildings[2] = House;
	}
}

void ACitizen::SetJobHouseEducation(int32 TimeToCompleteDay)
{
	for (int32 i = 0; i < AllocatedBuildings.Num(); i++) {
		ABuilding* building = AllocatedBuildings[i];

		if (!IsValid(building) || building == Building.School || building == Building.Employment || building == Building.House)
			continue;

		TArray<ACitizen*> roommates;

		if (i == 0 && IsValid(Building.School))
			Building.School->RemoveVisitor(Building.School->GetOccupant(this), this);
		else if (i == 1 && IsValid(Building.Employment))
			Building.Employment->RemoveCitizen(this);
		else if (i == 2 && IsValid(Building.House)) {
			if (Building.House->GetOccupied().Contains(this)) {
				roommates.Append(Building.House->GetVisitors(this));

				Building.House->RemoveCitizen(this);
			}
			else {
				Building.House->RemoveVisitor(Building.House->GetOccupant(this), this);
			}
		}

		if (i == 0)
			Cast<ASchool>(building)->AddVisitor(Cast<ASchool>(building)->GetOccupant(this), this);
		else
			building->AddCitizen(this);

		if (i == 2) {
			if (IsValid(Building.Orphanage))
				Building.Orphanage->RemoveVisitor(Building.Orphanage->GetOccupant(this), this);

			for (ACitizen* citizen : roommates)
				building->AddVisitor(this, citizen);

			if (BioStruct.Partner->IsValidLowLevelFast() && !roommates.Contains(BioStruct.Partner))
				building->AddVisitor(this, Cast<ACitizen>(BioStruct.Partner));
		}

		SetAcquiredTime(i, GetWorld()->GetTimeSeconds());
	}
}

float ACitizen::GetAcquiredTime(int32 Index)
{
	return TimeOfAcquirement[Index];
}

void ACitizen::SetAcquiredTime(int32 Index, float Time)
{
	TimeOfAcquirement[Index] = Time;
}

bool ACitizen::CanFindAnything(int32 TimeToCompleteDay)
{
	if (GetWorld()->GetTimeSeconds() < GetAcquiredTime(0) + TimeToCompleteDay && GetWorld()->GetTimeSeconds() < GetAcquiredTime(1) + TimeToCompleteDay && GetWorld()->GetTimeSeconds() < GetAcquiredTime(2) + TimeToCompleteDay)
		return false;

	return true;
}

//
// On Hit
//
void ACitizen::SetHarvestVisuals(AResource* Resource)
{
	USoundBase* sound = nullptr;

	FHitResult hit(ForceInit);
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	FLinearColor colour;

	if (Resource->IsA<AMineral>()) {
		sound = Mines[FMath::RandRange(0, Mines.Num() - 1)];

		FResourceStruct resourceStruct;
		resourceStruct.Type = Resource->GetClass();

		int32 index = Camera->ResourceManager->ResourceList.Find(resourceStruct);
		FString category = Camera->ResourceManager->ResourceList[index].Category;

		if (category == "Stone")
			colour = FLinearColor(0.571125f, 0.590619f, 0.64448f);
		else if (category == "Marble")
			colour = FLinearColor(0.768151f, 0.73791f, 0.610496f);
		else if (category == "Iron")
			colour = FLinearColor(0.291771f, 0.097587f, 0.066626f);
		else
			colour = FLinearColor(1.0f, 0.672443f, 0.0f);
	}
	else {
		sound = Chops[FMath::RandRange(0, Chops.Num() - 1)];

		colour = FLinearColor(0.270498f, 0.158961f, 0.07036f);
	}

	HarvestNiagaraComponent->SetVariableLinearColor(TEXT("Colour"), colour);
	HarvestNiagaraComponent->Activate();

	Camera->PlayAmbientSound(AmbientAudioComponent, sound);
}

//
// Cosmetics
//
void ACitizen::SetTorch(int32 Hour)
{
	if (Hour >= 18 || Hour < 6) {
		TorchMesh->SetHiddenInGame(false);
		TorchNiagaraComponent->Activate();
	}
	else {
		TorchMesh->SetHiddenInGame(true);
		TorchNiagaraComponent->Deactivate();
	}
}

//
// Education
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

	if (money < Camera->CitizenManager->GetLawValue(EBillType::EducationCost))
		return false;

	if (IsValid(Building.School))
		PayForEducationLevels();

	return true;
}

void ACitizen::PayForEducationLevels()
{
	if (BioStruct.EducationLevel < BioStruct.PaidForEducationLevel)
		return;

	TMap<ACitizen*, int32> wallet;
	int32 cost = Camera->CitizenManager->GetLawValue(EBillType::EducationCost);

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
			int32 index = FMath::RandRange(0, wallet.Num() - 1);

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

//
// Work
//
bool ACitizen::CanWork(ABuilding* WorkBuilding)
{
	if (BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge))
		return false;

	if (WorkBuilding->IsA<ABroadcast>() && (Cast<ABroadcast>(WorkBuilding)->Belief != Spirituality.Faith || (Cast<ABroadcast>(WorkBuilding)->Allegiance != EParty::Undecided && Cast<ABroadcast>(WorkBuilding)->Allegiance != Camera->CitizenManager->GetCitizenParty(this))))
		return false;

	return true;
}

bool ACitizen::WillWork()
{
	int32 pensionAge = Camera->CitizenManager->GetLawValue(EBillType::PensionAge);

	if (BioStruct.Age < pensionAge)
		return true;

	int32 pension = Camera->CitizenManager->GetLawValue(EBillType::Pension);

	if (IsValid(AllocatedBuildings[2]) && pension >= Cast<AHouse>(AllocatedBuildings[2])->Rent)
		return false;

	return true;
}

float ACitizen::GetProductivity()
{
	return ProductivityMultiplier * (1 + BioStruct.EducationLevel * 0.1);
}

void ACitizen::Heal(ACitizen* Citizen)
{		
	Camera->CitizenManager->Cure(Citizen);

	if (AIController->MoveRequest.GetGoalActor() == Citizen)
		Camera->CitizenManager->PickCitizenToHeal(this);
}

int32 ACitizen::GetLeftoverMoney()
{
	int32 money = Balance;

	if (IsValid(Building.House))
		money -= Building.House->Rent;

	for (ACitizen* child : BioStruct.Children) {
		int32 maxF = FMath::CeilToInt((100 - child->Hunger) / (25.0f * child->FoodMultiplier));
		int32 cost = Camera->CitizenManager->GetLawValue(EBillType::FoodCost);

		int32 modifier = 1;

		if (BioStruct.Partner->IsValidLowLevelFast() && IsValid(BioStruct.Partner->Building.Employment))
			modifier = 2;

		money -= (cost * maxF) / modifier;
	}

	int32 maxF = FMath::CeilToInt((100 - Hunger) / (25.0f * FoodMultiplier));
	int32 cost = Camera->CitizenManager->GetLawValue(EBillType::FoodCost);

	money -= cost * maxF;

	return money;
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

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;

	for (int32 i = 0; i < Food.Num(); i++) {
		int32 curAmount = Camera->ResourceManager->GetResourceAmount(Food[i]);

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	int32 cost = Camera->CitizenManager->GetLawValue(EBillType::FoodCost);

	int32 maxF = FMath::CeilToInt((100 - Hunger) / (25.0f * FoodMultiplier));
	int32 quantity = FMath::Clamp(totalAmount, 0, maxF);

	TMap<ACitizen*, int32> wallet;

	if (cost > 0 && !IsValid(Building.Orphanage)) {
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

	if (quantity == 0) {
		PopupComponent->SetHiddenInGame(false);

		SetActorTickEnabled(true);

		SetPopupImageState("Add", "Hunger");

		return;
	}

	for (int32 i = 0; i < quantity; i++) {
		int32 selected = FMath::RandRange(0, totalAmount - 1);

		for (int32 j = 0; j < foodAmounts.Num(); j++) {
			if (foodAmounts[j] <= selected) {
				selected -= foodAmounts[j];
			}
			else {
				Camera->ResourceManager->TakeUniversalResource(Food[j], 1, 0);

				foodAmounts[j] -= 1;
				totalAmount -= 1;

				if (cost > 0 && !IsValid(Building.Orphanage)) {
					for (int32 k = 0; k < cost; k++) {
						if (Balance <= 0 && !wallet.IsEmpty()) {
							int32 index = FMath::RandRange(0, wallet.Num() - 1);

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

		Camera->ResourceManager->AddUniversalResource(Camera->ResourceManager->Money, cost);

		Hunger = FMath::Clamp(Hunger + 25, 0, 100);
	}

	if (!PopupComponent->bHiddenInGame) {
		if (HealthIssues.IsEmpty()) {
			PopupComponent->SetHiddenInGame(true);

			SetActorTickEnabled(false);
		}

		SetPopupImageState("Remove", "Hunger");
	}
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

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty() || (!Building.Employment->bCanRest && Building.Employment->bOpen) || bWorshipping || (Building.Employment->IsA<AClinic>() && Camera->CitizenManager->Healing.Contains(AIController->MoveRequest.GetGoalActor())))
		return;

	if (IsValid(Building.House) || IsValid(Building.Orphanage)) {
		if (IsValid(Building.House))
			AIController->AIMoveTo(Building.House);
		else
			AIController->AIMoveTo(Building.Orphanage);

		if (!IsValid(Building.Employment) || !Building.Employment->IsA<AExternalProduction>())
			return;

		for (FValidResourceStruct validResource : Cast<AExternalProduction>(Building.Employment)->Resources) {
			for (FWorkerStruct workerStruct : validResource.Resource->WorkerStruct) {
				if (!workerStruct.Citizens.Contains(this))
					continue;

				workerStruct.Citizens.Remove(this);

				break;
			}
		}
	}
	else {
		TArray<TWeakObjectPtr<ACitizen>> parents = { BioStruct.Mother, BioStruct.Father };

		for (TWeakObjectPtr<ACitizen> parent : parents) {
			if (!parent->IsValidLowLevelFast() || !IsValid(parent->Building.House))
				continue;

			AIController->AIMoveTo(parent->Building.House);

			break;
		}
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
	float time = FMath::RandRange(6.0f, 10.0f);
	time /= (FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed) * GetProductivity());

	AttackComponent->MeleeAnim->RateScale = 10.0f / time;
	Mesh->PlayAnimation(AttackComponent->MeleeAnim, true);

	FTimerStruct timer;
	timer.CreateTimer("Harvest", this, time, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, Resource), false, true);

	Camera->CitizenManager->Timers.Add(timer);

	GetWorldTimerManager().SetTimer(AmbientAudioHandle, FTimerDelegate::CreateUObject(this, &ACitizen::SetHarvestVisuals, Resource), time / 10.0f, true, time / 10.0f / 2.0f);

	AIController->StopMovement();
}

void ACitizen::HarvestResource(AResource* Resource)
{
	Mesh->Play(false);
	
	AResource* resource = Resource->GetHarvestedResource();

	GetWorldTimerManager().ClearTimer(AmbientAudioHandle);
	HarvestNiagaraComponent->Deactivate();

	Camera->CitizenManager->Injure(this, 99);

	LoseEnergy();

	int32 instance = AIController->MoveRequest.GetGoalInstance();
	int32 amount = Resource->GetYield(this, instance);

	if (!Camera->ResourceManager->GetResources(Building.Employment).Contains(resource->GetClass())) {
		ABuilding* broch = Camera->ResourceManager->GameMode->Broch;

		if (!AIController->CanMoveTo(broch->GetActorLocation()))
			AIController->DefaultAction();
		else
			Carry(resource, amount, broch);
	}
	else
		Carry(resource, amount, Building.Employment);
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
	BioStruct.Age++;

	if (BioStruct.Age == 5)
		GivePersonalityTrait(BioStruct.Mother.Get());
	else if (BioStruct.Age == 10)
		GivePersonalityTrait(BioStruct.Father.Get());

	RemoveFromHouse();

	if (BioStruct.Age > 50) {
		float ratio = FMath::Clamp(FMath::Pow(FMath::LogX(50.0f, BioStruct.Age - 50.0f), 3.0f), 0.0f, 1.0f);
		float odds = ratio * 90.0f;

		HealthComponent->MaxHealth = MaxHealthBeforeOld / (ratio * 5.0f);
		HealthComponent->AddHealth(0);

		MovementComponent->InitialSpeed = SpeedBeforeOld / (ratio * 2.0f);

		int32 chance = FMath::RandRange(1, 100);

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

		float scale = (BioStruct.Age * 0.04f) + 0.28f;
		AsyncTask(ENamedThreads::GameThread, [this, scale]() { Mesh->SetRelativeScale3D(FVector(scale, scale, scale)); });
	}
	else if (BioStruct.Partner != nullptr && BioStruct.Sex == ESex::Female)
		AsyncTask(ENamedThreads::GameThread, [this]() { HaveChild(); });

	if (BioStruct.Age >= 18 && BioStruct.Partner == nullptr && !Camera->Start)
		FindPartner();

	if (BioStruct.Age == 18) {
		AttackComponent->bCanAttack = true;

		SetReligion();
	}

	if (BioStruct.Age == Camera->CitizenManager->GetLawValue(EBillType::WorkAge) && IsValid(Building.Orphanage)) {
		int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

		FTimerStruct* foundTimer = Camera->CitizenManager->FindTimer("Orphanage", this);

		if (foundTimer != nullptr) {
			Camera->CitizenManager->ResetTimer("Orphanage", this);
		}
		else {
			FTimerStruct timer;
			timer.CreateTimer("Orphanage", this, timeToCompleteDay * 2.0f, FTimerDelegate::CreateUObject(Building.Orphanage, &AOrphanage::Kickout, this), false);

			Camera->CitizenManager->Timers.Add(timer);
		}
	}

	if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::VoteAge))
		SetPoliticalLeanings();

	if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::WorkAge)) {
		if (IsValid(Building.House)) {
			ACitizen* occupant = Building.House->GetOccupant(this);

			if (occupant != this && !GetLikedFamily(false).Contains(occupant))
				Building.House->RemoveVisitor(occupant, this);
		}

		if (IsValid(Building.School))
			Building.School->RemoveVisitor(Building.School->GetOccupant(this), this);
	}
}

void ACitizen::SetSex()
{
	int32 choice = FMath::RandRange(1, 100);

	float male = 0.0f;
	float total = 0.0f;

	for (AActor* actor : Camera->CitizenManager->Citizens) {
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

	int32 index = FMath::RandRange(0, parsed.Num() - 1);

	BioStruct.Name = parsed[index];
}

void ACitizen::FindPartner()
{
	ACitizen* citizen = nullptr;
	int32 curCount = 1;

	for (AActor* actor : Camera->CitizenManager->Citizens) {
		ACitizen* c = Cast<ACitizen>(actor);

		if (!IsValid(c) || c->BioStruct.Sex == BioStruct.Sex || c->BioStruct.Partner != nullptr || c->BioStruct.Age < 18)
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

		double magnitude = AIController->GetClosestActor(50.0f, GetActorLocation(), citizen->GetActorLocation(), c->GetActorLocation(), true, curCount, count);

		if (magnitude <= 0.0f)
			continue;

		citizen = c;
	}

	if (citizen != nullptr) {
		SetPartner(citizen);

		citizen->SetPartner(this);

		if (!IsValid(Building.House) && !IsValid(citizen->Building.House))
			return;

		bool bThisHouse = true;

		if (IsValid(Building.House) && IsValid(citizen->Building.House)) {
			int32 h1 = Building.House->GetQuality() / 10 + Building.House->Space;
			int32 h2 = citizen->Building.House->GetQuality() / 10 + citizen->Building.House->Space;

			if (h2 > h1)
				bThisHouse = false;
		}
		else if (IsValid(citizen->Building.House)) {
			bThisHouse = false;
		}

		if (bThisHouse) {
			if (IsValid(citizen->Building.House))
				citizen->Building.House->RemoveCitizen(citizen);
			else if (IsValid(citizen->Building.Orphanage))
				citizen->Building.Orphanage->RemoveVisitor(citizen->Building.Orphanage->GetOccupant(citizen), citizen);

			Building.House->AddVisitor(this, citizen);
		}
		else {
			if (IsValid(Building.House))
				Building.House->RemoveCitizen(this);
			else if (IsValid(Building.Orphanage))
				Building.Orphanage->RemoveVisitor(Building.Orphanage->GetOccupant(this), this);

			citizen->Building.House->AddVisitor(citizen, this);
		}

		SetAcquiredTime(2, GetWorld()->GetTimeSeconds());
	}
}

void ACitizen::SetPartner(ACitizen* Citizen)
{
	BioStruct.Partner = Citizen;
}

void ACitizen::HaveChild()
{
	if (!IsValid(Building.House) || BioStruct.Children.Num() >= Camera->CitizenManager->GetLawValue(EBillType::ChildPolicy))
		return;

	ACitizen* occupant = Building.House->GetOccupant(this);

	if (Building.House->GetVisitors(occupant).Num() == Building.House->Space)
		return;
	
	float chance = FMath::FRandRange(0.0f, 100.0f) * BioStruct.Partner->Fertility * Fertility;
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance < passMark)
		return;

	FVector location = GetActorLocation() + GetActorForwardVector() * 10.0f;

	if (Building.BuildingAt != nullptr)
		location = Building.EnterLocation;

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), location, GetActorRotation());

	if (!IsValid(citizen))
		return;

	citizen->BioStruct.Mother = this;
	citizen->BioStruct.Father = BioStruct.Partner;

	for (ACitizen* child : BioStruct.Children) {
		citizen->BioStruct.Siblings.Add(child);
		child->BioStruct.Siblings.Add(citizen);
	}

	citizen->Building.House->AddVisitor(occupant, citizen);

	BioStruct.Children.Add(citizen);
	BioStruct.Partner->BioStruct.Children.Add(citizen);
}

void ACitizen::RemoveFromHouse()
{
	TArray<ACitizen*> likedFamily = GetLikedFamily(false);

	if (likedFamily.Contains(BioStruct.Mother) || likedFamily.Contains(BioStruct.Father))
		return;

	for (ACitizen* sibling : likedFamily) {
		if (!IsValid(sibling->Building.House) || sibling->Building.House->GetVisitors(sibling->Building.House->GetOccupant(sibling)).Num() == sibling->Building.House->Space)
			continue;

		sibling->Building.House->AddVisitor(sibling->Building.House->GetOccupant(sibling), this);

		return;
	}

	if (BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge)) {
		for (ABuilding* building : Camera->CitizenManager->Buildings) {
			if (!building->IsA<AOrphanage>())
				continue;

			for (ACitizen* citizen : building->GetOccupied()) {
				if (building->GetVisitors(citizen).Num() == building->Space)
					continue;

				Building.House->RemoveVisitor(Building.House->GetOccupant(this), this);

				if (BioStruct.Mother->IsValidLowLevelFast()) {
					BioStruct.Mother = nullptr;
					BioStruct.Mother->BioStruct.Children.Remove(this);
				}

				if (BioStruct.Father->IsValidLowLevelFast()) {
					BioStruct.Father = nullptr;
					BioStruct.Father->BioStruct.Children.Remove(this);
				}

				for (ACitizen* siblings : BioStruct.Siblings)
					siblings->BioStruct.Siblings.Remove(this);

				BioStruct.Siblings.Empty();

				Cast<AOrphanage>(building)->AddVisitor(citizen, this);

				return;
			}
		}
	}
	else {
		Building.House->RemoveVisitor(Building.House->GetOccupant(this), this);
	}
}

TArray<ACitizen*> ACitizen::GetLikedFamily(bool bFactorAge)
{
	TArray<ACitizen*> family;

	if (BioStruct.Mother->IsValidLowLevelFast())
		family.Add(Cast<ACitizen>(BioStruct.Mother));

	if (BioStruct.Father->IsValidLowLevelFast())
		family.Add(Cast<ACitizen>(BioStruct.Father));

	if (BioStruct.Partner->IsValidLowLevelFast())
		family.Add(Cast<ACitizen>(BioStruct.Partner));

	for (ACitizen* child : BioStruct.Children)
		if (IsValid(child))
			family.Add(child);

	for (ACitizen* sibling : BioStruct.Siblings)
		if (IsValid(sibling))
			family.Add(sibling);

	if (bFactorAge && BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge))
		return family;

	for (int32 i = (family.Num() - 1); i > -1; i--) {
		ACitizen* citizen = family[i];
		int32 count = 0;

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

		if (count < 0)
			family.RemoveAt(i);
	}

	return family;
}

//
// Politics
//
void ACitizen::SetPoliticalLeanings()
{
	TArray<EParty> partyList;

	TEnumAsByte<ESway>* sway = nullptr;

	FPartyStruct* party = Camera->CitizenManager->GetMembersParty(this);

	if (party != nullptr)
		sway = party->Members.Find(this);

	if (sway != nullptr && sway->GetValue() == ESway::Radical)
		return;

	if (Camera->CitizenManager->Representatives.Contains(this))
		for (int32 i = 0; i < sway->GetIntValue(); i++)
			partyList.Add(party->Party);

	if (IsValid(Building.House)) {
		for (ABroadcast* broadcaster : Building.House->Influencers) {
			if (broadcaster->GetOccupied().IsEmpty() || broadcaster->Allegiance == EParty::Undecided)
				continue;

			partyList.Add(broadcaster->Allegiance);
		}
	}

	int32 itterate = FMath::Floor(GetHappiness() / 10) - 5;

	if (itterate < 0)
		for (int32 i = 0; i < FMath::Abs(itterate); i++)
			partyList.Add(EParty::ShellBreakers);

	for (FPartyStruct p : Camera->CitizenManager->Parties) {
		int32 count = 0;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
			for (EPersonality trait : p.Agreeable) {
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

	int32 index = FMath::RandRange(0, partyList.Num() - 1);

	if (party != nullptr && party->Party == partyList[index]) {
		int32 mark = FMath::RandRange(0, 100);
		int32 pass = 50;

		pass = 80;

		if (sway->GetValue() == ESway::Strong)
			pass = 95;

		if (mark > pass) {
			if (sway->GetValue() == ESway::Strong)
				party->Members.Emplace(this, ESway::Radical);
			else
				party->Members.Emplace(this, ESway::Strong);
		}
	}
	else {
		if (sway != nullptr && sway->GetValue() == ESway::Strong) {
			party->Members.Emplace(this, ESway::Moderate);

			return;
		}

		if (party != nullptr)
			party->Members.Remove(this);

		FPartyStruct partyStruct;
		partyStruct.Party = partyList[index];

		int32 i = Camera->CitizenManager->Parties.Find(partyStruct);

		Camera->CitizenManager->Parties[i].Members.Add(this, ESway::Moderate);

		if (Camera->CitizenManager->Parties[i].Party == EParty::ShellBreakers && Camera->CitizenManager->IsRebellion())
			Camera->CitizenManager->SetupRebel(this);
	}
}

//
// Religion
//
void ACitizen::SetReligion()
{
	TArray<EReligion> religionList;

	if (BioStruct.Father->IsValidLowLevelFast()) {
		Spirituality.FathersFaith = BioStruct.Father->Spirituality.Faith;

		religionList.Add(Spirituality.FathersFaith); 

		for (ABroadcast* broadcaster : BioStruct.Father->Building.House->Influencers) {
			if (broadcaster->GetOccupied().IsEmpty() || broadcaster->Allegiance != EParty::Undecided)
				continue;

			religionList.Add(broadcaster->Belief);
		}
	}

	if (BioStruct.Mother->IsValidLowLevelFast()) {
		Spirituality.MothersFaith = BioStruct.Mother->Spirituality.Faith;

		religionList.Add(Spirituality.MothersFaith);

		for (ABroadcast* broadcaster : BioStruct.Mother->Building.House->Influencers) {
			if (broadcaster->GetOccupied().IsEmpty() || broadcaster->Allegiance != EParty::Undecided)
				continue;

			religionList.Add(broadcaster->Belief);
		}
	}

	if (BioStruct.Partner->IsValidLowLevelFast())
		religionList.Add(BioStruct.Partner->Spirituality.Faith);

	religionList.Add(Spirituality.Faith);

	if (Building.Employment->IsValidLowLevelFast() && Building.Employment->IsA<ABroadcast>() && !Cast<ABroadcast>(Building.Employment)->GetOccupied().IsEmpty() && Cast<ABroadcast>(Building.Employment)->Allegiance == EParty::Undecided)
		religionList.Add(Cast<ABroadcast>(Building.Employment)->Belief);

	for (FReligionStruct religion : Camera->CitizenManager->Religions) {
		int32 count = 0;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
			for (EPersonality trait : religion.Agreeable) {
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

	int32 index = FMath::RandRange(0, religionList.Num() - 1);

	Spirituality.Faith = religionList[index];
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

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;
	timer.CreateTimer("Mass", this, timeToCompleteDay * 2, FTimerDelegate::CreateUObject(this, &ACitizen::SetAttendStatus, EAttendStatus::Neutral, bMass), false);

	Camera->CitizenManager->Timers.Add(timer);
}

void ACitizen::SetHolliday(bool bStatus)
{
	bHolliday = bStatus;
}

int32 ACitizen::GetHappiness()
{
	int32 value = 50;

	for (const TPair<FString, int32>& pair : Happiness.Modifiers)
		value += pair.Value;

	return value;
}

void ACitizen::SetHappiness()
{
	Happiness.ClearValues();

	if (!IsValid(Building.House))
		Happiness.SetValue("Homeless", -20);
	else {
		Happiness.SetValue("Housed", 10);

		int32 quality = (Building.House->GetQuality() / 5 - 10) * 2;

		if (quality > 0)
			quality *= 0.75f;

		FString message = "";

		if (Building.House->GetQuality() < 25)
			message = "Awful Housing Quality";
		else if (Building.House->GetQuality() < 50)
			message = "Substandard Housing";
		else if (Building.House->GetQuality() > 50)
			message = "Quality Housing";
		else if (Building.House->GetQuality() > 75)
			message = "High-Quality Housing";

		Happiness.SetValue(message, quality);

		if (Spirituality.Faith != EReligion::Atheist) {
			bool bNearbyFaith = false;

			for (ABroadcast* broadcaster : Building.House->Influencers) {
				if (broadcaster->GetOccupied().IsEmpty() || !broadcaster->bHolyPlace && broadcaster->Belief != Spirituality.Faith)
					continue;

				bNearbyFaith = true;
			}

			if (!bNearbyFaith)
				Happiness.SetValue("No Working Holy Place Nearby", -25);
			else
				Happiness.SetValue("Holy Place Nearby", 15);
		}

		bool bIsEvil = false;

		for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this))
			if (personality->Trait == EPersonality::Cruel)
				bIsEvil = true;

		bool bIsPark = false;

		for (ABroadcast* broadcaster : Building.House->Influencers) {
			if (!broadcaster->bIsPark)
				continue;

			bIsPark = true;

			break;
		}

		if (bIsPark) {
			if (bIsEvil)
				Happiness.SetValue("Park Nearby", -5);
			else
				Happiness.SetValue("Park Nearby", 5);
		}
		else
			Happiness.SetValue("No Nearby Park", -10);

		bool bPropaganda = true;

		for (ABroadcast* broadcaster : Building.House->Influencers) {
			if (broadcaster->bIsPark || broadcaster->bHolyPlace)
				continue;

			if (broadcaster->Allegiance == Camera->CitizenManager->GetCitizenParty(this) || broadcaster->Belief == Spirituality.Faith)
				bPropaganda = false;
		}

		if (Building.House->Influencers.IsEmpty())
			bPropaganda = false;

		if (bPropaganda)
			Happiness.SetValue("Nearby propaganda tower", -25);
		else
			Happiness.SetValue("Nearby eggtastic tower", 15);
	}

	if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::WorkAge)) {
		if (Building.Employment == nullptr)
			Happiness.SetValue("Unemployed", -10);
		else
			Happiness.SetValue("Employed", 5);

		if (HoursWorked.Num() < IdealHoursWorkedMin || HoursWorked.Num() > IdealHoursWorkedMax)
			Happiness.SetValue("Inadequate Hours Worked", -15);
		else
			Happiness.SetValue("Ideal Hours Worked", 10);

		if (Balance < 5)
			Happiness.SetValue("Poor", -25);
		else if (Balance >= 5 && Balance < 15)
			Happiness.SetValue("Well Off", 10);
		else
			Happiness.SetValue("Rich", 20);

		if (IsValid(Building.Employment) && GetWorld()->GetTimeSeconds() >= GetAcquiredTime(1) + 300.0f) {
			int32 count = 0;

			for (ACitizen* citizen : Building.Employment->GetOccupied()) {
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

	if (Camera->CitizenManager->GetCitizenParty(this) != EParty::Undecided) {
		int32 lawTally = 0;

		for (FLawStruct law : Camera->CitizenManager->Laws) {
			if (law.BillType == EBillType::Abolish)
				continue;

			int32 count = 0;

			FLeanStruct partyLean;
			partyLean.Party = Camera->CitizenManager->GetMembersParty(this)->Party;

			int32 index = law.Lean.Find(partyLean);

			if (index == INDEX_NONE)
				continue;

			if (!law.Lean[index].ForRange.IsEmpty() && Camera->CitizenManager->IsInRange(law.Lean[index].ForRange, law.Value))
				count++;
			else if (!law.Lean[index].AgainstRange.IsEmpty() && Camera->CitizenManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
				count--;

			for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(this)) {
				FLeanStruct personalityLean;
				personalityLean.Personality = personality->Trait;

				index = law.Lean.Find(personalityLean);

				if (index == INDEX_NONE)
					continue;

				if (!law.Lean[index].ForRange.IsEmpty() && Camera->CitizenManager->IsInRange(law.Lean[index].ForRange, law.Value))
					count++;
				else if (!law.Lean[index].AgainstRange.IsEmpty() && Camera->CitizenManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
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

	if (GetHappiness() < 35)
		SadTimer++;
	else
		SadTimer = 0;

	if (SadTimer == 300) {
		FEventStruct event;
		event.Type = EEventType::Protest;

		int32 index = Camera->CitizenManager->Events.Find(event);
		
		if (Camera->CitizenManager->Events[index].Times.IsEmpty())
			Camera->CitizenManager->CreateEvent(EEventType::Protest, nullptr, "", 0, FMath::RandRange(6, 9), FMath::RandRange(12, 18), false);
	}
}

//
// Genetics
//
void ACitizen::GenerateGenetics()
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

		int32 choice = FMath::RandRange(0, grades.Num() - 1);

		genetic.Grade = grades[choice];

		int32 mutate = FMath::RandRange(1, 100);

		int32 chance = 100 - (Camera->CitizenManager->PrayStruct.Bad * 5) - (Camera->CitizenManager->PrayStruct.Good * 5);

		if (mutate >= chance)
			continue;

		grades.Empty();

		grades.Add(EGeneticsGrade::Neutral);

		for (int32 i = 0; i <= Camera->CitizenManager->PrayStruct.Good; i++)
			grades.Add(EGeneticsGrade::Good);

		for (int32 i = 0; i <= Camera->CitizenManager->PrayStruct.Bad; i++)
			grades.Add(EGeneticsGrade::Bad);

		grades.Remove(genetic.Grade);

		choice = FMath::RandRange(0, grades.Num() - 1);

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
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ApplyToMultiplier("Awareness", 0.75f);
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

	int32 chance = FMath::RandRange(0, 100);

	if (chance >= 45 * parentsPersonalities.Num())
		personalities = Camera->CitizenManager->Personalities;
	else
		for (FPersonality* personality : parentsPersonalities)
			personalities.Add(*personality);

	for (int32 i = personalities.Num() - 1; i > -1; i--) {
		if (!personalities[i].Citizens.Contains(this))
			continue;

		for (int32 j = personalities.Num() - 1; j > -1; j--) {
			if (!personalities[i].Dislikes.Contains(personalities[j].Trait))
				continue;

			if (j < i)
				i--;

			personalities.RemoveAt(j);
		}

		personalities.RemoveAt(i);
	}

	auto index = Async(EAsyncExecution::TaskGraph, [personalities]() { return FMath::RandRange(0, personalities.Num() - 1); });

	int32 i = Camera->CitizenManager->Personalities.Find(personalities[index.Get()]);

	Camera->CitizenManager->Personalities[i].Citizens.Add(this);

	ApplyTraitAffect(Camera->CitizenManager->Personalities[i].Trait);
}

void ACitizen::ApplyTraitAffect(EPersonality Trait)
{
	if (Trait == EPersonality::Outgoing)
		ApplyToMultiplier("Fertility", 1.15f);
	else if (Trait == EPersonality::Reserved)
		ApplyToMultiplier("Fertility", 0.85f);

	if (Trait == EPersonality::Talented || Trait == EPersonality::Diligent)
		ApplyToMultiplier("Productivity", 1.15f);
	else if (Trait == EPersonality::Inept || Trait == EPersonality::Lazy)
		ApplyToMultiplier("Productivity", 0.85f);

	if (Trait == EPersonality::Energetic)
		ApplyToMultiplier("Speed", 1.15f);
	else if (Trait == EPersonality::Lethargic)
		ApplyToMultiplier("Speed", 0.85f);

	if (Trait == EPersonality::Brave || Trait == EPersonality::Inept)
		ApplyToMultiplier("Damage", 1.5f);
	else if (Trait == EPersonality::Craven)
		ApplyToMultiplier("Damage", 0.5f);

	if (Trait == EPersonality::Outgoing || Trait == EPersonality::Lethargic || Trait == EPersonality::Workaholic)
		IdealHoursSlept = 6;
	else if (Trait == EPersonality::Lazy || Trait == EPersonality::Energetic || Trait == EPersonality::Idler)
		IdealHoursSlept = 10;

	if (Trait == EPersonality::Workaholic) {
		IdealHoursWorkedMin = 8;
		IdealHoursWorkedMax = 16;
	}
	else if (Trait == EPersonality::Idler) {
		IdealHoursWorkedMin = 0;
		IdealHoursWorkedMax = 8;
	}
}

void ACitizen::ApplyToMultiplier(FString Affect, float Amount)
{
	Amount = Amount - 1.0f;
	
	if (Affect == "Damage") {
		AttackComponent->DamageMultiplier += Amount;
	}
	else if (Affect == "Speed") {
		MovementComponent->SpeedMultiplier += Amount;
	}
	else if (Affect == "Health") {
		HealthComponent->HealthMultiplier += Amount;

		float percHealth = HealthComponent->Health / HealthComponent->MaxHealth;
		HealthComponent->MaxHealth = FMath::Clamp(10 + (5 * BioStruct.Age), 0, 100) * HealthComponent->HealthMultiplier;
		HealthComponent->Health = HealthComponent->MaxHealth * percHealth;
	}
	else if (Affect == "Fertility") {
		Fertility += Amount;
	}
	else if (Affect == "Productivity") {
		ProductivityMultiplier += Amount;
	}
	else if (Affect == "Reach") {
		ReachMultiplier += Amount;

		Capsule->SetRelativeScale3D(FVector(1.0f) * ReachMultiplier);

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

		int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);
		Camera->CitizenManager->UpdateTimerLength("Eat", this, (timeToCompleteDay / 200) * HungerMultiplier);
	}
	else if (Affect == "Energy") {
		EnergyMultiplier += Amount;

		int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);
		Camera->CitizenManager->UpdateTimerLength("Energy", this, (timeToCompleteDay / 100) * EnergyMultiplier);
	}
}