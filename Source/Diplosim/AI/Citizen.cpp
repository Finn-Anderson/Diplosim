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
#include "Player/Components/BuildComponent.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/House.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Work/Service/School.h"
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
	Mesh->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

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

	AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);
	AttackComponent->RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);

	Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);
	Reach->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	TimeOfEmployment = -1000.0f;
	TimeOfResidence = -1000.0f;

	bGain = false;

	bHasBeenLeader = false;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ProductivityMultiplier = 1.0f;
	Fertility = 1.0f;

	bSleep = false;
	IdealHoursSlept = 8;
	HoursSleptToday = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };

	IdealHoursWorkedMin = 0;
	IdealHoursWorkedMax = 12;

	SpeedBeforeOld = 100.0f;
	MaxHealthBeforeOld = 100.0f;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	Camera->CitizenManager->Citizens.Add(this);
	Camera->CitizenManager->Infectible.Add(this);

	int32 timeToCompleteDay = 360 / (24 * Camera->Grid->AtmosphereComponent->Speed);

	FTimerStruct timer;

	timer.CreateTimer("Eat", this, timeToCompleteDay / 200, FTimerDelegate::CreateUObject(this, &ACitizen::Eat), true);
	Camera->CitizenManager->Timers.Add(timer);

	timer.CreateTimer("Energy", this, timeToCompleteDay / 100, FTimerDelegate::CreateUObject(this, &ACitizen::CheckGainOrLoseEnergy), true);
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

	AIController->Idle();

	SetTorch(Camera->Grid->AtmosphereComponent->Calendar.Hour);

	GenerateGenetics();
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// Disease
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(OtherActor);

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AClinic>()) {
			for (FConditionStruct condition : HealthIssues) {
				if (citizen->HealthIssues.Contains(condition))
					continue;

				int32 chance = FMath::RandRange(1, 100);

				if (chance <= condition.Spreadability)
					citizen->HealthIssues.Add(condition);
			}

			if (!citizen->HealthIssues.IsEmpty() && !citizen->DiseaseNiagaraComponent->IsActive())
				Camera->CitizenManager->Infect(citizen);
		}

		if (Building.Employment != nullptr && Building.Employment->IsA<AClinic>() && !GetWorldTimerManager().IsTimerActive(HealTimer)) {
			CitizenHealing = citizen;

			GetWorldTimerManager().SetTimer(HealTimer, FTimerDelegate::CreateUObject(this, &ACitizen::Heal, citizen), 2.0f / GetProductivity(), false);
		}
	}

	// Movement
	if (OtherActor->IsA<AResource>() || OtherActor->IsA<ABuilding>()) {
		FCollidingStruct collidingStruct;
		collidingStruct.Actor = OtherActor;

		if (OtherActor->IsA<AResource>())
			collidingStruct.Instance = OtherBodyIndex;

		StillColliding.Add(collidingStruct);
	}

	if (OtherActor != AIController->MoveRequest.GetGoalActor() || (OtherBodyIndex != AIController->MoveRequest.GetGoalInstance() && AIController->MoveRequest.GetGoalInstance() > -1))
		return;

	if (OtherActor->IsA<AResource>()) {
		AResource* r = Cast<AResource>(OtherActor);

		StartHarvestTimer(r, OtherBodyIndex);
	}
	else if (OtherActor->IsA<ABuilding>()) {
		ABuilding* b = Cast<ABuilding>(OtherActor);

		b->Enter(this);
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	FCollidingStruct collidingStruct;
	collidingStruct.Actor = OtherActor;

	if (OtherActor->IsA<AResource>())
		collidingStruct.Instance = OtherBodyIndex;
	else if (OtherActor->IsA<ACitizen>() && CitizenHealing == OtherActor && GetWorldTimerManager().IsTimerActive(HealTimer))
		GetWorldTimerManager().ClearTimer(HealTimer);

	if (StillColliding.Contains(collidingStruct))
		StillColliding.Remove(collidingStruct);
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
	
	int32 money = Balance;

	if (BioStruct.Mother->IsValidLowLevelFast())
		money += BioStruct.Mother->Balance;

	if (BioStruct.Father->IsValidLowLevelFast())
		money += BioStruct.Father->Balance;

	if (money < Camera->CitizenManager->GetLawValue(EBillType::EducationCost))
		return false;

	if (IsValid(Building.School))
		PayForEducationsLevels();

	return true;
}

