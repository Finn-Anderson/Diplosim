#include "Player/Managers/DiseaseManager.h"

#include "Components/WidgetComponent.h"
#include "Misc/ScopeTryLock.h"

#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Work/Service/Clinic.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Universal/HealthComponent.h"

UDiseaseManager::UDiseaseManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Conditions.json");
}

void UDiseaseManager::ReadJSONFile(FString path)
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	FString fileContents;
	FFileHelper::LoadFileToString(fileContents, *path);
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

	if (!FJsonSerializer::Deserialize(jsonReader, jsonObject) || !jsonObject.IsValid())
		return;

	for (auto& element : jsonObject->Values) {
		for (auto& e : element.Value->AsArray()) {
			FConditionStruct condition;

			for (auto& v : e->AsObject()->Values) {
				if (v.Value->Type == EJson::Array) {
					for (auto& ev : v.Value->AsArray()) {
						FAffectStruct affect;

						for (auto& bv : ev->AsObject()->Values) {
							if (bv.Key == "Affect")
								affect.Affect = EAffect(FCString::Atoi(*bv.Value->AsString()));
							else
								affect.Amount = bv.Value->AsNumber();
						}

						condition.Affects.Add(affect);
					}
				}
				else if (v.Value->Type == EJson::String) {
					condition.Name = v.Value->AsString();
				}
				else {
					uint8 index = FCString::Atoi(*v.Value->AsString());

					if (v.Key == "Grade")
						condition.Grade = EGrade(index);
					else if (v.Key == "Spreadability")
						condition.Spreadability = index;
					else
						condition.DeathTime = index;
				}
			}

			if (element.Key == "Injuries")
				Injuries.Add(condition);
			else
				Diseases.Add(condition);
		}
	}
}

void UDiseaseManager::CalculateDisease(ACamera* Camera)
{
	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock lock(&DiseaseSpreadLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Disease);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens;
			citizens.Append(faction.Citizens);
			citizens.Append(faction.Rebels);

			for (ACitizen* citizen : citizens) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen))
					continue;

				for (FConditionStruct& condition : citizen->HealthIssues)
					if (condition.AcquiredTime + condition.DeathTime <= GetWorld()->GetTimeSeconds())
						citizen->HealthComponent->TakeHealth(1000, citizen);

				if (citizen->HealthComponent->GetHealth() <= 0)
					continue;

				float reach = citizen->GetReach();

				if (IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->IsA<AClinic>() && faction.Citizens.Contains(citizen->AIController->MoveRequest.GetGoalActor()) && citizen->CanReach(citizen->AIController->MoveRequest.GetGoalActor(), reach)) {
					if (Camera->TimerManager->DoesTimerExist("Healing", citizen))
						continue;

					TArray<FTimerParameterStruct> params;
					Camera->TimerManager->SetParameter(citizen, params);
					Camera->TimerManager->SetParameter(citizen->AIController->MoveRequest.GetGoalActor(), params);

					Camera->TimerManager->CreateTimer("Healing" + citizen->GetName(), Camera, 1.0f / citizen->GetProductivity(), "Cure", params, false);
				}
				else if (HasInfection(citizen)) {
					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.GetCitizenInteractions(false, true);

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Both);

					for (AActor* actor : actors) {
						if (!IsValid(actor))
							continue;

						ACitizen* c = Cast<ACitizen>(actor);

						if (c->HealthComponent->GetHealth() <= 0 || !IsInfectible(c))
							continue;

						for (FConditionStruct condition : citizen->HealthIssues) {
							if (c->HealthIssues.Contains(condition) || citizen->ImmunityTimer > 0)
								continue;

							int32 chance = Camera->Stream.RandRange(1, 10000);

							if (chance <= condition.Spreadability)
								GiveCondition(Camera, c, condition);
						}
					}
				}

				citizen->ImmunityTimer = FMath::Max(citizen->ImmunityTimer--, 0);
			}

			PairCitizenToHealer(&faction);
		}
	});
}

void UDiseaseManager::GiveCondition(ACamera* Camera, ACitizen* Citizen, FConditionStruct Condition)
{
	Condition.AcquiredTime = GetWorld()->GetTimeSeconds();
	Citizen->HealthIssues.Add(Condition);

	FString status = "infected";
	if (Condition.Spreadability == 0)
		status = "injured";

	FFactionStruct faction = Camera->ConquestManager->GetCitizenFaction(Citizen);

	if (faction.Name == Camera->ColonyName)
		Camera->NotifyLog("Bad", Citizen->BioComponent->Name + " is " + status + " with " + Condition.Name, faction.Name);

	UpdateHealthText(Citizen);
}

void UDiseaseManager::StartDiseaseTimer(ACamera* Camera)
{
	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(Camera, params);

	Camera->TimerManager->CreateTimer("Disease", Camera, Camera->Stream.RandRange(timeToCompleteDay / 2, timeToCompleteDay * 3), "SpawnDisease", params, false);
}

void UDiseaseManager::SpawnDisease(ACamera* Camera)
{
	for (FFactionStruct faction : Camera->ConquestManager->Factions) {
		TArray<ACitizen*> infectible;
		for (ACitizen* citizen : faction.Citizens)
			if (IsInfectible(citizen))
				infectible.Add(citizen);

		if (infectible.IsEmpty())
			continue;

		int32 infectibleIndex = Camera->Stream.RandRange(0, infectible.Num() - 1);
		int32 diseaseIndex = Camera->Stream.RandRange(0, Diseases.Num() - 1);

		GiveCondition(Camera, infectible[infectibleIndex], Diseases[diseaseIndex]);
	}

	StartDiseaseTimer(Camera);
}

