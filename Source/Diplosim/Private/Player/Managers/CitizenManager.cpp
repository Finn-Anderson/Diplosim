#include "Player/Managers/CitizenManager.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Misc/ScopeTryLock.h"
#include "Blueprint/UserWidget.h"

#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/House.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Work/Booster.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/EventsManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Components/SaveGameComponent.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"

UCitizenManager::UCitizenManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	IssuePensionHour = 18;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Personalities.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Religions.json");
}

void UCitizenManager::ReadJSONFile(FString path)
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	FString fileContents;
	FFileHelper::LoadFileToString(fileContents, *path);
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

	if (!FJsonSerializer::Deserialize(jsonReader, jsonObject) || !jsonObject.IsValid())
		return;

	for (auto& element : jsonObject->Values) {
		for (auto& e : element.Value->AsArray()) {
			FPersonality personality;
			FReligionStruct religion;

			for (auto& v : e->AsObject()->Values) {
				uint8 index = 0;

				if (v.Value->Type == EJson::Array) {
					for (auto& ev : v.Value->AsArray()) {
						if (element.Key == "Personalities") {
							if (v.Key == "Likes")
								personality.Likes.Add(ev->AsString());
							else if (v.Key == "Dislikes")
								personality.Dislikes.Add(ev->AsString());
							else {
								FString key = "";
								float value = 1.0f;

								for (auto& bv : ev->AsObject()->Values) {
									if (bv.Key == "Affect")
										key = bv.Value->AsString();
									else
										value = bv.Value->AsNumber();
								}

								personality.Affects.Add(key, value);
							}
						}
						else {
							religion.Agreeable.Add(ev->AsString());
						}
					}
				}
				else if (v.Value->Type == EJson::String) {
					FString value = v.Value->AsString();

					if (element.Key == "Personalities")
						personality.Trait = value;
					else if (element.Key == "Religions")
						religion.Faith = value;
				}
				else {
					float value = v.Value->AsNumber();

					if (v.Key == "Aggressiveness")
						personality.Aggressiveness = value;
					else
						personality.Morale = value;
				}
			}

			if (element.Key == "Personalities")
				Personalities.Add(personality);
			else if (element.Key == "Religions")
				Religions.Add(religion);
		}
	}
}

void UCitizenManager::CitizenGeneralLoop()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&CitizenGeneralLoopLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::CitizenLoop);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			if (faction.Citizens.IsEmpty())
				continue;

			Camera->PoliticsManager->PoliticsLoop(&faction);

			int32 rebelCount = 0;
			int32 happinessCount = 0;

			int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

			for (ACitizen* citizen : faction.Citizens) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() == 0)
					continue;

				citizen->BuildingComponent->AllocatedBuildings.Empty();
				citizen->BuildingComponent->AllocatedBuildings = { citizen->BuildingComponent->School, citizen->BuildingComponent->Employment, citizen->BuildingComponent->House };

				TArray<ACitizen*> roommates = citizen->BuildingComponent->GetRoomates();

				if (citizen->BuildingComponent->CanFindAnything(timeToCompleteDay, &faction)) {
					for (ABuilding* building : faction.Buildings) {
						if (Camera->SaveGameComponent->IsLoading())
							return;

						if (!IsValid(building) || building->HealthComponent->GetHealth() == 0 || citizen->BuildingComponent->AllocatedBuildings.Contains(building) || (!building->IsA<AWork>() && !building->IsA<AHouse>()) || !citizen->AIController->CanMoveTo(building->GetActorLocation()))
							continue;

						if (building->IsA<ASchool>())
							citizen->BuildingComponent->FindEducation(Cast<ASchool>(building), timeToCompleteDay);
						else if (building->IsA<AWork>())
							citizen->BuildingComponent->FindJob(Cast<AWork>(building), timeToCompleteDay);
						else if (building->IsA<AHouse>())
							citizen->BuildingComponent->FindHouse(Cast<AHouse>(building), timeToCompleteDay, roommates);
					}

					citizen->BuildingComponent->SetJobHouseEducation(timeToCompleteDay, roommates);
				}

				citizen->HappinessComponent->SetHappiness();

				int32 happiness = citizen->HappinessComponent->GetHappiness();
				Camera->Grid->AIVisualiser->SetEyesVisuals(citizen, happiness);

				happinessCount += happiness;

				if (Camera->PoliticsManager->GetMembersParty(citizen) != nullptr && Camera->PoliticsManager->GetMembersParty(citizen)->Party == "Shell Breakers")
					rebelCount++;

				if (!IsValid(citizen->BuildingComponent->Employment))
					continue;

				if (citizen->BuildingComponent->Employment->IsA<AWall>())
					Cast<AWall>(citizen->BuildingComponent->Employment)->SetEmergency(!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty() || !faction.Rebels.IsEmpty());
				else if (citizen->BuildingComponent->Employment->IsA<AClinic>())
					Cast<AClinic>(citizen->BuildingComponent->Employment)->SetEmergency(!Camera->DiseaseManager->Infected.IsEmpty());
			}

			float rebelsPerc = rebelCount / (float)faction.Citizens.Num();

			Camera->PoliceManager->RespondToReports(&faction);

			if (rebelsPerc > 0.33f)
				Camera->PoliticsManager->ChooseRebellionType(&faction);

			if (faction.Name == Camera->ColonyName) {
				float happinessPerc = happinessCount / (faction.Citizens.Num() * 100.0f);

				Async(EAsyncExecution::TaskGraphMainTick, [this, happinessPerc, rebelsPerc]() { Camera->UpdateUI(happinessPerc, rebelsPerc); });
			}
		}
	});
}

