#include "Work.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Buildings/House.h"

AWork::AWork()
{
	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(1500.0f, 1500.0f, 1500.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90, 0, 0));
	DecalComponent->SetVisibility(false);

	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetSphereRadius(1500.0f);

	bRange = false;

	Wage = 0;
}

void AWork::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AWork::OnRadialOverlapBegin);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AWork::OnRadialOverlapEnd);
}

void AWork::OnRadialOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bRange || !OtherActor->IsA<AHouse>() || GetOccupied().IsEmpty())
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	house->Religions.Add(Belief);
	
	for (FPartyStruct party : Swing)
		house->Parties.Add(party);

	Houses.Add(house);
}

void AWork::OnRadialOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Houses.Contains(OtherActor))
		return;

	AHouse* house = Cast<AHouse>(OtherActor);

	if (Belief.Religion != EReligion::Atheist)
		house->Religions.RemoveSingle(Belief);

	for (FPartyStruct party : Swing)
		house->Parties.RemoveSingle(party);

	Houses.Remove(house);
}

void AWork::UpkeepCost()
{
	for (int i = 0; i < Occupied.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(Occupied[i]);

		c->Balance += Wage;
	}

	int32 upkeep = Wage * Occupied.Num();
	Camera->ResourceManager->TakeUniversalResource(Money, upkeep, -100000);
}

void AWork::FindCitizens()
{
	TArray<AActor*> citizens;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), citizens);

	for (int i = 0; i < citizens.Num(); i++) {
		ACitizen* c = Cast<ACitizen>(citizens[i]);

		if (!c->CanWork(this) || c->Building.Employment != nullptr || !c->AIController->CanMoveTo(GetActorLocation()))
			continue;
			
		AddCitizen(c);

		if (GetCapacity() == GetOccupied().Num())
			return;
	}

	if (GetCapacity() > GetOccupied().Num())
		GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);
}

bool AWork::AddCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::AddCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = this;

	Citizen->HatMesh->SetStaticMesh(WorkHat);

	Citizen->AIController->AIMoveTo(this);

	return true;
}

bool AWork::RemoveCitizen(ACitizen* Citizen)
{
	bool bCheck = Super::RemoveCitizen(Citizen);

	if (!bCheck)
		return false;

	Citizen->Building.Employment = nullptr;

	Citizen->HatMesh->SetStaticMesh(nullptr);

	if (!GetWorldTimerManager().IsTimerActive(FindTimer))
		GetWorldTimerManager().SetTimer(FindTimer, this, &AWork::FindCitizens, 30.0f, false);

	return true;
}

void AWork::Production(ACitizen* Citizen)
{
	StoreResource(Citizen);
}