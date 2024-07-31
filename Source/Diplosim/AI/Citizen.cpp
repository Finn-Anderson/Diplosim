#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"

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

ACitizen::ACitizen()
{
	SetTickableWhenPaused(true);

	GetCapsuleComponent()->SetCapsuleSize(9.0f, 11.5f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -14.0f));
	GetMesh()->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

	HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
	HatMesh->SetCollisionProfileName("NoCollision", false);
	HatMesh->SetupAttachment(GetMesh(), "HatSocket");
	HatMesh->SetWorldScale3D(FVector(0.1f, 0.1f, 0.1f));

	TorchMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TorchMesh"));
	TorchMesh->SetCollisionProfileName("NoCollision", false);
	TorchMesh->SetupAttachment(GetMesh(), "TorchSocket");
	TorchMesh->SetHiddenInGame(true);

	TorchNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TorchNiagaraComponent"));
	TorchNiagaraComponent->SetupAttachment(TorchMesh, "ParticleSocket");
	TorchNiagaraComponent->SetHiddenInGame(true);

	DiseaseNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DiseaseNiagaraComponent"));
	DiseaseNiagaraComponent->SetupAttachment(GetMesh());
	DiseaseNiagaraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
	DiseaseNiagaraComponent->bAutoActivate = false;

	PopupComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PopupComponent"));
	PopupComponent->SetupAttachment(GetCapsuleComponent());
	PopupComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	PopupComponent->SetHiddenInGame(true);

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	HungerTimer = 0;
	EnergyTimer = 0;
	AgeTimer = 0;

	bGain = false;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;

	InitialSpeed = GetCharacterMovement()->MaxWalkSpeed;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	camera->CitizenManager->Citizens.Add(this);
	camera->CitizenManager->Infectible.Add(this);

	SetSex();
	SetName();

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	GetMesh()->SetMaterial(0, material);

	if (BioStruct.Mother != nullptr && BioStruct.Mother->Building.BuildingAt != nullptr)
		BioStruct.Mother->Building.BuildingAt->Enter(this);

	AIController->Idle();
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// Disease
	if (OtherActor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(OtherActor);

		APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		ACamera* camera = PController->GetPawn<ACamera>();

		if (citizen->Building.Employment == nullptr || !citizen->Building.Employment->IsA<AClinic>()) {
			for (FDiseaseStruct disease : CaughtDiseases) {
				if (citizen->CaughtDiseases.Contains(disease))
					continue;

				int32 chance = FMath::RandRange(1, 100);

				if (chance <= disease.Spreadability)
					citizen->CaughtDiseases.Add(disease);
			}

			if (!citizen->CaughtDiseases.IsEmpty() && !citizen->DiseaseNiagaraComponent->IsActive())
				camera->CitizenManager->Infect(citizen);
		}

		if (Building.Employment != nullptr && Building.Employment->IsA<AClinic>()) {
			camera->CitizenManager->Cure(citizen);

			if (AIController->MoveRequest.GetGoalActor() == citizen)
				camera->CitizenManager->PickCitizenToHeal(this);
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
void ACitizen::SetTorch()
{
	TorchMesh->SetHiddenInGame(!TorchMesh->bHiddenInGame);
	TorchNiagaraComponent->SetHiddenInGame(!TorchNiagaraComponent->bHiddenInGame);
}

//
// Work
//
bool ACitizen::CanWork(ABuilding* ReligiousBuilding)
{
	if (BioStruct.Age < 18)
		return false;

	if (ReligiousBuilding->Belief.Religion != EReligion::Undecided && ReligiousBuilding->Belief.Religion != Spirituality.Faith.Religion && Spirituality.Faith.Leaning > ESway::Moderate)
		return false;

	return true;
}

//
// Food
//
void ACitizen::Eat()
{
	HungerTimer = 0;

	Hunger = FMath::Clamp(Hunger - 1, 0, 100);

	if (Hunger > 25)
		return;
	else if (Hunger == 0)
		HealthComponent->TakeHealth(10, this);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;

	for (int32 i = 0; i < Food.Num(); i++) {
		int32 curAmount = camera->ResourceManager->GetResourceAmount(Food[i]);

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	if (totalAmount <= 0) {
		PopupComponent->SetHiddenInGame(false);

		SetActorTickEnabled(true);

		SetPopupImageState("Add", "Hunger");

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
				camera->ResourceManager->TakeUniversalResource(Food[j], 1, 0);

				foodAmounts[j] -= 1;
				totalAmount -= 1;

				break;
			}
		}

		Hunger = FMath::Clamp(Hunger + 25, 0, 100);
	}

	if (!PopupComponent->bHiddenInGame) {
		if (CaughtDiseases.IsEmpty()) {
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

	EnergyTimer = 0;
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, 60.0f, InitialSpeed);

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty())
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
	FTimerHandle harvestTimer;
	float time = FMath::RandRange(6.0f, 10.0f);
	time /= FMath::LogX(100.0f, FMath::Clamp(Energy, 2, 100));

	GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, Resource, Instance), time, false);

	AIController->StopMovement();
}

