#include "HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AI/Citizen.h"
#include "AI/Enemy.h"
#include "AI/Projectile.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Parliament.h"
#include "DiplosimGameModeBase.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false; 
	
	HitAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HitAudioComponent"));
	HitAudioComponent->SetVolumeMultiplier(0.0f);
	HitAudioComponent->bCanPlayMultipleInstances = true;

	HealthMultiplier = 1.0f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

void UHealthComponent::AddHealth(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
}

void UHealthComponent::TakeHealth(int32 Amount, AActor* Attacker, USoundBase* Sound)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Amount, Attacker, Sound]() {
		if (GetHealth() == 0)
			return;

		Health = FMath::Clamp(Health - Amount, 0, MaxHealth);

		ApplyDamageOverlay();

		FTimerStruct* foundTimer = Camera->CitizenManager->FindTimer("RemoveDamageOverlay", GetOwner());

		if (foundTimer == nullptr)
			Camera->CitizenManager->CreateTimer("RemoveDamageOverlay", GetOwner(), 0.25f, "RemoveDamageOverlay", {}, false, true);
		else
			Camera->CitizenManager->ResetTimer("RemoveDamageOverlay", GetOwner());

		if (Health == 0)
			Death(Attacker);
		else if (GetOwner()->IsA<ACitizen>())
			Camera->CitizenManager->Injure(Cast<ACitizen>(GetOwner()), Camera->Grid->Stream.RandRange(0, 100));

		Camera->PlayAmbientSound(HitAudioComponent, Sound);
	});
}

bool UHealthComponent::IsMaxHealth()
{
	if (Health != MaxHealth)
		return false;

	return true;
}

void UHealthComponent::ApplyDamageOverlay(bool bLoad)
{
	float opacity = (MaxHealth - Health) / (float)MaxHealth;

	if (GetOwner()->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(GetOwner());

		if (!bLoad)
			Camera->ConstructionManager->AddBuilding(building, EBuildStatus::Damaged);

		building->BuildingMesh->SetOverlayMaterial(OnHitEffect);
		building->BuildingMesh->SetCustomPrimitiveDataFloat(6, opacity);
	}
	else {
		TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(GetOwner()));
		info.Key->SetCustomDataValue(info.Value, 9, opacity);
	}
}

void UHealthComponent::RemoveDamageOverlay()
{
	if (GetOwner()->IsA<ABuilding>()) {
		Cast<ABuilding>(GetOwner())->BuildingMesh->SetOverlayMaterial(nullptr);
		Cast<ABuilding>(GetOwner())->BuildingMesh->SetCustomPrimitiveDataFloat(6, 0.0f);
	}
	else {
		TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = Camera->Grid->AIVisualiser->GetAIHISM(Cast<AAI>(GetOwner()));
		info.Key->SetCustomDataValue(info.Value, 9, 0.0f);
	}
}

