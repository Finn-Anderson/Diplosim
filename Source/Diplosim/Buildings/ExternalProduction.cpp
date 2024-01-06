#include "ExternalProduction.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "Resource.h"
#include "Map/Vegetation.h"
#include "Player/Camera.h"
#include "Player/ResourceManager.h"

AExternalProduction::AExternalProduction()
{
	RangeComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeComponent"));
	RangeComponent->SetupAttachment(RootComponent);
	RangeComponent->SetCollisionProfileName("Spectator", true);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	RangeComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	RangeComponent->SetSphereRadius(1500.0f);
	RangeComponent->SetHiddenInGame(false);
}

void AExternalProduction::BeginPlay()
{
	Super::BeginPlay();

	RangeComponent->OnComponentBeginOverlap.AddDynamic(this, &AExternalProduction::OnResourceOverlapBegin);
}

void AExternalProduction::OnResourceOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(Camera->ResourceManagerComponent->GetResource(this))) {
		ResourceList.Add(Cast<AResource>(OtherActor));
	}
}

void AExternalProduction::Enter(ACitizen* Citizen)
{
	Super::Enter(Citizen);

	if (GetOccupied().Contains(Citizen)) {
		Store(Citizen->Carrying.Amount, Citizen);
	}
}

void AExternalProduction::Production(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt != this)
		return;

	if (!Resource->IsValidLowLevelFast() || Resource->WorkerCount == Resource->MaxWorkers || Resource->Quantity <= 0) {
		Resource = nullptr;

		for (AResource* r : ResourceList) {
			if (r->IsHidden() || r->Quantity <= 0 || r->WorkerCount == r->MaxWorkers)
				continue;

			FClosestStruct closestStruct = Citizen->AIController->GetClosestActor(Resource, r);

			Resource = Cast<AResource>(closestStruct.Actor);
		}
	}

	if (Resource != nullptr) {
		Resource->WorkerCount++;

		Citizen->AIController->AIMoveTo(Resource);
	}
	else {
		GetWorldTimerManager().SetTimer(ProdTimer, FTimerDelegate::CreateUObject(this, &AExternalProduction::Production, Citizen), 30.0f, false);
	}
}