void ACitizen::HarvestResource(AResource* Resource, int32 Instance)
{
	AResource* resource = Resource->GetHarvestedResource();

	Carry(resource, Resource->GetYield(this, Instance), Building.Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

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
	AgeTimer = 0;

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
		GetMesh()->SetWorldScale3D(FVector(scale, scale, scale));
	}
	else if (BioStruct.Partner != nullptr)
		HaveChild();

	if (BioStruct.Age >= 18 && BioStruct.Partner == nullptr)
		FindPartner();

	if (BioStruct.Age == 18)
		AttackComponent->bCanAttack = true;

	if (BioStruct.Age >= 12) {
		SetPolticalLeanings();
		SetReligionLeanings();
	}
}

void ACitizen::SetSex()
{
	int32 choice = FMath::RandRange(1, 100);

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	float male = 0.0f;
	float total = 0.0f;

	for (AActor* actor : citizens) {
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

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (AActor* actor : citizens) {
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
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	float chance = FMath::FRandRange(0.0f, 100.0f);
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance < passMark)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation loc;
	nav->ProjectPointToNavigation(GetActorLocation() + GetActorForwardVector() * 10.0f, loc, FVector(200.0f, 200.0f, 10.0f));

	FVector location = loc.Location;

	if (Building.BuildingAt != nullptr)
		location = Building.EnterLocation;

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), location, GetActorRotation());
	
	if (!citizen->IsValidLowLevelFast())
		return;

	if (!TorchMesh->bHiddenInGame)
		citizen->SetTorch();

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

	for (int32 i = 0; i < Spirituality.Faith.Leaning; i++)
		partyList.Add(EParty::Religious);

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
		if (Politics.Ideology.Leaning != ESway::Strong)
			Politics.Ideology.Party = partyList[index];

		Politics.Ideology.Leaning = ESway::Moderate;
	}
}

//
// Religion
//
void ACitizen::SetReligionLeanings()
{
	if (Spirituality.Faith.Leaning == ESway::Radical)
		return;

	TArray<EReligion> religionList;

	FReligionStruct partnersReligion;
	TArray<FReligionStruct> buildingReligions;

	if (BioStruct.Father->IsValidLowLevelFast())
		Spirituality.FathersFaith = BioStruct.Father->Spirituality.Faith;

	if (BioStruct.Mother->IsValidLowLevelFast())
		Spirituality.MothersFaith = BioStruct.Mother->Spirituality.Faith;

	if (BioStruct.Partner->IsValidLowLevelFast())
		partnersReligion = BioStruct.Partner->Spirituality.Faith;

	if (Building.Employment->IsValidLowLevelFast()) {
		if (Building.Employment->Belief.Religion != EReligion::Undecided)
			buildingReligions.Add(Building.Employment->Belief);

		for (ACitizen* citizen : Building.Employment->GetOccupied())
			if (citizen != this)
				buildingReligions.Add(citizen->Spirituality.Faith);
	}

	if (Building.House->IsValidLowLevelFast())
		buildingReligions.Append(Building.House->Religions);

	for (int32 i = 0; i < Spirituality.FathersFaith.Leaning; i++)
		religionList.Add(Spirituality.FathersFaith.Religion);

	for (int32 i = 0; i < Spirituality.MothersFaith.Leaning; i++)
		religionList.Add(Spirituality.MothersFaith.Religion);

	for (int32 i = 0; i < partnersReligion.Leaning; i++)
		religionList.Add(partnersReligion.Religion);

	for (FReligionStruct faith : buildingReligions)
		for (int32 i = 0; i < faith.Leaning; i++)
			religionList.Add(faith.Religion);

	int32 lean = Spirituality.Faith.Leaning;

	if (Spirituality.bBoost)
		lean *= 2;

	for (int32 i = 0; i < lean; i++)
		religionList.Add(Spirituality.Faith.Religion);

	if (religionList.IsEmpty())
		return;

	int32 index = FMath::RandRange(0, religionList.Num() - 1);

	int32 mark = FMath::RandRange(0, 100);
	int32 pass = 50;

	if (Spirituality.Faith.Religion == religionList[index]) {
		pass = 80;

		if (Spirituality.Faith.Leaning == ESway::Strong)
			pass = 95;

		if (mark > pass) {
			if (Spirituality.Faith.Leaning == ESway::Strong)
				Spirituality.Faith.Leaning = ESway::Radical;
			else
				Spirituality.Faith.Leaning = ESway::Strong;
		}
	}
	else if(mark > pass) {
		if (Spirituality.Faith.Leaning != ESway::Strong)
			Spirituality.Faith.Religion = religionList[index];

		Spirituality.Faith.Leaning = ESway::Moderate;
	}
}