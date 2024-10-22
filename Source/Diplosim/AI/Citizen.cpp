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
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/House.h"
#include "Buildings/Work/Service/Clinic.h"
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

	TimeOfEmployment = -300.0f;
	TimeOfResidence = -300.0f;

	bGain = false;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;
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

	timer.CreateTimer(this, timeToCompleteDay / 200, FTimerDelegate::CreateUObject(this, &ACitizen::Eat), true);
	Camera->CitizenManager->Timers.Add(timer);

	timer.CreateTimer(this, timeToCompleteDay / 100, FTimerDelegate::CreateUObject(this, &ACitizen::CheckGainOrLoseEnergy), true);
	Camera->CitizenManager->Timers.Add(timer);

	timer.CreateTimer(this, timeToCompleteDay / 10, FTimerDelegate::CreateUObject(this, &ACitizen::Birthday), true);
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

		if (Building.Employment != nullptr && Building.Employment->IsA<AClinic>()) {
			Camera->CitizenManager->Cure(citizen);

			if (AIController->MoveRequest.GetGoalActor() == citizen)
				Camera->CitizenManager->PickCitizenToHeal(this);
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
// Work
//
bool ACitizen::CanWork(ABuilding* ReligiousBuilding)
{
	if (BioStruct.Age < 18)
		return false;

	if (ReligiousBuilding->Belief != EReligion::Atheist && ReligiousBuilding->Belief != Spirituality.Faith)
		return false;

	return true;
}

void ACitizen::FindJobAndHouse()
{
	if (GetWorld()->GetTimeSeconds() < TimeOfResidence + 300.0f && GetWorld()->GetTimeSeconds() < TimeOfEmployment + 300.0f)
		return;

	for (ABuilding* building : Camera->CitizenManager->Buildings) {
		if (building->GetCapacity() == building->GetOccupied().Num() || !CanWork(building) || !AIController->CanMoveTo(building->GetActorLocation()))
			continue;

		if (building->IsA<AHouse>()) {
			if (GetWorld()->GetTimeSeconds() < TimeOfResidence + 300.0f)
				continue;

			int32 currentRent = 0;
			int32 newRent = 0;

			for (FRentStruct rentStruct : Camera->CitizenManager->RentList) {
				if (Building.House->IsA(rentStruct.HouseType)) {
					currentRent = rentStruct.Rent;

					break;
				}
			}

			for (FRentStruct rentStruct : Camera->CitizenManager->RentList) {
				if (building->IsA(rentStruct.HouseType)) {
					newRent = rentStruct.Rent;

					break;
				}
			}

			if (Balance < newRent)
				continue;

			if (Building.Employment != nullptr && Building.House != nullptr) {
				double magnitude = AIController->GetClosestActor(Building.Employment->GetActorLocation(), Building.House->GetActorLocation(), building->GetActorLocation(), currentRent, newRent);

				if (magnitude <= 0.0f)
					continue;
			}
		}
		else if (GetWorld()->GetTimeSeconds() < TimeOfEmployment + 300.0f || (Building.Employment != nullptr && Building.Employment->Wage >= Cast<AWork>(building)->Wage))
			continue;

		AsyncTask(ENamedThreads::GameThread, [this, building]() { 
			if (Building.Employment != nullptr && building->IsA<AWork>())
				Building.Employment->RemoveCitizen(this);

			building->AddCitizen(this); 
		});
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

	if (totalAmount <= 0) {
		PopupComponent->SetHiddenInGame(false);

		SetActorTickEnabled(true);

		AsyncTask(ENamedThreads::GameThread, [this]() { SetPopupImageState("Add", "Hunger"); });

		return;
	}

	int32 maxF = FMath::CeilToInt((100 - Hunger) / 25.0f);
	int32 quantity = FMath::Clamp(totalAmount, 1, maxF);

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

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty() || (!Building.Employment->bCanRest && Building.Employment->bOpen) || bWorshipping)
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
	time /= FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed);

	FTimerStruct timer;
	timer.CreateTimer(this, time, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, Resource, Instance), false);

	Camera->CitizenManager->Timers.Add(timer);

	USoundBase* sound = nullptr;

	if (Resource->IsA<AMineral>())
		sound = Mines[FMath::RandRange(0, Mines.Num() - 1)];
	else
		sound = Chops[FMath::RandRange(0, Chops.Num() - 1)];

	timer.CreateTimer(this, 1, FTimerDelegate::CreateUObject(Camera, &ACamera::PlayAmbientSound, AmbientAudioComponent, sound), true);

	Camera->CitizenManager->Timers.Add(timer);

	AIController->StopMovement();
}

