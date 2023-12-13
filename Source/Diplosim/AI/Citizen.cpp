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

ACitizen::ACitizen()
{
	GetMesh()->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BuildCapsuleCollision"));
	CapsuleCollision->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	CapsuleCollision->SetupAttachment(GetMesh());

	Balance = 20;

	Energy = 100;

	Age = 0;

	Partner = nullptr;

	Sex = ESex::NaN;

	HealthComponent->Health = 10;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &ACitizen::OnOverlapBegin);
	CapsuleCollision->OnComponentEndOverlap.AddDynamic(this, &ACitizen::OnOverlapEnd);

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);

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
	if (OtherActor->IsA<ABuilding>() && Cast<ABuilding>(OtherActor)->BuildStatus != EBuildStatus::Blueprint) {
		ABuilding* b = Cast<ABuilding>(OtherActor);

		b->Leave(this);
	}
}

void ACitizen::StartLoseEnergyTimer()
{
	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, 0, 100);

	if (Energy <= 15 && Building.House->IsValidLowLevelFast()) {
		MoveTo(Building.House);
	}

	if (Energy == 0) {
		HealthComponent->TakeHealth(100, GetActorLocation() + GetVelocity());

		GetWorld()->GetTimerManager().ClearTimer(EnergyTimer);
	}
}

void ACitizen::StartGainEnergyTimer(int32 Max)
{
	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, FTimerDelegate::CreateUObject(this, &ACitizen::GainEnergy, Max), 0.6f, true);
}

void ACitizen::GainEnergy(int32 Max)
{
	Energy = FMath::Clamp(Energy + 1, 0, Max);

	HealthComponent->AddHealth(1);

	if (Energy >= Max) {
		MoveTo(Building.Employment);
	}
}

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

void ACitizen::Birthday()
{
	Age += 1;

	if (Age >= 60) {
		HealthComponent->MaxHealth = 50;
		HealthComponent->AddHealth(0);

		int32 chance = FMath::RandRange(1, 100);

		if (chance < 5) {
			HealthComponent->TakeHealth(50);
		}
	}

	if (Age <= 18) {
		HealthComponent->Health += 5;

		int32 scale = (Age * 0.04) + 0.28;
		GetMesh()->SetWorldScale3D(FVector(scale, scale, scale));
	}
	else if (Partner != nullptr) {
		HaveChild();
	}

	if (Age >= 18) {
		FindPartner();
	}
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

		if (c->Sex == ESex::Male) {
			male += 1.0f;
		}

		total += 1.0f;
	}

	float mPerc = 50.0f;

	if (total > 0) {
		mPerc = (male / total) * 100.0f;
	}

	if (choice > mPerc) {
		Sex = ESex::Male;
	}
	else {
		Sex = ESex::Female;
	}
}

void ACitizen::FindPartner()
{
	ACitizen* citizen = nullptr;

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (!CanMoveTo(c) || c->Sex == Sex || c->Partner != nullptr || c->Age < 18) {
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
	Partner = Citizen;

	if (Sex == ESex::Female) {
		GetWorld()->GetTimerManager().SetTimer(ChildTimer, this, &ACitizen::HaveChild, 45.0f, true);
	}
}

void ACitizen::HaveChild()
{
	float chance = FMath::FRandRange(0.0f, 100.0f);
	float passMark = FMath::LogX(60.0f, Age) * 100.0f;

	if (chance > passMark) {
		GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), GetActorLocation(), GetActorRotation());
	}
}