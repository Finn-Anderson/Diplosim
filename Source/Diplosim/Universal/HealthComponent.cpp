#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/Projectile.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker)
{
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
	else {
		Cast<AAI>(GetOwner())->GetMesh()->SetOverlayMaterial(material);
	}

	FTimerHandle matTimer;
	GetWorld()->GetTimerManager().SetTimer(matTimer, FTimerDelegate::CreateUObject(this, &UHealthComponent::RemoveDamageOverlay), 0.15f, false);

	if (Health == 0)
		Death(Attacker);
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
		Cast<AAI>(GetOwner())->GetMesh()->SetOverlayMaterial(nullptr);
}

void UHealthComponent::Death(AActor* Attacker)
{
	AActor* actor = GetOwner();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACamera* camera = PController->GetPawn<ACamera>();

	if (actor->IsA<ABroch>()) {
		camera->Lose();
	}
	else if (actor->IsA<AAI>()) {
		USkeletalMeshComponent* mesh = actor->GetComponentByClass<USkeletalMeshComponent>();
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		attackComp->RangeComponent->SetGenerateOverlapEvents(false);
		Cast<AAI>(actor)->Reach->SetGenerateOverlapEvents(false);

		attackComp->ClearAttacks();

		mesh->SetSimulatePhysics(true);

		const FVector direction = (actor->GetActorLocation() - Attacker->GetActorLocation()).Rotation().Vector() * 50;
		mesh->SetPhysicsLinearVelocity(direction, false);

		Cast<AAI>(actor)->DetachFromControllerPendingDestroy();

		if (actor->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(actor);

			citizen->PopupComponent->SetHiddenInGame(true);

			camera->CitizenManager->Citizens.Remove(citizen);
		}
	} 
	else if (actor->IsA<ABuilding>()) {
		TArray<AActor*> foundCitizens;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACitizen::StaticClass(), foundCitizens);

		for (AActor* c : foundCitizens) {
			ACitizen* citizen = Cast<ACitizen>(c);

			if (citizen->Building.BuildingAt != actor)
				continue;

			citizen->HealthComponent->TakeHealth(100, Attacker);
		}

		ABuilding* building = Cast<ABuilding>(actor);

		building->BuildingMesh->SetGenerateOverlapEvents(false);

		UConstructionManager* cm = building->Camera->ConstructionManager;
		cm->RemoveBuilding(building);

		camera->CitizenManager->Buildings.Remove(building);
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