void UCitizenManager::CalculateGoalInteractions()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&GoalInteractionsLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Interactions);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			for (ACitizen* citizen : citizens) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				if (!IsValid(citizen->AIController->MoveRequest.GetGoalActor()) || citizen->AIController->MoveRequest.GetGoalActor()->IsA<AAI>())
					continue;

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bBuildings = true;
				requestedOverlaps.bResources = true;

				float reach = citizen->Range / 15.0f;
				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);

				for (AActor* actor : actors) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					if (citizen->AIController->MoveRequest.GetGoalActor() != actor || !citizen->CanReach(actor, reach, citizen->AIController->MoveRequest.GetGoalInstance()))
						continue;

					Async(EAsyncExecution::TaskGraphMainTick, [this, citizen, actor]() {
						if (actor->IsA<AResource>() && Camera->TimerManager->FindTimer("Harvest", citizen) == nullptr) {
							AResource* r = Cast<AResource>(actor);

							citizen->StartHarvestTimer(r);
						}
						else if (actor->IsA<ABuilding>()) {
							ABuilding* b = Cast<ABuilding>(actor);

							b->Enter(citizen);
						}

						citizen->AIController->StopMovement();
					});
				}
			}
		}
	});
}

void UCitizenManager::CalculateConversationInteractions()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&ConversationInteractionsLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Conversations);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty() || !faction.Rebels.IsEmpty())
				continue;

			for (ACitizen* citizen : citizens) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				bool bCitizenInReport = Camera->PoliceManager->IsInAPoliceReport(citizen, &faction);

				if (citizen->bConversing || citizen->bSleep || (bCitizenInReport && (!IsValid(citizen->BuildingComponent->Employment) || !citizen->BuildingComponent->Employment->IsA(Camera->PoliceManager->PoliceStationClass))))
					continue;

				float reach = citizen->Range / 15.0f;
				
				if (IsValid(citizen->AIController->MoveRequest.GetGoalActor()) && IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->IsA(Camera->PoliceManager->PoliceStationClass)) {
					Camera->PoliceManager->PoliceInteraction(&faction, citizen, reach);
				}
				else if (Camera->Stream.RandRange(0, 1000) == 1000) {
					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.bCitizens = true;

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);
					TArray<ACitizen*> citizensToTalkTo;

					for (AActor* actor : actors) {
						if (Camera->SaveGameComponent->IsLoading())
							return;

						ACitizen* c = Cast<ACitizen>(actor);

						if (!c->bConversing && !c->bSleep && c->AttackComponent->OverlappingEnemies.IsEmpty() && !bCitizenInReport && !Camera->PoliceManager->IsInAPoliceReport(c, &faction))
							continue;

						citizensToTalkTo.Add(c);
					}

					if (citizensToTalkTo.IsEmpty())
						continue;

					int32 index = Camera->Stream.RandRange(0, citizensToTalkTo.Num() - 1);

					StartConversation(&faction, citizen, citizensToTalkTo[index], false);
				}
			}
		}
	});
}