void ACitizen::HarvestResource(AResource* Resource, int32 Instance)
{
	AResource* resource = Resource->GetHarvestedResource();

	Camera->CitizenManager->RemoveTimer(this, FTimerDelegate::CreateUObject(Camera, &ACamera::PlayAmbientSound, AmbientAudioComponent, AmbientAudioComponent->GetSound()));

	Camera->CitizenManager->Injure(this);

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

	if (BioStruct.Age >= 60) {
		HealthComponent->MaxHealth = 50;
		HealthComponent->AddHealth(0);

		int32 chance = FMath::RandRange(1, 100);

		if (chance < 5)
			HealthComponent->TakeHealth(50, this);
	}

	if (BioStruct.Age <= 18) {
		HealthComponent->MaxHealth = 5 * BioStruct.Age + 10.0f;
		HealthComponent->AddHealth(5);

		float scale = (BioStruct.Age * 0.04f) + 0.28f;
		AsyncTask(ENamedThreads::GameThread, [this, scale]() { Mesh->SetWorldScale3D(FVector(scale, scale, scale)); });
	}
	else if (BioStruct.Partner != nullptr)
		AsyncTask(ENamedThreads::GameThread, [this]() { HaveChild(); });

	if (BioStruct.Age >= 18 && BioStruct.Partner == nullptr)
		FindPartner();

	if (BioStruct.Age == 18) {
		AttackComponent->bCanAttack = true;

		SetReligion();
	}

	if (BioStruct.Age >= 12)
		SetPolticalLeanings();
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

	for (AActor* actor : Camera->CitizenManager->Citizens) {
		ACitizen* c = Cast<ACitizen>(actor);

		if (c->BioStruct.Sex == BioStruct.Sex || c->BioStruct.Partner != nullptr || c->BioStruct.Age < 18)
			continue;

		if (citizen == nullptr) {
			citizen = c;

			continue;
		}

		double magnitude = AIController->GetClosestActor(GetActorLocation(), citizen->GetActorLocation(), c->GetActorLocation());

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
	float chance = FMath::FRandRange(0.0f, 100.0f);
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
	if (Politics.Ideology.Leaning == ESway::Radical)
		return;

	TArray<EParty> partyList;

	FPartyStruct partnersParty;
	TArray<FPartyStruct> buildingParties;

	if (BioStruct.Father->IsValidLowLevelFast())
		Politics.FathersIdeology = BioStruct.Father->Politics.Ideology;

	if (BioStruct.Mother->IsValidLowLevelFast())
		Politics.MothersIdeology = BioStruct.Mother->Politics.Ideology;

	if (BioStruct.Partner->IsValidLowLevelFast())
		partnersParty = BioStruct.Partner->Politics.Ideology;

	if (Building.Employment->IsValidLowLevelFast()) {
		buildingParties.Append(Building.Employment->Swing);

		for (ACitizen* citizen : Building.Employment->GetOccupied())
			if (citizen != this)
				buildingParties.Add(citizen->Politics.Ideology);
	}

	if (Building.House->IsValidLowLevelFast())
		buildingParties.Append(Building.House->Parties);

	for (int32 i = 0; i < Politics.FathersIdeology.Leaning; i++)
		partyList.Add(Politics.FathersIdeology.Party);

	for (int32 i = 0; i < Politics.MothersIdeology.Leaning; i++)
		partyList.Add(Politics.MothersIdeology.Party);

	for (int32 i = 0; i < partnersParty.Leaning; i++)
		partyList.Add(partnersParty.Party);

	for (FPartyStruct party : buildingParties)
		for (int32 i = 0; i < party.Leaning; i++)
			partyList.Add(party.Party);

	for (int32 i = 0; i < Politics.Ideology.Leaning; i++)
		partyList.Add(Politics.Ideology.Party);

	if (Spirituality.Faith != EReligion::Atheist)
		for (int32 i = 0; i < 2; i++)
			partyList.Add(EParty::Religious);

	int32 itterate = FMath::Floor(GetHappiness() / 10) - 5;

	if (itterate < 0)
		for (int32 i = 0; i < FMath::Abs(itterate); i++)
			partyList.Add(EParty::Freedom);

	if (partyList.IsEmpty())
		return;

	int32 index = FMath::RandRange(0, partyList.Num() - 1);

	int32 mark = FMath::RandRange(0, 100);
	int32 pass = 50;

	if (Politics.Ideology.Party == partyList[index]) {
		pass = 80;

		if (Politics.Ideology.Leaning == ESway::Strong)
			pass = 95;

		if (mark > pass) {
			if (Politics.Ideology.Leaning == ESway::Strong)
				Politics.Ideology.Leaning = ESway::Radical;
			else
				Politics.Ideology.Leaning = ESway::Strong;
		}
	}
	else if (mark > pass) {
		if (Politics.Ideology.Leaning != ESway::Strong) {
			Politics.Ideology.Party = partyList[index];

			if (Politics.Ideology.Party == EParty::Freedom && Camera->CitizenManager->IsRebellion())
				Camera->CitizenManager->SetupRebel(this);
		}

		Politics.Ideology.Leaning = ESway::Moderate;
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
	}

	if (BioStruct.Mother->IsValidLowLevelFast()) {
		Spirituality.MothersFaith = BioStruct.Mother->Spirituality.Faith;

		religionList.Add(Spirituality.MothersFaith);
	}

	if (BioStruct.Partner->IsValidLowLevelFast())
		religionList.Add(BioStruct.Partner->Spirituality.Faith);

	if (Building.Employment->IsValidLowLevelFast()) {
		if (Building.Employment->Belief != EReligion::Atheist)
			religionList.Add(Building.Employment->Belief);

		for (ACitizen* citizen : Building.Employment->GetOccupied())
			if (citizen != this)
				religionList.Add(citizen->Spirituality.Faith);
	}

	if (Building.House->IsValidLowLevelFast())
		religionList.Append(Building.House->Religions);

	religionList.Add(EReligion::Atheist);
	religionList.Add(EReligion::Fox);
	religionList.Add(EReligion::Chicken);
	religionList.Add(EReligion::Egg);

	int32 index = FMath::RandRange(0, religionList.Num() - 1);

	Spirituality.Faith = religionList[index];
}

void ACitizen::SetMassStatus(EMassStatus Status)
{
	MassStatus = Status;
}

// Happiness
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

	if (Building.House == nullptr)
		Happiness.SetValue("Homeless", -10);
	else
		Happiness.SetValue("Housed", 5);

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

	if (MassStatus == EMassStatus::Missed)
		Happiness.SetValue("Missed Mass", -25);
	else if (MassStatus == EMassStatus::Attended)
		Happiness.SetValue("Attended Mass", 15);

	if (GetHappiness() < 20)
		SadTimer++;
	else
		SadTimer = 0;

	if (SadTimer == 300)
		HealthComponent->TakeHealth(HealthComponent->GetHealth(), this);
}