void ACitizen::PayForEducationsLevels()
{
	int32 money = Balance;

	if (BioStruct.Mother->IsValidLowLevelFast())
		money += BioStruct.Mother->Balance;

	if (BioStruct.Father->IsValidLowLevelFast())
		money += BioStruct.Father->Balance;

	int32 levels = FMath::Clamp(money / Camera->CitizenManager->GetLawValue(EBillType::EducationCost), 0, 5);

	int32 cost = Camera->CitizenManager->GetLawValue(EBillType::EducationCost) * levels;

	for (int32 i = 0; i < cost; i++) {
		if (BioStruct.Mother->IsValidLowLevelFast() && BioStruct.Father->IsValidLowLevelFast()) {
			if (BioStruct.Mother->Balance > BioStruct.Father->Balance && BioStruct.Mother->Balance > Balance)
				BioStruct.Mother->Balance--;
			else if (BioStruct.Father->Balance > BioStruct.Mother->Balance && BioStruct.Father->Balance > Balance)
				BioStruct.Father->Balance--;
			else
				Balance--;
		}
		else if (BioStruct.Mother->IsValidLowLevelFast()) {
			if (BioStruct.Mother->Balance > Balance)
				BioStruct.Mother->Balance--;
			else
				Balance--;
		}
		else {
			if (BioStruct.Father->Balance > Balance)
				BioStruct.Father->Balance--;
			else
				Balance--;
		}
	}
}

