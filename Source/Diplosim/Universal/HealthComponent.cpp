#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
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
#include "Player/Managers/ResourceManager.h"

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

	if (DeltaTime < 0.0001f)
		return;

	ABuilding* building = Cast<ABuilding>(GetOwner());

	float height = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

	FVector location = FMath::VInterpTo(building->GetActorLocation(), building->GetActorLocation() - FVector(0.0f, 0.0f, height + 1.0f), DeltaTime, 0.15f);

	FVector difference = building->GetActorLocation() - location;

	building->SetActorLocation(location);

	building->DestructionComponent->SetRelativeLocation(building->DestructionComponent->GetRelativeLocation() + difference);
	building->GroundDecalComponent->SetRelativeLocation(building->GroundDecalComponent->GetRelativeLocation() + difference);

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
		int32 force = FMath::Max(FMath::Abs(Health - Amount), 1);

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
			Death(Attacker, force);
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

void UHealthComponent::Death(AActor* Attacker, int32 Force)
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

		const FVector direction = (actor->GetActorLocation() - Attacker->GetActorLocation()).Rotation().Vector() * 50 * Force;
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

		RebuildLocation = building->GetActorLocation();

		CrumbleLocation = RebuildLocation - FVector(0.0f, 0.0f, dimensions.Z);

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

		if (citizen->BioStruct.Mother->IsValidLowLevelFast())
			citizen->BioStruct.Mother->BioStruct.Children.Remove(citizen);

		if (citizen->BioStruct.Father->IsValidLowLevelFast())
			citizen->BioStruct.Father->BioStruct.Children.Remove(citizen);

		if (Attacker->IsA<AEnemy>())
			gamemode->WavesData.Last().NumKilled++;
		else {
			TMap<ACitizen*, int32> favouredChildren;
			int32 totalCount = 0;

			APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			ACamera* camera = PController->GetPawn<ACamera>();

			for (ACitizen* c : citizen->BioStruct.Children) {
				int32 count = 0;

				for (FPersonality* personality : camera->CitizenManager->GetCitizensPersonalities(citizen)) {
					for (FPersonality* p : camera->CitizenManager->GetCitizensPersonalities(c)) {
						if (personality->Trait == p->Trait)
							count += 2;
						else if (personality->Likes.Contains(p->Trait))
							count++;
						else if (personality->Dislikes.Contains(p->Trait))
							count--;
					}
				}

				if (count < 0)
					continue;

				favouredChildren.Emplace(c, count);

				totalCount += count;
			}

			if (totalCount > 0) {
				int32 remainder = citizen->Balance % totalCount;

				for (auto& element : favouredChildren)
					element.Key->Balance += (citizen->Balance / totalCount * element.Value);

				camera->ResourceManager->AddUniversalResource(camera->ResourceManager->Money, remainder);
			}
			else {
				camera->ResourceManager->AddUniversalResource(camera->ResourceManager->Money, citizen->Balance);
			}
		}
	}
	else if (actor->IsA<AEnemy>()) {
		if (Attacker->IsA<AProjectile>())
			gamemode->WavesData.Last().SetDiedTo(Attacker->GetOwner());
		else
			gamemode->WavesData.Last().SetDiedTo(Attacker);

		if (gamemode->CheckEnemiesStatus())
			gamemode->SetWaveTimer();
	}
	else if (actor->IsA<ABuilding>()) {
		Cast<ABuilding>(actor)->GroundDecalComponent->SetHiddenInGame(false);

		Cast<ABuilding>(actor)->DestructionComponent->Deactivate();

		return;
	}

	actor->Destroy();
}

int32 UHealthComponent::GetHealth()
{
	return Health;
}