//
// House
//
void UCitizenManager::UpdateAllTypeAmount(FString FactionName, TSubclassOf<class ABuilding> BuildingType, float NewAmount)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA(BuildingType))
			continue;

		for (FCapacityStruct& capacityStruct : Cast<AHouse>(building)->Occupied)
			capacityStruct.Amount = NewAmount;
	}
}

//
// Death
//
void UCitizenManager::ClearCitizen(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPartyStruct& party : faction->Politics.Parties) {
		if (!party.Members.Contains(Citizen))
			continue;

		if (party.Leader == Citizen)
			Camera->PoliticsManager->SelectNewLeader(&party);

		party.Members.Remove(Citizen);

		if (Camera->InfoUIInstance->IsInViewport())
			Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Party, party.Party);

		break;
	}

	for (ACitizen* citizen : Citizen->BioComponent->GetLikedFamily(false)) {
		int32 value = -12;

		if (citizen->BioComponent->Partner == Citizen)
			value = -20;

		citizen->HappinessComponent->SetDecayingHappiness(&citizen->HappinessComponent->FamilyDeathHappiness, value);
	}

	FOverlapsStruct requestedOverlaps;
	requestedOverlaps.GetCitizenInteractions(false, false);

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, Citizen, Citizen->Range, requestedOverlaps, EFactionType::Same);

	for (AActor* actor : actors) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden())
			continue;

		bool bIsCruel = false;

		for (FPersonality* personality : GetCitizensPersonalities(citizen))
			if (personality->Trait == "Cruel")
				bIsCruel = true;

		int32 happinessValue = -6;

		if (bIsCruel)
			happinessValue = 6;

		citizen->HappinessComponent->SetDecayingHappiness(&citizen->HappinessComponent->WitnessedDeathHappiness, happinessValue);
	}

	for (FPersonality* personality : GetCitizensPersonalities(Citizen)) {
		if (!personality->Citizens.Contains(Citizen))
			continue;

		personality->Citizens.Remove(Citizen);

		if (Camera->InfoUIInstance->IsInViewport())
			Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Personality, personality->Trait);
	}

	if (IsValid(Citizen->BuildingComponent->Employment))
		Citizen->BuildingComponent->Employment->RemoveCitizen(Citizen);

	if (IsValid(Citizen->BuildingComponent->House))
		Citizen->BuildingComponent->RemoveCitizenFromHouse(Citizen);

	Camera->ArmyManager->RemoveFromArmy(Citizen);

	if (Camera->InfoUIInstance->IsInViewport())
		Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Religion, Citizen->Spirituality.Faith);
}

//
// Work
//
void UCitizenManager::CheckWorkStatus(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			for (auto element : citizen->BuildingComponent->HoursWorked) {
				if (!IsValid(citizen->BuildingComponent->Employment) || (citizen->BuildingComponent->HoursWorked.Contains(Hour) && !citizen->BuildingComponent->Employment->IsWorking(citizen, Hour)))
					citizen->BuildingComponent->HoursWorked.Remove(Hour);
				else if (!citizen->BuildingComponent->HoursWorked.Contains(Hour))
					citizen->BuildingComponent->HoursWorked.Add(Hour, citizen->BuildingComponent->Employment->GetAmount(citizen));
			}
		}

		for (ABuilding* building : faction.Buildings) {
			if (!IsValid(building) || !building->IsA<AWork>())
				continue;

			Cast<AWork>(building)->CheckWorkStatus(Hour);
		}
	}
}