void ACitizen::PayForEducationLevels()
{
	if (BioStruct.EducationProgress < 100)
		return;

	float progress = BioStruct.EducationProgress / 100.0f;
	int32 levels = FMath::FloorToInt(progress);

	for (int32 i = 0; i < levels; i++) {
		if (!CanAffordEducationLevel())
			return;

		BioStruct.EducationProgress -= 100;

		BioStruct.EducationLevel++;

		if (BioStruct.EducationLevel == 5) {
			Building.School->RemoveStudent(this);

			return;
		}
	}
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

void ACitizen::FindJobAndHouse(int32 TimeToCompleteDay)
{
	if (BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge)) {
		if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::EducationAge) && BioStruct.EducationLevel < 5 && CanAffordEducationLevel()) {
			ASchool* chosenSchool = Building.School;

			for (ABuilding* building : Camera->CitizenManager->Buildings) {
				if (!building->IsA<ASchool>() || building->GetOccupied().IsEmpty() || !AIController->CanMoveTo(building->GetActorLocation()) || chosenSchool == building)
					continue;

				ASchool* school = Cast<ASchool>(building);

				if (!IsValid(chosenSchool)) {
					chosenSchool = school;

					continue;
				}

				FVector location = GetActorLocation();

				if (BioStruct.Mother->IsValidLowLevelFast() && IsValid(BioStruct.Mother->Building.House))
					location = BioStruct.Mother->Building.House->GetActorLocation();
				else if (BioStruct.Father->IsValidLowLevelFast() && IsValid(BioStruct.Father->Building.House))
					location = BioStruct.Mother->Building.House->GetActorLocation();

				double magnitude = AIController->GetClosestActor(location, chosenSchool->GetActorLocation(), building->GetActorLocation(), 2, 1);

				if (magnitude <= 0.0f)
					continue;

				chosenSchool = school;
			}

			if (chosenSchool == Building.School)
				return;

			AsyncTask(ENamedThreads::GameThread, [this, chosenSchool]() {
				if (IsValid(chosenSchool)) {
					if (Building.School != nullptr)
						Building.School->RemoveStudent(this);

					chosenSchool->AddCitizen(this);
				}
			});
			PayForEducationsLevels();
		}

		return;
	}

	if (!IsValid(Building.House))
		TimeOfResidence = -1000.0f;

	if (!IsValid(Building.Employment))
		TimeOfEmployment = -1000.0f;

	if (GetWorld()->GetTimeSeconds() < TimeOfResidence + TimeToCompleteDay && GetWorld()->GetTimeSeconds() < TimeOfEmployment + TimeToCompleteDay)
		return;

	int32 curDiff = 0;
	AWork* chosenWorkplace = Building.Employment;
	AHouse* chosenHouse = Building.House;

	for (ABuilding* building : Camera->CitizenManager->Buildings) {
		if (!IsValid(building) || building->GetCapacity() == building->GetOccupied().Num() || !CanWork(building) || !AIController->CanMoveTo(building->GetActorLocation()))
			continue;

		if (building->IsA<AHouse>()) {
			if (GetWorld()->GetTimeSeconds() < TimeOfResidence + TimeToCompleteDay || chosenHouse == building)
				continue;

			AHouse* house = Cast<AHouse>(building);

			if (Balance < house->Rent && (!IsValid(Building.Employment) || Building.Employment->Wage < house->Rent))
				continue;

			if (chosenHouse != nullptr) {
				int32 curLeftoverMoney = 0;

				if (IsValid(Building.Employment))
					curLeftoverMoney = Building.Employment->Wage;
				else
					curLeftoverMoney = Balance;

				int32 newLeftoverMoney = (curLeftoverMoney - house->Rent) * 50;

				curLeftoverMoney -= chosenHouse->Rent;
				curLeftoverMoney *= 50;

				int32 currentValue = FMath::Max(chosenHouse->GetQuality() + curLeftoverMoney, 0);
				int32 newValue = FMath::Max(house->GetQuality() + newLeftoverMoney, 0);

				FVector workLocation = GetActorLocation();

				if (IsValid(Building.Employment))
					workLocation = Building.Employment->GetActorLocation();

				double magnitude = AIController->GetClosestActor(workLocation, chosenHouse->GetActorLocation(), building->GetActorLocation(), currentValue, newValue);

				if (magnitude <= 0.0f)
					continue;
			}

			chosenHouse = house;
		}
		else if (!building->IsA<AWork>() || GetWorld()->GetTimeSeconds() < TimeOfEmployment + TimeToCompleteDay || chosenWorkplace == building)
			continue;

		int32 diff = Cast<AWork>(building)->Wage;

		if (IsValid(Building.Employment))
			diff -= Building.Employment->Wage;

		if (diff > curDiff) {
			curDiff = diff;
			chosenWorkplace = Cast<AWork>(building);
		}
	}

	int32 pensionAge = Camera->CitizenManager->GetLawValue(EBillType::PensionAge);

	if (BioStruct.Age >= pensionAge) {
		int32 pension = Camera->CitizenManager->GetLawValue(EBillType::Pension);

		if (pension >= chosenHouse->Rent) {
			Building.Employment->RemoveCitizen(this);

			chosenWorkplace = nullptr;
		}
	}

	if (IsValid(chosenWorkplace) && chosenWorkplace != Building.Employment) {
		int32* value = Happiness.Modifiers.Find("Work Happiness");

		if (value != nullptr)
			curDiff -= *value;

		int32 chance = FMath::RandRange(0, 100);

		if (chance < 50 - (curDiff * 15) && Building.Employment != nullptr)
			return;
	}

	AsyncTask(ENamedThreads::GameThread, [this, chosenWorkplace, chosenHouse]() {
		if (IsValid(chosenWorkplace) && chosenWorkplace != Building.Employment) {
			if (Building.Employment != nullptr)
				Building.Employment->RemoveCitizen(this);

			chosenWorkplace->AddCitizen(this);
		}

		if (IsValid(chosenHouse) && chosenHouse != Building.House) {
			if (Building.House != nullptr)
				Building.House->RemoveCitizen(this);

			chosenHouse->AddCitizen(this);
		}
	});
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

	TArray<AActor*> actors;
	Reach->GetOverlappingActors(actors);

	for (AActor* actor : actors) {
		if (!actor->IsA<ACitizen>() || Cast<ACitizen>(actor)->HealthIssues.IsEmpty())
			continue;

		ACitizen* citizen = Cast<ACitizen>(actor);

		CitizenHealing = citizen;

		GetWorldTimerManager().SetTimer(HealTimer, FTimerDelegate::CreateUObject(this, &ACitizen::Heal, citizen), 2.0f / GetProductivity(), false);

		break;
	}
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
		AsyncTask(ENamedThreads::GameThread, [this]() { HealthComponent->TakeHealth(10, this); });

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;

	for (int32 i = 0; i < Food.Num(); i++) {
		auto value = Async(EAsyncExecution::TaskGraphMainThread, [this, i]() { return Camera->ResourceManager->GetResourceAmount(Food[i]); });

		int32 curAmount = value.Get();

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	int32 cost = Camera->CitizenManager->GetLawValue(EBillType::FoodCost);

	int32 maxF = FMath::CeilToInt((100 - Hunger) / 25.0f);
	int32 quantity = FMath::Clamp(totalAmount, 0, maxF);

	if (cost > 0)
		quantity = FMath::Min(quantity, Balance / cost);

	if (quantity == 0) {
		PopupComponent->SetHiddenInGame(false);

		SetActorTickEnabled(true);

		AsyncTask(ENamedThreads::GameThread, [this]() { SetPopupImageState("Add", "Hunger"); });

		return;
	}

	for (int32 i = 0; i < quantity; i++) {
		int32 selected = FMath::RandRange(0, totalAmount - 1);

		for (int32 j = 0; j < foodAmounts.Num(); j++) {
			if (foodAmounts[j] <= selected) {
				selected -= foodAmounts[j];
			}
			else {
				AsyncTask(ENamedThreads::GameThread, [this, j]() { Camera->ResourceManager->TakeUniversalResource(Food[j], 1, 0); });

				foodAmounts[j] -= 1;
				totalAmount -= 1;

				break;
			}
		}

		Balance -= cost;
		AsyncTask(ENamedThreads::GameThread, [this]() { Camera->ResourceManager->AddUniversalResource(Camera->ResourceManager->Money, 1); });

		Hunger = FMath::Clamp(Hunger + 25, 0, 100);
	}

	if (!PopupComponent->bHiddenInGame) {
		if (HealthIssues.IsEmpty()) {
			PopupComponent->SetHiddenInGame(true);

			SetActorTickEnabled(false);
		}

		AsyncTask(ENamedThreads::GameThread, [this]() { SetPopupImageState("Remove", "Hunger"); });
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

	if (Building.House->IsValidLowLevelFast()) {
		AIController->AIMoveTo(Building.House);

		if (!Building.Employment->IsA<AExternalProduction>())
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
	else if (BioStruct.Age < 18) {
		TArray<TWeakObjectPtr<ACitizen>> parents = { BioStruct.Mother, BioStruct.Father };

		for (TWeakObjectPtr<ACitizen> parent : parents) {
			if (parent == nullptr || !parent->Building.House->IsValidLowLevelFast())
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

	if (Energy >= 100)
		AIController->DefaultAction();
}

//
// Resources
//
void ACitizen::StartHarvestTimer(AResource* Resource, int32 Instance)
{
	float time = FMath::RandRange(6.0f, 10.0f);
	time /= (FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed) * GetProductivity());

	FTimerStruct timer;
	timer.CreateTimer("Harvest", this, time, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, Resource, Instance), false);

	Camera->CitizenManager->Timers.Add(timer);

	USoundBase* sound = nullptr;

	if (Resource->IsA<AMineral>())
		sound = Mines[FMath::RandRange(0, Mines.Num() - 1)];
	else
		sound = Chops[FMath::RandRange(0, Chops.Num() - 1)];

	timer.CreateTimer("AmbientHarvestSound", this, 1, FTimerDelegate::CreateUObject(Camera, &ACamera::PlayAmbientSound, AmbientAudioComponent, sound), true);

	Camera->CitizenManager->Timers.Add(timer);

	AIController->StopMovement();
}

void ACitizen::HarvestResource(AResource* Resource, int32 Instance)
{
	AResource* resource = Resource->GetHarvestedResource();

	Camera->CitizenManager->RemoveTimer("AmbientHarvestSound", this);

	Camera->CitizenManager->Injure(this, 99);

	LoseEnergy();

	if (!Camera->ResourceManager->GetResources(Building.Employment).Contains(resource->GetClass())) {
		ABuilding* broch = Camera->ResourceManager->GameMode->Broch;

		if (!AIController->CanMoveTo(broch->GetActorLocation()))
			AIController->DefaultAction();
		else
			Carry(resource, Resource->GetYield(this, Instance), broch);
	}
	else
		Carry(resource, Resource->GetYield(this, Instance), Building.Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

	LoseEnergy();

	if (Location == nullptr)
		AIController->StopMovement();
	else
		AIController->AIMoveTo(Location);
}

//
// Bio
//
void ACitizen::Birthday()
{
	BioStruct.Age++;

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
		AsyncTask(ENamedThreads::GameThread, [this, scale]() { Mesh->SetWorldScale3D(FVector(scale, scale, scale)); });
	}
	else if (BioStruct.Partner != nullptr && BioStruct.Sex == ESex::Female)
		AsyncTask(ENamedThreads::GameThread, [this]() { HaveChild(); });

	if (BioStruct.Age >= 18 && BioStruct.Partner == nullptr)
		FindPartner();

	if (BioStruct.Age == 18) {
		AttackComponent->bCanAttack = true;

		SetReligion();
	}

	if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::VoteAge))
		SetPolticalLeanings();

	if (BioStruct.Age >= Camera->CitizenManager->GetLawValue(EBillType::WorkAge) && IsValid(Building.School))
		Building.School->RemoveStudent(this);

	if (BioStruct.Age == 5)
		GivePersonalityTrait(BioStruct.Mother.Get());
	else if (BioStruct.Age == 10)
		GivePersonalityTrait(BioStruct.Father.Get());
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

		if (c->BioStruct.Sex == BioStruct.Sex || c->BioStruct.Partner != nullptr || c->BioStruct.Age < 18)
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

		double magnitude = AIController->GetClosestActor(GetActorLocation(), citizen->GetActorLocation(), c->GetActorLocation(), curCount, count);

		if (magnitude <= 0.0f)
			continue;

		citizen = c;
	}

	if (citizen != nullptr) {
		SetPartner(citizen);

		citizen->SetPartner(this);
	}
}

