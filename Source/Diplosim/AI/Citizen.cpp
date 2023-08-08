#include "Citizen.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

#include "Resource.h"
#include "Buildings/Work.h"
#include "Buildings/House.h"
#include "HealthComponent.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	CitizenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CitizenMesh"));
	CitizenMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	CitizenMesh->bCastDynamicShadow = true;
	CitizenMesh->CastShadow = true;
	CitizenMesh->SetWorldScale3D(FVector(0.28, 0.28, 0.28));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	House = nullptr;
	Employment = nullptr;

	Balance = 20;

	Carrying = 0;

	Energy = 100;

	Age = 0;

	Partner = nullptr;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	CitizenMesh->OnComponentBeginOverlap.AddDynamic(this, &ACitizen::OnOverlapBegin);
	
	aiController = Cast<AAIController>(GetController());

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);

	FTimerHandle ageTimer;
	GetWorld()->GetTimerManager().SetTimer(ageTimer, this, &ACitizen::Birthday, 60.0f, false);

	SetSex();

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(CitizenMesh->GetMaterial(0), NULL);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
}

void ACitizen::MoveTo(AActor* Location)
{
	aiController->MoveToActor(Location, 10.0f, true);
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal && OtherActor->IsA<AResource>()) {
		AResource* r = Cast<AResource>(OtherActor);

		FTimerHandle harvestTimer;
		GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::Carry, r), 2.0f, false);
	}
	else if (OtherActor->IsA<ACitizen>() && Partner == nullptr) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		SetPartner(c);
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

		Goal = House;
	}

	if (Energy == -100) {
		HealthComponent->TakeHealth(100, GetActorLocation() + GetVelocity());
	}
}

void ACitizen::StartGainEnergyTimer()
{
	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::GainEnergy, 0.6f, true);
}

void ACitizen::GainEnergy()
{
	Energy = FMath::Clamp(Energy + 1, -100, 100);

	if (Energy == 100) {
		MoveTo(Employment);

		Goal = Employment;
	}
}

void ACitizen::Carry(AResource* Resource)
{
	Carrying = Resource->GetYield();

	MoveTo(Employment);

	Goal = Employment;
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
		CitizenMesh->SetWorldScale3D(FVector(scale, scale, scale));
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
			GetWorld()->GetTimerManager().SetTimer(ChildTimer, this, &ACitizen::HaveChild, 120.0f, true);
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