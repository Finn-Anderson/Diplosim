#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/Projectile.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(0.01f);

	RebuildLocation = FVector::Zero();
	CrumbleLocation = FVector::Zero();

	HealthMultiplier = 1.0f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

void UHealthComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime < 0.009f || DeltaTime > 1.0f)
		return;

	if (GetOwner()->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(GetOwner());

		float height = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;

		FVector location = FMath::VInterpTo(building->GetActorLocation(), building->GetActorLocation() - FVector(0.0f, 0.0f, height + 1.0f), DeltaTime, 0.15f);

		FVector difference = building->GetActorLocation() - location;

		building->SetActorLocation(location);

		building->DestructionComponent->SetRelativeLocation(building->DestructionComponent->GetRelativeLocation() + difference);
		building->GroundDecalComponent->SetRelativeLocation(building->GroundDecalComponent->GetRelativeLocation() + difference);

		if (CrumbleLocation.Z <= building->GetActorLocation().Z) {
			UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, building->GetActorLocation(), 0.0f, 2000.0f, 1.0f);
		}
		else {
			building->DestructionComponent->Deactivate();

			SetComponentTickEnabled(false);
		}
	}
	else {
		AAI* ai = Cast<AAI>(GetOwner());

		FRotator rotation = ai->Mesh->GetRelativeRotation();
		rotation.Pitch++;

		ai->Mesh->SetRelativeRotation(rotation);

		if (rotation.Pitch >= 90.0f)
			SetComponentTickEnabled(false);
	}
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker)
{
	AsyncTask(ENamedThreads::GameThread, [this, Amount, Attacker]() {
		if (GetHealth() == 0)
			return;

		int32 force = FMath::Max(FMath::Abs(Health - Amount), 1);

		Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

		float opacity = (MaxHealth - Health) / (float)MaxHealth;

		if (GetOwner()->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(GetOwner());

			Camera->ConstructionManager->AddBuilding(building, EBuildStatus::Damaged);

			building->BuildingMesh->SetOverlayMaterial(OnHitEffect);
			building->BuildingMesh->SetCustomPrimitiveDataFloat(6, opacity);
		}
		else {
			Cast<AAI>(GetOwner())->Mesh->SetOverlayMaterial(OnHitEffect);
			Cast<AAI>(GetOwner())->Mesh->SetCustomPrimitiveDataFloat(8, opacity);
		}

		FTimerStruct* foundTimer = Camera->CitizenManager->FindTimer("RemoveDamageOverlay", GetOwner());

		if (foundTimer == nullptr)
			Camera->CitizenManager->CreateTimer("RemoveDamageOverlay", GetOwner(), 0.15f, FTimerDelegate::CreateUObject(this, &UHealthComponent::RemoveDamageOverlay), false, true);
		else
			Camera->CitizenManager->ResetTimer("RemoveDamageOverlay", GetOwner());

		if (Health == 0)
			Death(Attacker, force);
		else if (Camera->CitizenManager->Citizens.Contains(GetOwner()))
			Camera->CitizenManager->Injure(Cast<ACitizen>(GetOwner()), Camera->Grid->Stream.RandRange(0, 100));
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

	if (actor->IsA<ABroch>())
		Camera->Lose();

	if (actor->IsA<AAI>()) {
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		attackComp->ClearAttacks();

		Cast<AAI>(actor)->AIController->StopMovement();
		Cast<AAI>(actor)->DetachFromControllerPendingDestroy();

		Camera->CitizenManager->AIPendingRemoval.Add(Cast<AAI>(actor));

		if (actor->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(actor);

			citizen->HungerMesh->SetHiddenInGame(true);
			citizen->IllnessMesh->SetHiddenInGame(true);

			//FWorldTileStruct* tile = Camera->ConquestManager->GetColonyContainingCitizen(citizen);

			Camera->CitizenManager->ClearCitizen(citizen);

			//Camera->NotifyLog("Bad", citizen->BioStruct.Name + " has died", tile->Name);
		}
	} 
	else if (actor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(actor);

		for (ACitizen* citizen : building->GetOccupied())
			building->RemoveCitizen(citizen);

		for (AActor* citizen : building->Inside)
			Cast<ACitizen>(citizen)->HealthComponent->TakeHealth(1000, Attacker);

		building->BuildingMesh->SetGenerateOverlapEvents(false);

		building->Storage.Empty();

		Camera->CitizenManager->Buildings.Remove(building);
		Camera->ConstructionManager->RemoveBuilding(building);

		FVector dimensions = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		RebuildLocation = building->GetActorLocation();

		CrumbleLocation = RebuildLocation - FVector(0.0f, 0.0f, dimensions.Z);

		building->DestructionComponent->SetVariableVec2(TEXT("XY"), FVector2D(dimensions.X, dimensions.Y));
		building->DestructionComponent->SetVariableFloat(TEXT("Radius"), dimensions.X / 2);

		building->DestructionComponent->Activate();
	}

	SetComponentTickEnabled(true);

	Camera->CitizenManager->CreateTimer("Clear Death", GetOwner(), 10.0f, FTimerDelegate::CreateUObject(this, &UHealthComponent::Clear, Attacker), false, true);
}

void UHealthComponent::Clear(AActor* Attacker)
{
	AActor* actor = GetOwner();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if (actor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->BioStruct.Mother->IsValidLowLevelFast())
			citizen->BioStruct.Mother->BioStruct.Children.Remove(citizen);

		if (citizen->BioStruct.Father->IsValidLowLevelFast())
			citizen->BioStruct.Father->BioStruct.Children.Remove(citizen);

		for (ACitizen* sibling : citizen->BioStruct.Siblings)
			sibling->BioStruct.Siblings.Remove(citizen);

		if (Attacker->IsA<AEnemy>())
			gamemode->WavesData.Last().NumKilled++;
		else {
			TMap<ACitizen*, int32> favouredChildren;
			int32 totalCount = 0;

			for (ACitizen* c : citizen->BioStruct.Children) {
				int32 count = 0;

				for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen)) {
					for (FPersonality* p : Camera->CitizenManager->GetCitizensPersonalities(c)) {
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

				Camera->ResourceManager->AddUniversalResource(Camera->ResourceManager->Money, remainder);
			}
			else {
				Camera->ResourceManager->AddUniversalResource(Camera->ResourceManager->Money, citizen->Balance);
			}
		}
	}
	else if (actor->IsA<AEnemy>()) {
		if (Attacker->IsA<AProjectile>()) {
			AActor* a = nullptr;

			if (Attacker->GetOwner()->IsA<AWall>())
				a = Attacker->GetOwner();
			else
				a = Cast<ACitizen>(Attacker->GetOwner())->Building.Employment;

			gamemode->WavesData.Last().SetDiedTo(a);
		}
		else
			gamemode->WavesData.Last().SetDiedTo(Attacker);

		if (gamemode->CheckEnemiesStatus())
			gamemode->SetWaveTimer();
	}
	else if (actor->IsA<ABuilding>()) {
		Cast<ABuilding>(actor)->GroundDecalComponent->SetHiddenInGame(false);

		Cast<ABuilding>(actor)->DestructionComponent->Deactivate();

		SetComponentTickEnabled(false);

		return;
	}

	actor->Destroy();
}

int32 UHealthComponent::GetHealth()
{
	return Health;
}

void UHealthComponent::OnFire(int32 Counter)
{
	int32 amount = 50;

	if (Counter > 0)
		amount = 10;

	TakeHealth(amount, GetOwner());

	if (Counter < 5)
		Camera->CitizenManager->CreateTimer("OnFire", GetOwner(), 1.0f, FTimerDelegate::CreateUObject(this, &UHealthComponent::OnFire, Counter++), false);
}