void ACitizen::SetPartner(ACitizen* Citizen)
{
	BioStruct.Partner = Citizen;
}

void ACitizen::HaveChild()
{
	float chance = FMath::FRandRange(0.0f, 100.0f) * BioStruct.Partner->Fertility * Fertility;
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance < passMark)
		return;

	FVector location = GetActorLocation() + GetActorForwardVector() * 10.0f;

	if (Building.BuildingAt != nullptr)
		location = Building.EnterLocation;

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), location, GetActorRotation());

	if (!citizen->IsValidLowLevelFast())
		return;

	citizen->BioStruct.Mother = this;
	citizen->BioStruct.Father = BioStruct.Partner;

	BioStruct.Children.Add(citizen);
	BioStruct.Partner->BioStruct.Children.Add(citizen);
}

//
// Politics
//
void ACitizen::SetPolticalLeanings()
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

	int32 mark = FMath::RandRange(0, 100);
	int32 pass = 50;

	if (party != nullptr && party->Party == partyList[index]) {
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
	else if (mark > pass) {
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

void ACitizen::SetMassStatus(EMassStatus Status)
{
	MassStatus = Status;
}

//
// Happiness
//
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

		if (bPropaganda)
			Happiness.SetValue("Nearby propaganda tower", -25);
		else
			Happiness.SetValue("Nearby eggtastic tower", 15);
	}

	if (Building.Employment == nullptr)
		Happiness.SetValue("Unemployed", -10);
	else
		Happiness.SetValue("Employed", 5);

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

	if (HoursWorked.Num() < IdealHoursWorkedMin || HoursWorked.Num() > IdealHoursWorkedMax)
		Happiness.SetValue("Inadequate Hours Worked", -15);
	else if (HoursSleptToday.Num() >= IdealHoursSlept)
		Happiness.SetValue("Ideal Hours Worked", 10);

	if (MassStatus == EMassStatus::Missed)
		Happiness.SetValue("Missed Mass", -25);
	else if (MassStatus == EMassStatus::Attended)
		Happiness.SetValue("Attended Mass", 15);

	if (IsValid(Building.Employment) && GetWorld()->GetTimeSeconds() >= TimeOfEmployment + 300.0f) {
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

		Happiness.SetValue("Work Happiness", count * 5 + 5);
	}

	if (Balance < 5)
		Happiness.SetValue("Poor", -25);
	else if (Balance >= 5 && Balance < 15)
		Happiness.SetValue("Well Off", 10);
	else
		Happiness.SetValue("Rich", 20);

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

	if (GetHappiness() < 20)
		SadTimer++;
	else
		SadTimer = 0;

	if (SadTimer == 300)
		HealthComponent->TakeHealth(HealthComponent->GetHealth(), this);
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
		if (Genetic.Grade == EGeneticsGrade::Good) {
			MovementComponent->InitialSpeed = 250.0f;
			MovementComponent->MaxSpeed = 250.0f;
		}
		else if (Genetic.Grade == EGeneticsGrade::Bad) {
			MovementComponent->InitialSpeed = 150.0f;
			MovementComponent->MaxSpeed = 150.0f;
		}
	}
	else if (Genetic.Type == EGeneticsType::Shell) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			HealthComponent->SetHealthMultiplier(1.25f);
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			HealthComponent->SetHealthMultiplier(0.75f);

		HealthComponent->MaxHealth = HealthComponent->MaxHealth * HealthComponent->HealthMultiplier;
		HealthComponent->Health = HealthComponent->MaxHealth;
	}
	else if (Genetic.Type == EGeneticsType::Reach) {
		if (Genetic.Grade == EGeneticsGrade::Good) {
			SetActorScale3D(FVector(1.25f));

			AttackComponent->RangeComponent->SetRelativeScale3D(FVector(0.8f));
		}
		else if (Genetic.Grade == EGeneticsGrade::Bad) {
			SetActorScale3D(FVector(0.75f));

			AttackComponent->RangeComponent->SetRelativeScale3D(FVector(1.333333333333f));
		}
	}
	else if (Genetic.Type == EGeneticsType::Awareness) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			AttackComponent->RangeComponent->SetWorldScale3D(AttackComponent->RangeComponent->GetRelativeScale3D() * FVector(1.25f));
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			AttackComponent->RangeComponent->SetWorldScale3D(AttackComponent->RangeComponent->GetRelativeScale3D() * FVector(0.75f));
	}
	else if (Genetic.Type == EGeneticsType::Productivity) {
		if (Genetic.Grade == EGeneticsGrade::Good)
			ProductivityMultiplier = 1.15f;
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			ProductivityMultiplier = 0.85f;
	}
	else {
		if (Genetic.Grade == EGeneticsGrade::Good)
			Fertility = 1.15f;
		else if (Genetic.Grade == EGeneticsGrade::Bad)
			Fertility = 0.85f;
	}
}