void UDiseaseManager::Injure(ACitizen* Citizen, int32 Odds)
{
	int32 index = Citizen->Camera->Stream.RandRange(1, 10);

	if (index < Odds)
		return;

	TArray<FConditionStruct> conditions = Injuries;

	for (FConditionStruct condition : Citizen->HealthIssues)
		conditions.Remove(condition);

	if (conditions.IsEmpty())
		return;

	index = Citizen->Camera->Stream.RandRange(0, conditions.Num() - 1);
	GiveCondition(Citizen->Camera, Citizen, conditions[index]);

	for (FAffectStruct affect : conditions[index].Affects) {
		if (affect.Affect == EAffect::Movement)
			Citizen->ApplyToMultiplier("Speed", affect.Amount);
		else if (affect.Affect == EAffect::Damage)
			Citizen->ApplyToMultiplier("Damage", affect.Amount);
		else
			Citizen->ApplyToMultiplier("Health", affect.Amount);
	}

	Citizen->Camera->PlayAmbientSound(Citizen->AmbientAudioComponent, InjureSound);
}

void UDiseaseManager::Cure(ACitizen* Healer, ACitizen* Citizen)
{
	for (FConditionStruct condition : Citizen->HealthIssues) {
		for (FAffectStruct affect : condition.Affects) {
			if (affect.Affect == EAffect::Movement)
				Citizen->ApplyToMultiplier("Speed", 1.0f + (1.0f - affect.Amount));
			else if (affect.Affect == EAffect::Damage)
				Citizen->ApplyToMultiplier("Damage", 1.0f + (1.0f - affect.Amount));
			else
				Citizen->ApplyToMultiplier("Health", 1.0f + (1.0f - affect.Amount));
		}
	}

	Citizen->ImmunityTimer = Citizen->Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay() / 4;
	Citizen->HealthIssues.Empty();

	FFactionStruct faction = Citizen->Camera->ConquestManager->GetCitizenFaction(Citizen);

	if (faction.Name == Citizen->Camera->ColonyName)
		Citizen->Camera->NotifyLog("Good", Citizen->BioComponent->Name + " has been healed", Citizen->Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	if (Citizen != Healer)
		Healer->AIController->DefaultAction();

	UpdateHealthText(Citizen);
}

void UDiseaseManager::UpdateHealthText(ACitizen* Citizen)
{
	if (Citizen->Camera->WidgetComponent->IsAttachedTo(Citizen->GetRootComponent()))
		Async(EAsyncExecution::TaskGraphMainTick, [this, Citizen]() { Citizen->Camera->UpdateHealthIssues(); });
}

TArray<ACitizen*> UDiseaseManager::GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill)
{
	TArray<ACitizen*> healers;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<AClinic>())
			continue;

		AClinic* clinic = Cast<AClinic>(building);

		for (ACitizen* citizen : clinic->GetOccupied()) {
			if (!clinic->IsWorking(citizen) || citizen->Camera->ArmyManager->IsCitizenInAnArmy(citizen))
				continue;

			AActor* goal = citizen->AIController->MoveRequest.GetGoalActor();

			if (IsValid(goal) && Ill.Contains(goal)) {
				Ill.Remove(Cast<ACitizen>(goal));

				continue;
			}

			healers.Add(citizen);
		}
	}

	return healers;
}

void UDiseaseManager::PairCitizenToHealer(FFactionStruct* Faction)
{
	TArray<ACitizen*> ill = GetIll(Faction, false);

	if (ill.IsEmpty())
		return;

	TArray<ACitizen*> healers;
	healers.Append(GetAvailableHealers(Faction, ill));

	for (ACitizen* healer : healers) {
		ACitizen* chosenPatient = nullptr;
		FVector location = healer->Camera->GetTargetActorLocation(healer);

		for (ACitizen* citizen : ill) {
			if (!citizen->AIController->CanMoveTo(location))
				continue;

			if (!IsValid(chosenPatient)) {
				chosenPatient = citizen;

				continue;
			}

			int32 curValue = HasInfection(chosenPatient) ? 3.0f : 1.0f;
			int32 newValue = HasInfection(citizen) ? 3.0f : 1.0f;

			double magnitude = healer->AIController->GetClosestActor(50.0f, citizen->Camera->GetTargetActorLocation(healer), healer->Camera->GetTargetActorLocation(chosenPatient), healer->Camera->GetTargetActorLocation(citizen), true, curValue, newValue);

			if (magnitude > 0.0f)
				chosenPatient = citizen;
		}

		if (IsValid(chosenPatient)) {
			healer->AIController->AIMoveTo(chosenPatient);

			ill.Remove(chosenPatient);
		}
	}
}

TArray<ACitizen*> UDiseaseManager::GetIll(FFactionStruct* Faction, bool bOnlyInfections)
{
	TArray<ACitizen*> ill;

	for (ACitizen* citizen : Faction->Citizens)
		if (!citizen->HealthIssues.IsEmpty() && (!bOnlyInfections || HasInfection(citizen)))
			ill.Add(citizen);

	return ill;
}

TTuple<bool, bool> UDiseaseManager::HasInjuryAndInfection(ACitizen* Citizen)
{
	TTuple<bool, bool> status = TTuple<bool, bool>(false, false);

	for (FConditionStruct condition : Citizen->HealthIssues) {
		if (condition.Spreadability > 0)
			status.Value = true;
		else
			status.Key = true;
	}

	return status;
}

bool UDiseaseManager::HasInfection(ACitizen* Citizen)
{
	for (FConditionStruct condition : Citizen->HealthIssues)
		if (condition.Spreadability > 0)
			return true;

	return false;
}

bool UDiseaseManager::IsInfectible(ACitizen* Citizen)
{
	return !IsValid(Citizen->BuildingComponent->Employment) || !Citizen->BuildingComponent->Employment->IsA<AClinic>();
}