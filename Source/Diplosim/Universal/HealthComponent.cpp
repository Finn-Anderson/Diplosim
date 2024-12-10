#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/Projectile.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	CrumbleLocation = FVector::Zero();

	HealthMultiplier = 1.0f;
}

void UHealthComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ABuilding* building = Cast<ABuilding>(GetOwner());

	float height = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	SetComponentTickEnabled(true);

	FVector location = FMath::VInterpTo(building->GetActorLocation(), building->GetActorLocation() - FVector(0.0f, 0.0f, height + 1.0f), DeltaTime, 0.15f);

	FVector difference = building->GetActorLocation() - location;

	building->SetActorLocation(location);

	building->DestructionComponent->SetRelativeLocation(building->DestructionComponent->GetRelativeLocation() + difference);

	if (CrumbleLocation.Z <= building->GetActorLocation().Z)
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, building->GetActorLocation(), 0.0f, 2000.0f, 1.0f);
	else
		building->DestructionComponent->Deactivate();
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker)
{
	AsyncTask(ENamedThreads::GameThread, [this, Amount, Attacker]() {
		Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

		float opacity = (MaxHealth - Health) / float(MaxHealth);

		UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(OnHitEffect, GetOwner());
		material->SetScalarParameterValue("Opacity", opacity);

		if (GetOwner()->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(GetOwner());

			UConstructionManager* cm = building->Camera->ConstructionManager;
			cm->AddBuilding(building, EBuildStatus::Damaged);

			building->BuildingMesh->SetOverlayMaterial(material);
		}
		else
			Cast<AAI>(GetOwner())->Mesh->SetOverlayMaterial(material);

		FTimerHandle matTimer;
		GetWorld()->GetTimerManager().SetTimer(matTimer, FTimerDelegate::CreateUObject(this, &UHealthComponent::RemoveDamageOverlay), 0.15f, false);

		if (Health == 0)
			Death(Attacker);
	});
}

bool UHealthComponent::IsMaxHealth()
{
	if (Health != MaxHealth)
		return false;

	return true;
}

void UHealthComponent::RemoveDamageOverlay()
{
	if (GetOwner()->IsA<ABuilding>())
		Cast<ABuilding>(GetOwner())->BuildingMesh->SetOverlayMaterial(nullptr);
	else
		Cast<AAI>(GetOwner())->Mesh->SetOverlayMaterial(nullptr);
}

void UHealthComponent::Death(AActor* Attacker)
{
	AActor* actor = GetOwner();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	if (actor->IsA<ABroch>())
		camera->Lose();

	if (actor->IsA<AAI>()) {
		USkeletalMeshComponent* mesh = actor->GetComponentByClass<USkeletalMeshComponent>();
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		attackComp->RangeComponent->SetGenerateOverlapEvents(false);
		Cast<AAI>(actor)->Reach->SetGenerateOverlapEvents(false);

		attackComp->ClearAttacks();

		mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		mesh->SetSimulatePhysics(true);

		const FVector direction = (actor->GetActorLocation() - Attacker->GetActorLocation()).Rotation().Vector() * 50;
		mesh->SetPhysicsLinearVelocity(direction, false);

		Cast<AAI>(actor)->DetachFromControllerPendingDestroy();

		if (actor->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(actor);

			citizen->PopupComponent->SetHiddenInGame(true);

			camera->CitizenManager->ClearCitizen(citizen);
		}
	} 
	else if (actor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(actor);

		for (ACitizen* citizen : building->GetOccupied())
			building->RemoveCitizen(citizen);

		for (AActor* citizen : building->Inside)
			Cast<ACitizen>(citizen)->HealthComponent->TakeHealth(100, Attacker);

		building->BuildingMesh->SetGenerateOverlapEvents(false);

		UConstructionManager* cm = building->Camera->ConstructionManager;
		cm->RemoveBuilding(building);

		camera->CitizenManager->Buildings.Remove(building);

		FVector dimensions = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		CrumbleLocation = building->GetActorLocation() - FVector(0.0f, 0.0f, dimensions.Z);

		building->DestructionComponent->SetVariableVec2(TEXT("XY"), FVector2D(dimensions.X, dimensions.Y));
		building->DestructionComponent->SetVariableFloat(TEXT("Radius"), dimensions.X / 2);

		building->DestructionComponent->Activate();

		SetComponentTickEnabled(true);
	}

	FTimerHandle clearTimer;
	GetWorld()->GetTimerManager().SetTimer(clearTimer, FTimerDelegate::CreateUObject(this, &UHealthComponent::Clear, Attacker), 10.0f, false);

	actor->SetActorTickEnabled(false);
	GetWorld()->GetTimerManager().ClearAllTimersForObject(actor);
}

void UHealthComponent::Clear(AActor* Attacker)
{
	AActor* actor = GetOwner();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if (actor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->Building.Employment != nullptr)
			citizen->Building.Employment->RemoveCitizen(citizen);

		if (citizen->Building.House != nullptr)
			citizen->Building.House->RemoveCitizen(citizen);

		if (citizen->BioStruct.Partner != nullptr) {
			citizen->BioStruct.Partner->BioStruct.Partner = nullptr;
			citizen->BioStruct.Partner = nullptr;
		}

		if (Attacker->IsA<AEnemy>())
			gamemode->WavesData.Last().NumKilled++;
	}
	else if (actor->IsA<AEnemy>()) {
		if (Attacker->IsA<AProjectile>())
			gamemode->WavesData.Last().SetDiedTo(Attacker->GetOwner());
		else
			gamemode->WavesData.Last().SetDiedTo(Attacker);

		if (gamemode->CheckEnemiesStatus())
			gamemode->SetWaveTimer();
	}

	actor->Destroy();
}

int32 UHealthComponent::GetHealth()
{
	return Health;
}

void UHealthComponent::SetHealthMultiplier(float Multiplier)
{
	HealthMultiplier = Multiplier;
}