void UHealthComponent::Death(AActor* Attacker)
{
	AActor* actor = GetOwner();

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", actor);

	if (actor->IsA<ABroch>()) {
		if (faction->Name == Camera->ColonyName)
			Camera->Lose();

		for (ACitizen* citizen : faction->Citizens)
			citizen->HealthComponent->TakeHealth(1000, Camera);

		for (ABuilding* building : faction->Buildings)
			building->HealthComponent->TakeHealth(1000, Camera);
	}

	if (actor->IsA<AAI>()) {
		UAttackComponent* attackComp = actor->GetComponentByClass<UAttackComponent>();

		attackComp->ClearAttacks();

		Cast<AAI>(actor)->AIController->StopMovement();
		Camera->CitizenManager->RemoveTimer("Idle", actor);

		if (actor->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(actor);

			Camera->CitizenManager->ClearCitizen(citizen);

			citizen->MovementComponent->SetAnimation(EAnim::Death);

			if (IsValid(Attacker))
				Camera->NotifyLog("Bad", citizen->BioStruct.Name + " has died", Camera->ConquestManager->GetCitizenFaction(citizen).Name);
		}
	} 
	else if (actor->IsA<ABuilding>()) {
		ABuilding* building = Cast<ABuilding>(actor);

		for (AActor* citizen : building->Inside)
			Cast<ACitizen>(citizen)->HealthComponent->TakeHealth(1000, Attacker);

		for (ACitizen* citizen : building->GetOccupied())
			building->RemoveCitizen(citizen);

		building->BuildingMesh->SetGenerateOverlapEvents(false);

		building->Storage.Empty();

		faction->Buildings.Remove(building);
		faction->RuinedBuildings.Add(building);

		Camera->ConstructionManager->RemoveBuilding(building);

		FVector dimensions = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

		building->DestructionComponent->SetVariableVec2(TEXT("XY"), FVector2D(dimensions.X, dimensions.Y));
		building->DestructionComponent->SetVariableFloat(TEXT("Radius"), dimensions.X / 2);

		building->ParticleComponent->Deactivate();
		building->DestructionComponent->Activate();

		UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, building->GetActorLocation(), 0.0f, 2000.0f, 1.0f);

		if (IsValid(Attacker))
			building->DeathTime = GetWorld()->GetTimeSeconds();

		Camera->Grid->AIVisualiser->DestructingBuildings.Add(building);

		if (building->IsA<AParliament>()) {
			faction->Politics.Representatives.Empty();

			Camera->CitizenManager->RemoveTimer(faction->Name + " Election", Camera);
			Camera->CitizenManager->RemoveTimer(faction->Name + " Bill", Camera);

			Camera->BribeUIInstance->RemoveFromParent();
			Camera->ParliamentUIInstance->RemoveFromParent();
		}
	}

	if (DeathSystem != nullptr) {
		actor->SetActorHiddenInGame(true);

		FVector origin;
		FVector extent;
		actor->GetActorBounds(false, origin, extent);

		origin = Camera->GetTargetActorLocation(actor);

		UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathSystem, origin + (extent / 2));

		if (actor->IsA<AEnemy>())
			deathComp->SetColorParameter("Colour", Cast<AEnemy>(actor)->Colour);
	}

	if (!IsValid(Attacker))
		return;

	TArray<FTimerParameterStruct> params;
	Camera->CitizenManager->SetParameter(*faction, params);
	Camera->CitizenManager->SetParameter(Attacker, params);
	Camera->CitizenManager->CreateTimer("Clear Death", GetOwner(), 10.0f, "Clear", params, false, true);
}

void UHealthComponent::Clear(FFactionStruct Faction, AActor* Attacker)
{
	AActor* actor = GetOwner();

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if (actor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->BioStruct.Mother != nullptr)
			citizen->BioStruct.Mother->BioStruct.Children.Remove(citizen);

		if (citizen->BioStruct.Father != nullptr)
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

				Camera->ResourceManager->AddUniversalResource(faction, Camera->ResourceManager->Money, remainder);
			}
			else {
				Camera->ResourceManager->AddUniversalResource(faction, Camera->ResourceManager->Money, citizen->Balance);
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
		ABuilding* building = Cast<ABuilding>(actor);

		building->BuildingMesh->SetCanEverAffectNavigation(false);

		building->GroundDecalComponent->SetHiddenInGame(false);

		building->DestructionComponent->Deactivate();
	}

	if (actor->IsA<AAI>()) {
		AAI* ai = Cast<AAI>(actor);

		TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> info = Camera->Grid->AIVisualiser->GetAIHISM(ai);

		Camera->Grid->AIVisualiser->RemoveInstance(info.Key, info.Value);

		if (ai->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(ai);

			if (faction->Citizens.Contains(citizen))
				faction->Citizens.Remove(citizen);
			else
				faction->Rebels.Remove(citizen);
		}
		else if (ai->IsA<AEnemy>())
			Camera->CitizenManager->Enemies.Remove(ai);
		else
			faction->Clones.Remove(ai);

		ai->Destroy();
	}
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

	if (Counter < 5) {
		TArray<FTimerParameterStruct> params;
		Camera->CitizenManager->SetParameter(Counter++, params);
		Camera->CitizenManager->CreateTimer("OnFire", GetOwner(), 1.0f, "OnFire", params, false);
	}
}