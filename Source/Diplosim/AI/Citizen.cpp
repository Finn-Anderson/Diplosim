#include "Citizen.h"

#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"	
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "NavigationSystem.h"


#include "Resource.h"
#include "Buildings/Work.h"
#include "Buildings/House.h"
#include "HealthComponent.h"
#include "Map/Vegetation.h"

ACitizen::ACitizen()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->SetCapsuleRadius(8.0f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(11.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	CitizenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CitizenMesh"));
	CitizenMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	CitizenMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	CitizenMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CitizenMesh->SetupAttachment(RootComponent);
	CitizenMesh->bCastDynamicShadow = true;
	CitizenMesh->CastShadow = true;
	CitizenMesh->SetWorldScale3D(FVector(0.28f, 0.28f, 0.28f));
	CitizenMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -9.0f));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	House = nullptr;
	Employment = nullptr;

	Balance = 20;

	Energy = 100;

	Age = 0;

	Partner = nullptr;

	GetCharacterMovement()->MaxWalkSpeed = 300.0f;
}

void ACitizen::BeginPlay()
{
	Super::BeginPlay();

	CitizenMesh->OnComponentBeginOverlap.AddDynamic(this, &ACitizen::OnOverlapBegin);
	CitizenMesh->OnComponentEndOverlap.AddDynamic(this, &ACitizen::OnOverlapEnd);

	SpawnDefaultController();
	
	AIController = Cast<AAIController>(GetController());

	GetWorld()->GetTimerManager().SetTimer(EnergyTimer, this, &ACitizen::LoseEnergy, 6.0f, true);

	FTimerHandle ageTimer;
	GetWorld()->GetTimerManager().SetTimer(ageTimer, this, &ACitizen::Birthday, 60.0f, false);

	SetSex();

	float r = FMath::FRandRange(0.0f, 1.0f);
	float g = FMath::FRandRange(0.0f, 1.0f);
	float b = FMath::FRandRange(0.0f, 1.0f);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(CitizenMesh->GetMaterial(0), this);
	material->SetVectorParameterValue("Colour", FLinearColor(r, g, b));
	CitizenMesh->SetMaterial(0, material);
}

void ACitizen::MoveTo(AActor* Location)
{
	if (Location->IsValidLowLevelFast()) {
		UStaticMeshComponent* comp = Cast<UStaticMeshComponent>(Location->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		TArray<FName> sockets = comp->GetAllSocketNames();

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* NavData = nav->GetNavDataForProps(GetNavAgentPropertiesRef());
			
		FName socket = "";
		for (int32 i = 0; i < sockets.Num(); i++) {
			FName s = sockets[i];

			FPathFindingQuery query(this, *NavData, GetActorLocation(), comp->GetSocketLocation(s));
			query.bAllowPartialPaths = false;

			bool path = nav->TestPathSync(query, EPathFindingMode::Hierarchical);

			if (path) {
				if (socket == "") {
					socket = s;
				}
				else {
					float curSocketLoc = FVector::Dist(comp->GetSocketLocation(socket), GetActorLocation());

					float newSocketLoc = FVector::Dist(comp->GetSocketLocation(s), GetActorLocation());

					if (curSocketLoc > newSocketLoc) {
						socket = s;
					}
				}
			}
		}

		AIController->MoveToLocation(comp->GetSocketLocation(socket));

		Goal = Location;
	}
}

void ACitizen::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == Goal && OtherActor->IsA<AResource>()) {
		AResource* r = Cast<AResource>(OtherActor);

		FTimerHandle harvestTimer;
		float time = FMath::RandRange(6.0f, 10.0f);
		GetWorldTimerManager().SetTimer(harvestTimer, FTimerDelegate::CreateUObject(this, &ACitizen::HarvestResource, r), time, false);

		AIController->StopMovement();
	}
	else if (OtherActor->IsA<ACitizen>() && Partner == nullptr) {
		ACitizen* c = Cast<ACitizen>(OtherActor);

		SetPartner(c);
	}
}

void ACitizen::OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
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
		CitizenMesh->SetWorldScale3D(FVector(scale, scale, scale));

		int32 z = (Age * 0.33333333333) - 9;
		CitizenMesh->SetRelativeLocation(FVector(0.0f, 0.0f, z));
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