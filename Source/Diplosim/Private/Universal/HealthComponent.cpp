#include "Universal/HealthComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AI/Enemy.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Parliament.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Universal/Projectile.h"
#include "Universal/AttackComponent.h"
#include "Universal/DiplosimGameModeBase.h"

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

		FTimerStruct* foundTimer = Camera->TimerManager->FindTimer("RemoveDamageOverlay", GetOwner());

		if (foundTimer == nullptr)
			Camera->TimerManager->CreateTimer("RemoveDamageOverlay", GetOwner(), 0.25f, "RemoveDamageOverlay", {}, false, true);
		else
			Camera->TimerManager->ResetTimer("RemoveDamageOverlay", GetOwner());

		if (Health == 0)
			Death(Attacker);
		else if (GetOwner()->IsA<ACitizen>())
			Camera->DiseaseManager->Injure(Cast<ACitizen>(GetOwner()), Camera->Stream.RandRange(0, 100));

		Camera->PlayAmbientSound(HitAudioComponent, Sound);
	});
}

bool UHealthComponent::IsMaxHealth()
{
	return Health == MaxHealth;
}

int32 UHealthComponent::GetHealth()
{
	return Health;
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

		Camera->TimerManager->CreateTimer("Decay", GetOwner(), 6.0f, "Clear", {}, false, true);

		if (actor->IsA<ACitizen>()) {
			ACitizen* citizen = Cast<ACitizen>(actor);

			Camera->CitizenManager->ClearCitizen(citizen);

			citizen->MovementComponent->SetAnimation(EAnim::Death);

			if (faction->Name == Camera->ColonyName)
				Camera->NotifyLog("Bad", citizen->BioComponent->Name + " has died", faction->Name);
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

		building->ParticleComponent->Deactivate();

		if (building->IsA<AParliament>()) {
			faction->Politics.Representatives.Empty();

			Camera->TimerManager->RemoveTimer(faction->Name + " Election", Camera);
			Camera->TimerManager->RemoveTimer(faction->Name + " Bill", Camera);

			Camera->BribeUIInstance->RemoveFromParent();
			Camera->ParliamentUIInstance->RemoveFromParent();
		}

		if (Camera->InfoUIInstance->IsInViewport())
			Camera->UpdateBuildingInfoDisplay(building, false);
	}

	Camera->Grid->AtmosphereComponent->ClearFire(actor);

	if (DeathSystem != nullptr) {
		FVector origin;
		FVector extent;
		actor->GetActorBounds(false, origin, extent);

		FVector location = Camera->GetTargetActorLocation(actor);

		if (actor->IsA<AEnemy>())
			origin = location;
		else
			origin.Z = location.Z;

		UNiagaraComponent* deathComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathSystem, origin);

		if (actor->IsA<AEnemy>()) {
			actor->SetActorHiddenInGame(true);

			deathComp->SetColorParameter("Colour", Cast<AEnemy>(actor)->Colour);
		}
		else if (actor->IsA<ABuilding>() || actor->IsA<AAISpawner>()) {
			UStaticMeshComponent* mesh = actor->GetComponentByClass<UStaticMeshComponent>();
			FVector dimensions = mesh->GetStaticMesh()->GetBounds().GetBox().GetSize();

			deathComp->SetVariableVec2(TEXT("XY"), FVector2D(dimensions.X, dimensions.Y));
			deathComp->SetVariableFloat(TEXT("Radius"), dimensions.X / 2);

			Camera->Grid->AIVisualiser->DestructingActors.Add(actor, GetWorld()->GetTimeSeconds());
			UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, origin, 0.0f, 2000.0f, 10000000.0f / (dimensions.X * dimensions.Y * dimensions.Z));

			if (Camera->WidgetComponent->GetAttachParentActor() == actor)
				Camera->SetInteractStatus(Camera->WidgetComponent->GetAttachmentRootActor(), false);
		}
	}

	if (!IsValid(Attacker))
		return;

	Camera->TimerManager->RemoveAllTimers(actor, {"RemoveDamageOverlay"});

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(*faction, params);
	Camera->TimerManager->SetParameter(Attacker, params);
	Camera->TimerManager->CreateTimer("Clear Death", GetOwner(), 10.0f, "Clear", params, false, true);
}

void UHealthComponent::AIDecay()
{
	AAI* ai = Cast<AAI>(GetOwner());
	ai->MovementComponent->SetAnimation(EAnim::Decay, false, 1.0f);
}

void UHealthComponent::Clear(FFactionStruct Faction, AActor* Attacker)
{
	AActor* actor = GetOwner();

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	if (Camera->AttachedTo.Actor == actor)
		Camera->SetInteractStatus(Camera->WidgetComponent->GetAttachmentRootActor(), false);

	if (actor->IsA<ACitizen>()) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		citizen->BioComponent->RemovePartner();
		citizen->BioComponent->Disown();

		if (Attacker->IsA<AEnemy>() && gamemode->Enemies.Contains(Attacker))
			gamemode->WavesData.Last().NumKilled++;
		else {
			TMap<ACitizen*, int32> favouredChildren;
			int32 totalCount = 0;

			for (ACitizen* c : citizen->BioComponent->Children) {
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
	else if (actor->IsA<AEnemy>() && gamemode->Enemies.Contains(actor)) {
		if (Attacker->IsA<AProjectile>()) {
			AActor* a = nullptr;

			if (Attacker->GetOwner()->IsA<AWall>())
				a = Attacker->GetOwner();
			else
				a = Cast<ACitizen>(Attacker->GetOwner())->BuildingComponent->Employment;

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
	}
	else if (actor->IsA<AAISpawner>()) {
		FFactionStruct* f = Camera->ConquestManager->GetFaction("", Attacker);

		AAISpawner* spawner = Cast<AAISpawner>(actor);
		spawner->ClearedNest(f);

		gamemode->SnakeSpawners.RemoveSingle(spawner);

		actor->Destroy();
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
		else if (ai->IsA<AEnemy>()) {
			AEnemy* enemy = Cast<AEnemy>(ai);

			if (gamemode->Enemies.Contains(enemy))
				gamemode->Enemies.Remove(enemy);
			else
				gamemode->Snakes.Remove(enemy);
		}
		else
			faction->Clones.Remove(ai);

		ai->Destroy();
	}
}

void UHealthComponent::OnFire()
{
	TakeHealth(5, GetOwner());
}