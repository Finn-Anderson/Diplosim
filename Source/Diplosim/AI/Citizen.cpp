#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Resource.h"
#include "HealthComponent.h"
#include "AttackComponent.h"
#include "DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"
#include "Buildings/ExternalProduction.h"
#include "InteractableInterface.h"
#include "DiplosimUserSettings.h"

ACitizen::ACitizen()
{
	GetCapsuleComponent()->SetCapsuleSize(9.0f, 11.5f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -11.5f));
	GetMesh()->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;

	InitialSpeed = GetCharacterMovement()->MaxWalkSpeed;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);

	GetWorld()->GetTimerManager().SetTimer(HungerTimer, this, &ACitizen::Eat, 3.0f, true);

	FTimerHandle ageTimer;
	GetWorld()->GetTimerManager().SetTimer(ageTimer, this, &ACitizen::Birthday, 20.0f, false);

	SetSex();

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
// Food
//
void ACitizen::Eat()
{
	Hunger = FMath::Clamp(Hunger - 1, 0, 100);

	InteractableComponent->SetHunger();
	InteractableComponent->ExecuteEditEvent("Hunger");

	if (Hunger > 25)
		return;
	else if (Hunger == 0)
		HealthComponent->TakeHealth(10, this);

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;

	for (int32 i = 0; i < Food.Num(); i++) {
		int32 curAmount = camera->ResourceManagerComponent->GetResourceAmount(Food[i]);

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	if (totalAmount <= 0)
		return;

	int32 maxF = FMath::CeilToInt((100 - Hunger) / 25.0f);
	int32 quantity = FMath::Clamp(totalAmount, 1, maxF);

	for (int32 i = 0; i < quantity; i++) {
		int32 selected = FMath::RandRange(0, totalAmount - 1);

		for (int32 j = 0; j < foodAmounts.Num(); j++) {
			if (foodAmounts[j] <= selected) {
				selected -= foodAmounts[j];
			}
			else {
				camera->ResourceManagerComponent->TakeUniversalResource(Food[j], 1, 0);

				foodAmounts[j] -= 1;
				totalAmount -= 1;

				break;
			}
		}

		Hunger = FMath::Clamp(Hunger + 25, 0, 100);
	}
}

//
// Energy
//
void ACitizen::SetEnergyTimer(bool bGain)
{
	auto func = &ACitizen::LoseEnergy;

	if (bGain)
		func = &ACitizen::GainEnergy;

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, func, 6.0f, true);
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	InteractableComponent->SetEnergy();
	InteractableComponent->ExecuteEditEvent("Energy");

	GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp(FMath::LogX(InitialSpeed, InitialSpeed * (Energy / 100.0f)) * InitialSpeed, 60.0f, InitialSpeed);

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty())
		return;

	if (Building.House->IsValidLowLevelFast()) {
		AIController->AIMoveTo(Building.House);

		if (!Building.Employment->IsA<AExternalProduction>())
			return;

		for (FValidResourceStruct validResourceStruct : Cast<AExternalProduction>(Building.Employment)->ValidResourceList) {
			for (FWorkerStruct workerStruct : validResourceStruct.Resource->WorkerStruct) {
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

	InteractableComponent->ExecuteEditEvent("Energy");

	HealthComponent->AddHealth(1);

	if (Energy >= 100 && Building.Employment != nullptr) {
		AIController->AIMoveTo(Building.Employment);
	}
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
	Carry(Resource, Resource->GetYield(this, Instance), Building.Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

	if (Location == nullptr) {
		AIController->StopMovement();
	}
	else {
		AIController->AIMoveTo(Location);
	}
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

		if (chance < 5) {
			HealthComponent->TakeHealth(50, this);
		}
	}

	if (BioStruct.Age <= 18) {
		HealthComponent->MaxHealth = 5 * BioStruct.Age + 10.0f;
		HealthComponent->AddHealth(5);

		int32 scale = (BioStruct.Age * 0.04) + 0.28;
		GetMesh()->SetWorldScale3D(FVector(scale, scale, scale));
	}
	else if (BioStruct.Partner != nullptr)
		HaveChild();

	if (BioStruct.Age >= 18)
		FindPartner();
}

void ACitizen::SetSex()
{
	int32 choice = FMath::RandRange(1, 100);

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	float male = 0.0f;
	float total = 0.0f;

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (c == this)
			continue;

		if (c->BioStruct.Sex == ESex::Male) {
			male += 1.0f;
		}

		total += 1.0f;
	}

	float mPerc = 50.0f;

	if (total > 0) {
		mPerc = (male / total) * 100.0f;
	}

	if (choice > mPerc) {
		BioStruct.Sex = ESex::Male;
	}
	else {
		BioStruct.Sex = ESex::Female;
	}
}

void ACitizen::FindPartner()
{
	ACitizen* citizen = nullptr;

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

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

	if (BioStruct.Sex == ESex::Female)
		GetWorld()->GetTimerManager().SetTimer(ChildTimer, this, &ACitizen::HaveChild, 45.0f, true);
}

void ACitizen::HaveChild()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	if (UDiplosimUserSettings::GetDiplosimUserSettings()->GetMaxCitizens() == citizens.Num())
		return;

	float chance = FMath::FRandRange(0.0f, 100.0f);
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance < passMark)
		return;

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation loc;
	nav->ProjectPointToNavigation(GetActorLocation() + GetActorForwardVector() * 10.0f, loc, FVector(200.0f, 200.0f, 10.0f));

	ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), loc.Location, GetActorRotation());
	
	if (!citizen->IsValidLowLevelFast())
		return;

	citizen->BioStruct.Mother = this;
	citizen->BioStruct.Father = BioStruct.Partner;
}