void UCitizenManager::AISetupRadioTowerBroadcasts(FFactionStruct* Faction)
{
	ACitizen* citizen = Faction->Citizens[Camera->Stream.RandRange(0, Faction->Citizens.Num() - 1)];
	bool bParty = Camera->Stream.RandRange(0, 1) == 1 ? true : false;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA(RadioTowerClass))
			continue;

		ABooster* radioTower = Cast<ABooster>(building);

		if (radioTower->AISetTypeCooldown > 0) {
			radioTower->AISetTypeCooldown--;

			continue;
		}

		if (!building->GetOccupied().IsEmpty())
			continue;

		FString type = bParty ? Camera->PoliticsManager->GetCitizenParty(citizen) : citizen->Spirituality.Faith;
		radioTower->SetBroadcastType(type);

		radioTower->AISetTypeCooldown = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();
	}
}

//
// Citizen
//
void UCitizenManager::CheckUpkeepCosts()
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			float amount = 0.0f;

			for (auto element : citizen->BuildingComponent->HoursWorked)
				amount += element.Value;

			citizen->Balance += FMath::RoundHalfFromZero(amount);
			Camera->ResourceManager->TakeUniversalResource(&faction, Camera->ResourceManager->Money, amount, -100000);

			if (IsValid(citizen->BuildingComponent->House))
				citizen->BuildingComponent->House->CollectRent(&faction, citizen);
		}
	}
}

void UCitizenManager::CheckCitizenStatus(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (ACitizen* citizen : faction.Citizens) {
			if (citizen->HoursSleptToday.Contains(Hour))
				citizen->HoursSleptToday.Remove(Hour);

			if (citizen->bSleep)
				citizen->HoursSleptToday.Add(Hour);

			if (citizen->HoursSleptToday.Num() < citizen->IdealHoursSlept && !citizen->bSleep && (!IsValid(citizen->BuildingComponent->Employment) || !citizen->BuildingComponent->Employment->IsWorking(citizen)) && IsValid(citizen->BuildingComponent->House) && citizen->BuildingComponent->BuildingAt == citizen->BuildingComponent->House) {
				citizen->bSleep = true;

				citizen->Snore(true);
			}
			else if (citizen->bSleep) {
				citizen->bSleep = false;

				FTimerStruct* timer = Camera->TimerManager->FindTimer("Snore", citizen);

				if (timer != nullptr)
					timer->Actor = nullptr;
			}

			citizen->HappinessComponent->DecayHappiness();
			citizen->BioComponent->IncrementHoursTogetherWithPartner();
		}
	}

	Camera->EventsManager->CreateWedding(Hour);
}

void UCitizenManager::IssuePensions(int32 Hour)
{
	if (Hour != IssuePensionHour)
		return;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		int32 value = Camera->PoliticsManager->GetLawValue(faction.Name, "Pension");

		if (value == 0)
			continue;

		for (ACitizen* citizen : faction.Citizens)
			if (citizen->BioComponent->Age >= Camera->PoliticsManager->GetLawValue(faction.Name, "Pension Age"))
				citizen->Balance += value;
	}
}

//
// Conversations
//
USoundBase* UCitizenManager::GetConversationSound(ACitizen* Citizen)
{
	Citizen->bConversing = true;
	Citizen->AIController->StopMovement();

	int32 index = Camera->Stream.RandRange(0, Citizen->NormalConversations.Num() - 1);
	USoundBase* convo = Citizen->NormalConversations[index];

	for (FPersonality* personality : GetCitizensPersonalities(Citizen)) {
		if (personality->Trait != "Inept")
			continue;

		index = Camera->Stream.RandRange(0, Citizen->IneptIdiotConversations.Num() - 1);

		convo = Citizen->IneptIdiotConversations[index];

		break;
	}

	return convo;
}