//
// Personality
//
void ACitizen::GivePersonalityTrait(ACitizen* Parent)
{
	TArray<FPersonality*> parentsPersonalities = Camera->CitizenManager->GetCitizensPersonalities(Parent);

	int32 chance = FMath::RandRange(0, 100);

	if (chance >= 45 * parentsPersonalities.Num()) {
		int32 index = FMath::RandRange(0, Camera->CitizenManager->Personalities.Num() - 1);

		Camera->CitizenManager->Personalities[index].Citizens.Add(this);

		ApplyTraitAffect(Camera->CitizenManager->Personalities[index].Trait);
	}
	else {
		int32 index = FMath::RandRange(0, parentsPersonalities.Num() - 1);

		for (FPersonality& personality : Camera->CitizenManager->Personalities)
			if (personality.Trait == parentsPersonalities[index]->Trait)
				personality.Citizens.Add(this);

		ApplyTraitAffect(parentsPersonalities[index]->Trait);
	}
}

void ACitizen::ApplyTraitAffect(EPersonality Trait)
{
	if (Trait == EPersonality::Outgoing)
		Fertility *= 1.15f;
	else if (Trait == EPersonality::Reserved)
		Fertility *= 0.85f;

	if (Trait == EPersonality::Talented || Trait == EPersonality::Diligent)
		ProductivityMultiplier *= 1.15f;
	else if (Trait == EPersonality::Inept || Trait == EPersonality::Lazy)
		ProductivityMultiplier *= 0.85f;

	if (Trait == EPersonality::Energetic) {
		MovementComponent->InitialSpeed *= 1.15f;
		MovementComponent->MaxSpeed *= 1.15f;
	}
	else if (Trait == EPersonality::Lethargic) {
		MovementComponent->InitialSpeed *= 0.85f;
		MovementComponent->MaxSpeed *= 0.85f;
	}

	if (Trait == EPersonality::Brave || Trait == EPersonality::Inept)
		AttackComponent->DamageMultiplier *= 1.5f;
	else if (Trait == EPersonality::Craven)
		AttackComponent->DamageMultiplier *= 0.5f;

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