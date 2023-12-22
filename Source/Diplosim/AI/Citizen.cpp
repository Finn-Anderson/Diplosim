#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"

#include "Buildings/Work.h"
#include "Buildings/House.h"
#include "Buildings/Broch.h"
#include "Resource.h"
#include "HealthComponent.h"
#include "AttackComponent.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

ACitizen::ACitizen()
{
	GetCapsuleComponent()->SetCapsuleSize(9.0f, 11.5f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -11.5f));
	GetMesh()->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BuildCapsuleCollision"));
	CapsuleCollision->SetCapsuleSize(18.0f, 23.0f);
	CapsuleCollision->SetupAttachment(RootComponent);

	Balance = 20;

	Hunger = 100;
	Energy = 100;

	HealthComponent->MaxHealth = 10;
	HealthComponent->Health = HealthComponent->MaxHealth;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &ACitizen::OnOverlapBegin);
	CapsuleCollision->OnComponentEndOverlap.AddDynamic(this, &ACitizen::OnOverlapEnd);

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);

	GetWorld()->GetTimerManager().SetTimer(HungerTimer, this, &ACitizen::Eat, 3.0f, true);

	FTimerHandle ageTimer;
	GetWorld()->GetTimerManager().SetTimer(ageTimer, this, &ACitizen::Birthday, 60.0f, false);

	SetSex();

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	GetMesh()->SetMaterial(0, material);

	TArray<AActor*> brochs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABroch::StaticClass(), brochs);

	MoveTo(brochs[0]);
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal) {
		if (OtherActor->IsA<AResource>()) {
			AResource* r = Cast<AResource>(OtherActor);

			FTimerHandle harvestTimer;
			float time = FMath::RandRange(6.0f, 10.0f);
			GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, r), time, false);

			AIController->StopMovement();
		}
		else if (OtherActor->IsA<ABuilding>()) {
			ABuilding* b = Cast<ABuilding>(OtherActor);

			b->Enter(this);
		}
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

//
// Food
//
void ACitizen::Eat()
{
	Hunger--;

	if (Hunger > 25)
		return;

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	TArray<int32> foodAmounts;
	int32 totalAmount = 0;
	for (int32 i = 0; i < Food.Num(); i++) {
		int32 curAmount = camera->ResourceManagerComponent->GetResourceAmount(Food[i]);

		foodAmounts.Add(curAmount);
		totalAmount += curAmount;
	}

	if (totalAmount < 0)
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

	if (Hunger == 0) {
		HealthComponent->TakeHealth(100, GetActorLocation());
	}
}

//
// Energy
//
void ACitizen::SetEnergyTimer(bool bGain)
{
	auto func = &ACitizen::LoseEnergy;

	if (bGain) {
		func = &ACitizen::GainEnergy;
	}

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, func, 6.0f, true);
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	if (Energy > 20 || !AttackComponent->OverlappingEnemies.IsEmpty())
		return;

	if (Building.House->IsValidLowLevelFast()) {
		MoveTo(Building.House);
	}
	else if (BioStruct.Mother != nullptr && BioStruct.Mother->Building.House->IsValidLowLevelFast()) {
		MoveTo(BioStruct.Mother->Building.House);
	}

	if (Energy == 0) {
		HealthComponent->TakeHealth(100, GetActorLocation() + GetVelocity());

		GetWorld()->GetTimerManager().ClearTimer(EnergyTimer);
	}
}

void ACitizen::GainEnergy()
{
	Energy = FMath::Clamp(Energy + 1, 0, 100);

	HealthComponent->AddHealth(1);

	if (Energy >= 100) {
		MoveTo(Building.Employment);
	}
}

//
// Resources
//
void ACitizen::HarvestResource(AResource* Resource)
{
	Carry(Resource, Resource->GetYield(), Building.Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

	if (Location == nullptr) {
		AIController->StopMovement();
	}
	else {
		MoveTo(Location);
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
			HealthComponent->TakeHealth(50);
		}
	}

	if (BioStruct.Age <= 18) {
		HealthComponent->MaxHealth += 5;

		int32 scale = (BioStruct.Age * 0.04) + 0.28;
		GetMesh()->SetWorldScale3D(FVector(scale, scale, scale));
	}
	else if (BioStruct.Partner != nullptr) {
		HaveChild();
	}

	if (BioStruct.Age >= 18) {
		FindPartner();
	}

	HealthComponent->Health += 5;
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

		if (!CanMoveTo(c) || c->BioStruct.Sex == BioStruct.Sex || c->BioStruct.Partner != nullptr || c->BioStruct.Age < 18) {
			continue;
		}

		citizen = Cast<ACitizen>(GetClosestActor(this, citizen, c));
	}

	if (citizen != nullptr) {
		SetPartner(citizen);

		citizen->SetPartner(this);
	}
}

void ACitizen::SetPartner(ACitizen* Citizen)
{
	BioStruct.Partner = Citizen;

	if (BioStruct.Sex == ESex::Female) {
		GetWorld()->GetTimerManager().SetTimer(ChildTimer, this, &ACitizen::HaveChild, 45.0f, true);
	}
}

void ACitizen::HaveChild()
{
	float chance = FMath::FRandRange(0.0f, 100.0f);
	float passMark = FMath::LogX(60.0f, BioStruct.Age) * 100.0f;

	if (chance > passMark) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), GetActorLocation(), GetActorRotation());
		citizen->BioStruct.Mother = this;
		citizen->BioStruct.Father = BioStruct.Father;
	}
}