void UCitizenManager::StartConversation(FFactionStruct* Faction, ACitizen* Citizen1, ACitizen* Citizen2, bool bInterrogation)
{
	Citizen1->MovementComponent->ActorToLookAt = Citizen2;
	Citizen2->MovementComponent->ActorToLookAt = Citizen1;

	USoundBase* convo1 = GetConversationSound(Citizen1);
	USoundBase* convo2 = GetConversationSound(Citizen2);
	
	Async(EAsyncExecution::TaskGraphMainTick, [this, Faction, Citizen1, Citizen2, bInterrogation, convo1, convo2]() {
		TArray<FTimerParameterStruct> params;
		Camera->TimerManager->SetParameter(*Faction, params);
		Camera->TimerManager->SetParameter(Citizen1, params);
		Camera->TimerManager->SetParameter(Citizen2, params);

		if (bInterrogation)
			Camera->TimerManager->CreateTimer("Interrogate", Citizen1, 6.0f, "InterrogateWitnesses", params, false);
		else
			Camera->TimerManager->CreateTimer("Interact", Citizen1, 6.0f, "Interact", params, false);

		Camera->PlayAmbientSound(Citizen1->AmbientAudioComponent, convo1, Citizen1->VoicePitch);
		Camera->PlayAmbientSound(Citizen2->AmbientAudioComponent, convo2, Citizen2->VoicePitch);
	});
}

void UCitizenManager::Interact(FFactionStruct Faction, ACitizen* Citizen1, ACitizen* Citizen2)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	Citizen1->bConversing = false;
	Citizen2->bConversing = false;

	int32 count = 0;

	float citizen1Aggressiveness = 0;
	float citizen2Aggressiveness = 0;

	PersonalityComparison(Citizen1, Citizen2, count, citizen1Aggressiveness, citizen2Aggressiveness);

	int32 chance = Camera->Stream.RandRange(0, 100);
	int32 positiveConversationLikelihood = 50 + count * 25;

	int32 happinessValue = 12;

	if (chance > positiveConversationLikelihood) {
		happinessValue = -12;

		Camera->PoliceManager->CalculateIfFight(&Faction, Citizen1, Citizen2, citizen1Aggressiveness, citizen2Aggressiveness);
	}

	Citizen1->HappinessComponent->SetDecayingHappiness(&Citizen1->HappinessComponent->ConversationHappiness, happinessValue);
	Citizen2->HappinessComponent->SetDecayingHappiness(&Citizen2->HappinessComponent->ConversationHappiness, happinessValue);

	if (!Citizen1->AttackComponent->IsComponentTickEnabled()) {
		Citizen1->AIController->DefaultAction();
		Citizen2->AIController->DefaultAction();
	}
}

//
// Genetics
//
void UCitizenManager::Pray(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	bool bPass = Camera->ResourceManager->TakeUniversalResource(faction, Camera->ResourceManager->Money, GetPrayCost(FactionName), 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	IncrementPray(*faction, "Good", 1);
}

int32 UCitizenManager::GetPrayCost(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 cost = 50;

	for (int32 i = 0; i < faction->PrayStruct.Good; i++)
		cost *= 1.15;

	return cost;
}

void UCitizenManager::Sacrifice(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (faction->Citizens.IsEmpty()) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	int32 index = Camera->Stream.RandRange(0, faction->Citizens.Num() - 1);
	ACitizen* citizen = faction->Citizens[index];

	Camera->TimerManager->RemoveTimer("Energy", citizen);
	Camera->TimerManager->RemoveTimer("Eat", citizen);

	citizen->AIController->StopMovement();
	citizen->MovementComponent->SetMaxSpeed(0.0f);

	UNiagaraComponent* component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SacrificeSystem, citizen->MovementComponent->Transform.GetLocation());

	TArray<FTimerParameterStruct> params1;
	Camera->TimerManager->SetParameter(1000, params1);
	Camera->TimerManager->SetParameter(Camera, params1);
	Camera->TimerManager->SetParameter(citizen->AttackComponent->OnHitSound, params1);

	Camera->TimerManager->CreateTimer("Sacrifice", citizen, 4.0f, "TakeHealth", params1, false);

	IncrementPray(*faction, "Bad", 1);
}

