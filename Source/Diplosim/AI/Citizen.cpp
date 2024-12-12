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

	bHasBeenLeader = false;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ProductivityMultiplier = 1.0f;
	Fertility = 1.0f;
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
bool ACitizen::CanWork(ABuilding* Building)
{
	if (BioStruct.Age < Camera->CitizenManager->GetLawValue(EBillType::WorkAge))
		return false;

	if (Building->Belief != EReligion::Atheist && Building->Belief != Spirituality.Faith)
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
		else if (GetWorld()->GetTimeSeconds() < TimeOfEmployment + 300.0f)
			continue;

		int32* value = Happiness.Modifiers.Find("Work Happiness");

		int32 wage = Cast<AWork>(building)->Wage;

		if (IsValid(Building.Employment))
			wage -= Building.Employment->Wage;

		if (*value >= 0 && wage <= 0)
			continue;

		int32 diff = -*value + wage;

		int32 chance = FMath::RandRange(0, 100);

		if (chance < 60 - (diff * 15))
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

	int32 maxF = FMath::CeilToInt((100 - Hunger) / 25.0f);
	int32 quantity = FMath::Clamp(totalAmount, 0, maxF);

	quantity = FMath::Min(quantity, Balance / Camera->CitizenManager->FoodCost);

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
	time /= (FMath::LogX(MovementComponent->InitialSpeed, MovementComponent->MaxSpeed) * ProductivityMultiplier);

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
		HealthComponent->MaxHealth /= 2;
		HealthComponent->AddHealth(0);

		int32 chance = FMath::RandRange(1, 100);

		if (chance < 5)
			HealthComponent->TakeHealth(HealthComponent->MaxHealth, this);
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

	FPartyStruct* party = Camera->CitizenManager->GetMembersParty(this);

	TEnumAsByte<ESway>* sway = party->Members.Find(this);

	if (sway->GetValue() == ESway::Radical)
		return;

	if (Camera->CitizenManager->Representatives.Contains(this))
		for (int32 i = 0; i < sway->GetIntValue(); i++)
			partyList.Add(party->Party);

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
	}

	if (BioStruct.Mother->IsValidLowLevelFast()) {
		Spirituality.MothersFaith = BioStruct.Mother->Spirituality.Faith;

		religionList.Add(Spirituality.MothersFaith);
	}

	if (BioStruct.Partner->IsValidLowLevelFast())
		religionList.Add(BioStruct.Partner->Spirituality.Faith);

	religionList.Add(Spirituality.Faith);

	if (Building.Employment->IsValidLowLevelFast() && Building.Employment->Belief != EReligion::Atheist)
		religionList.Add(Building.Employment->Belief);

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

	if (Building.House == nullptr)
		Happiness.SetValue("Homeless", -20);
	else
		Happiness.SetValue("Housed", 10);

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

	if (Spirituality.Faith != EReligion::Atheist && IsValid(Building.House)) {
		if (!Building.House->Religions.Contains(Spirituality.Faith))
			Happiness.SetValue("No Nearby Churches", -25);
		else
			Happiness.SetValue("Church Nearby", 15);
	}

	if (IsValid(Building.Employment) && GetWorld()->GetTimeSeconds() >= TimeOfEmployment + 300.0f) {
		int32 count = 0;

		for (ACitizen* citizen : Building.Employment->GetOccupied()) {
			if (citizen == this)
				return;

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

	int32 lawTally = 0;

	for (FLawStruct law : Camera->CitizenManager->Laws) {
		if (law.BillType == EBillType::Abolish)
			continue;

		FBillStruct billStruct;
		billStruct.bIsLaw = true;

		int32 index = law.Bills.Find(billStruct);

		if (index == INDEX_NONE)
			continue;

		FPartyStruct* party = Camera->CitizenManager->GetMembersParty(this);

		if (law.Bills[index].Agreeing.Contains(party->Party))
			lawTally++;
		else if (law.Bills[index].Opposing.Contains(party->Party))
			lawTally--;
	}

	if (lawTally < -2)
		Happiness.SetValue("Not Represented", -20);
	else if (lawTally > 2)
		Happiness.SetValue("Represented", 15);

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

	if (chance > 45 * parentsPersonalities.Num()) {
		int32 index = FMath::RandRange(0, Camera->CitizenManager->Personalities.Num());

		Camera->CitizenManager->Personalities[index].Citizens.Add(this);

		ApplyTraitAffect(Camera->CitizenManager->Personalities[index].Trait);
	}
	else {
		int32 index = FMath::RandRange(0, parentsPersonalities.Num());

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
	else if (Trait == EPersonality::Talented || Trait == EPersonality::Diligent)
		ProductivityMultiplier *= 1.15f;
	else if (Trait == EPersonality::Inept || Trait == EPersonality::Lazy)
		ProductivityMultiplier *= 0.85f;
	else if (Trait == EPersonality::Energetic) {
		MovementComponent->InitialSpeed *= 1.15f;
		MovementComponent->MaxSpeed *= 1.15f;
	}
	else if (Trait == EPersonality::Lethargic) {
		MovementComponent->InitialSpeed *= 0.85f;
		MovementComponent->MaxSpeed *= 0.85f;
	}
	else if (Trait == EPersonality::Brave || Trait == EPersonality::Inept)
		AttackComponent->DamageMultiplier *= 1.5f;
	else if (Trait == EPersonality::Craven)
		AttackComponent->DamageMultiplier *= 0.5f;
}