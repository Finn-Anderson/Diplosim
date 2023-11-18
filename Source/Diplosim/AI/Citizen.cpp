#include "Citizen.h"

#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"

#include "Buildings/Work.h"
#include "Buildings/House.h"
#include "Resource.h"
#include "HealthComponent.h"
#include "Map/Mineral.h"
#include "Enemy.h"

ACitizen::ACitizen()
{
	AIMesh->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));

	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BuildCapsuleCollision"));
	CapsuleCollision->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	CapsuleCollision->SetupAttachment(AIMesh);

	House = nullptr;
	Employment = nullptr;

	Balance = 20;

	Energy = 100;

	Age = 0;

	Partner = nullptr;
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

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(AIMesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	AIMesh->SetMaterial(0, material);
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
	else if (OtherActor->IsA<ACitizen>() && Partner == nullptr) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		SetPartner(c);
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA<ABuilding>()) {
		ABuilding* b = Cast<ABuilding>(OtherActor);

		b->Leave(this);
	}
}

void ACitizen::OnEnemyOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA<AEnemy>()) {
		OverlappingEnemies.Add(OtherActor);

		SetActorTickEnabled(true);
	}
}

void ACitizen::OnEnemyOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnEnemyOverlapEnd(OverlappedComp, OtherActor, OtherComp, OtherBodyIndex);

	if (OverlappingEnemies.IsEmpty()) {
		MoveTo(Employment);
	}
}

void ACitizen::StartLoseEnergyTimer()
{
	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);
}

void ACitizen::LoseEnergy()
{
	Energy = FMath::Clamp(Energy - 1, -100, 100);

	if (Energy <= 0 && House != nullptr) {
		MoveTo(House);
	}

	if (Energy == -100) {
		HealthComponent->TakeHealth(100, GetActorLocation() + GetVelocity());
	}
}

void ACitizen::StartGainEnergyTimer(int32 Max)
{
	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, FTimerDelegate::CreateUObject(this, &ACitizen::GainEnergy, Max), 0.6f, true);
}

void ACitizen::GainEnergy(int32 Max)
{
	Energy = FMath::Clamp(Energy + 1, -100, Max);

	if (Energy == Max) {
		MoveTo(Employment);
	}
}

void ACitizen::HarvestResource(AResource* Resource)
{
	Carry(Resource, Resource->GetYield(), Employment);
}

void ACitizen::Carry(AResource* Resource, int32 Amount, AActor* Location)
{
	Carrying.Type = Resource;
	Carrying.Amount = Amount;

	MoveTo(Location);
}

void ACitizen::Birthday()
{
	Age += 1;

	if (Age >= 60) {
		HealthComponent->MaxHealth = 50;
		HealthComponent->AddHealth(0);
	}

	if (Age <= 18) {
		int32 scale = (Age * 0.04) + 0.28;
		AIMesh->SetWorldScale3D(FVector(scale, scale, scale));
	}
}

void ACitizen::SetSex()
{
	int32 choice = FMath::RandRange(1, 100);

	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	int32 male = 0;

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (c->Sex == ESex::Male) {
			male += 1;
		}
	}

	float mPerc = (male / citizens.Num()) * 100;

	if (choice > mPerc) {
		Sex == ESex::Male;
	}
	else {
		Sex == ESex::Female;
	}
}

void ACitizen::SetPartner(ACitizen* Citizen)
{
	if (Citizen->Partner == this || (Citizen->Sex != Sex && Citizen->Partner == nullptr && Age > 17 && Citizen->Age > 17)) {
		Partner = Citizen;

		Citizen->SetPartner(this);

		if (Sex == ESex::Female) {
			GetWorld()->GetTimerManager().SetTimer(ChildTimer, this, &ACitizen::HaveChild, 90.0f, true);
		}
	}
}

void ACitizen::HaveChild()
{
	int32 chance = FMath::RandRange(1, 100);
	int32 likelihood = (Age / 60) * 100;

	if (chance > likelihood) {
		GetWorld()->SpawnActor<ACitizen>(ACitizen::GetClass(), GetActorLocation(), GetActorRotation());
	}
}