void UCitizenManager::IncrementPray(FFactionStruct Faction, FString Type, int32 Increment)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	if (Type == "Good")
		faction->PrayStruct.Good = FMath::Max(faction->PrayStruct.Good + Increment, 0);
	else
		faction->PrayStruct.Bad = FMath::Max(faction->PrayStruct.Bad + Increment, 0);

	if (Increment != -1)
		SetPrayTimer(Faction, Type);
}

TArray<ACitizen*> UCitizenManager::GetCitizensOfReligion(FString FactionName, FString ReligionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	TArray<ACitizen*> citizens;

	for (ACitizen* citizen : faction->Citizens)
		if (citizen->HealthComponent->GetHealth() > 0 && citizen->Spirituality.Faith == ReligionName)
			citizens.Add(citizen);

	return citizens;
}

void UCitizenManager::SetPrayTimer(FFactionStruct Faction, FString Type)
{
	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(Faction, params);
	Camera->TimerManager->SetParameter(Type, params);
	Camera->TimerManager->SetParameter(-1, params);

	Camera->TimerManager->CreateTimer("Pray", Camera, timeToCompleteDay, "IncrementPray", params, false);
}

//
// Personality
//
TArray<ACitizen*> UCitizenManager::GetCitizensOfPersonality(FString FactionName, FString PersonalityName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	TArray<ACitizen*> citizens;

	FPersonality personality;
	personality.Trait = PersonalityName;

	int32 index = Personalities.Find(personality);

	for (ACitizen* citizen : Personalities[index].Citizens)
		if (faction->Citizens.Contains(citizen))
			citizens.Add(citizen);

	return citizens;
}

TArray<FPersonality*> UCitizenManager::GetCitizensPersonalities(class ACitizen* Citizen)
{
	TArray<FPersonality*> personalities;

	for (FPersonality& personality : Personalities) {
		if (!personality.Citizens.Contains(Citizen))
			continue;

		personalities.Add(&personality);
	}

	return personalities;
}

float UCitizenManager::GetAggressiveness(ACitizen* Citizen)
{
	TArray<FPersonality*> citizenPersonalities = GetCitizensPersonalities(Citizen);
	float citizenAggressiveness = 0;

	for (FPersonality* personality : citizenPersonalities)
		citizenAggressiveness += personality->Aggressiveness / citizenPersonalities.Num();

	return citizenAggressiveness;
}

void UCitizenManager::PersonalityComparison(ACitizen* Citizen1, ACitizen* Citizen2, int32& Likeness, float& Citizen1Aggressiveness, float& Citizen2Aggressiveness)
{
	TArray<FPersonality*> citizen1Personalities = GetCitizensPersonalities(Citizen1);
	TArray<FPersonality*> citizen2Personalities = GetCitizensPersonalities(Citizen2);

	for (FPersonality* personality : citizen1Personalities) {
		Citizen1Aggressiveness += personality->Aggressiveness / citizen1Personalities.Num();

		for (FPersonality* p : citizen2Personalities) {
			Citizen2Aggressiveness += p->Aggressiveness / citizen2Personalities.Num();

			if (personality->Trait == p->Trait)
				Likeness += 2;
			else if (personality->Likes.Contains(p->Trait))
				Likeness++;
			else if (personality->Dislikes.Contains(p->Trait))
				Likeness--;
		}
	}
}

void UCitizenManager::PersonalityComparison(ACitizen* Citizen1, ACitizen* Citizen2, int32& Likeness)
{
	TArray<FPersonality*> citizen1Personalities = GetCitizensPersonalities(Citizen1);
	TArray<FPersonality*> citizen2Personalities = GetCitizensPersonalities(Citizen2);

	for (FPersonality* personality : citizen1Personalities) {
		for (FPersonality* p : citizen2Personalities) {
			if (personality->Trait == p->Trait)
				Likeness += 2;
			else if (personality->Likes.Contains(p->Trait))
				Likeness++;
			else if (personality->Dislikes.Contains(p->Trait))
				Likeness--;
		}
	}
}