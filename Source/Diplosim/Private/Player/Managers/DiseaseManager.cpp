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
	if (Infected.IsEmpty() && Injured.IsEmpty()) {
		if (Camera->SaveGameComponent->IsLoading())
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Disease);

		return;
	}

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

				if (citizen->HealthComponent->GetHealth() <= 0 || (!Infected.Contains(citizen) && (!IsValid(citizen->BuildingComponent->Employment) || !citizen->BuildingComponent->Employment->IsA<AClinic>())))
					continue;

				if (IsValid(citizen->BuildingComponent->Employment) && citizen->BuildingComponent->Employment->IsA<AClinic>() && faction.Citizens.Contains(citizen->AIController->MoveRequest.GetGoalActor()) && citizen->CanReach(citizen->AIController->MoveRequest.GetGoalActor(), citizen->Range / 15.0f)) {
					TArray<FTimerParameterStruct> params;
					Camera->TimerManager->SetParameter(citizen->AIController->MoveRequest.GetGoalActor(), params);

					Camera->TimerManager->CreateTimer("Healing", citizen, 1.0f / citizen->GetProductivity(), "Heal", params, false);
				}
				else if (Infectible.Contains(citizen) && Infected.Contains(citizen)) {
					FOverlapsStruct requestedOverlaps;
					requestedOverlaps.GetCitizenInteractions(false, true);

					TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range / 15.0f, requestedOverlaps, EFactionType::Both);

					for (AActor* actor : actors) {
						if (!IsValid(actor))
							continue;

						ACitizen* c = Cast<ACitizen>(actor);

						if (c->HealthComponent->GetHealth() <= 0 || !Infectible.Contains(c))
							continue;

						bool bInfected = false;

						for (FConditionStruct condition : citizen->HealthIssues) {
							if (c->HealthIssues.Contains(condition))
								continue;

							int32 chance = Camera->Stream.RandRange(1, 100);

							if (chance <= condition.Spreadability) {
								GiveCondition(c, condition);

								bInfected = true;

								Camera->NotifyLog("Bad", citizen->BioComponent->Name + " is infected with " + condition.Name, Camera->ConquestManager->GetCitizenFaction(citizen).Name);
							}
						}

						if (bInfected && !Infected.Contains(c))
							Infect(c);
					}
				}
			}

			PairCitizenToHealer(&faction);
		}
	});
}

void UDiseaseManager::GiveCondition(ACitizen* Citizen, FConditionStruct Condition)
{
	Condition.AcquiredTime = GetWorld()->GetTimeSeconds();
	Citizen->HealthIssues.Add(Condition);
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
	int32 index = Camera->Stream.RandRange(0, Infectible.Num() - 1);
	ACitizen* citizen = Infectible[index];

	index = Camera->Stream.RandRange(0, Diseases.Num() - 1);
	GiveCondition(citizen, Diseases[index]);

	Camera->NotifyLog("Bad", citizen->BioComponent->Name + " is infected with " + Diseases[index].Name, Camera->ConquestManager->GetCitizenFaction(citizen).Name);

	Infect(citizen);

	StartDiseaseTimer(Camera);
}

void UDiseaseManager::Infect(ACitizen* Citizen)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Citizen]() {
		Infected.Add(Citizen);

		UpdateHealthText(Citizen);
	});
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
	GiveCondition(Citizen, Diseases[index]);

	for (FAffectStruct affect : conditions[index].Affects) {
		if (affect.Affect == EAffect::Movement)
			Citizen->ApplyToMultiplier("Speed", affect.Amount);
		else if (affect.Affect == EAffect::Damage)
			Citizen->ApplyToMultiplier("Damage", affect.Amount);
		else
			Citizen->ApplyToMultiplier("Health", affect.Amount);
	}

	Citizen->Camera->NotifyLog("Bad", Citizen->BioComponent->Name + " is injured with " + conditions[index].Name, Citizen->Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	Injured.Add(Citizen);

	UpdateHealthText(Citizen);
}

void UDiseaseManager::Cure(ACitizen* Citizen)
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

	Citizen->HealthIssues.Empty();

	Infected.Remove(Citizen);
	Injured.Remove(Citizen);

	Citizen->Camera->NotifyLog("Good", Citizen->BioComponent->Name + " has been healed", Citizen->Camera->ConquestManager->GetCitizenFaction(Citizen).Name);

	UpdateHealthText(Citizen);
}

void UDiseaseManager::UpdateHealthText(ACitizen* Citizen)
{
	if (Citizen->Camera->WidgetComponent->IsAttachedTo(Citizen->GetRootComponent()))
		Async(EAsyncExecution::TaskGraphMainTick, [this, Citizen]() { Citizen->Camera->UpdateHealthIssues(); });
}

TArray<ACitizen*> UDiseaseManager::GetAvailableHealers(FFactionStruct* Faction, TArray<ACitizen*>& Ill, ACitizen* Target)
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
			if (goal->IsA<ACitizen>() && Ill.Contains(Cast<ACitizen>(goal)))
				Ill.Remove(Cast<ACitizen>(goal));

			if (IsValid(Target) && Target != citizen)
				continue;

			healers.Add(citizen);
		}
	}

	return healers;
}

void UDiseaseManager::PairCitizenToHealer(FFactionStruct* Faction, ACitizen* Healer)
{
	TArray<ACitizen*> ill;
	ill.Append(Infected);
	ill.Append(Injured);

	TArray<ACitizen*> healers;
	healers.Append(GetAvailableHealers(Faction, ill, Healer));

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

			int32 curValue = Infected.Contains(chosenPatient) ? 3.0f : 1.0f;
			int32 newValue = Infected.Contains(citizen) ? 3.0f : 1.0f;

			double magnitude = healer->AIController->GetClosestActor(50.0f, citizen->Camera->GetTargetActorLocation(healer), healer->Camera->GetTargetActorLocation(chosenPatient), healer->Camera->GetTargetActorLocation(citizen), true, curValue, newValue);

			if (magnitude > 0.0f)
				chosenPatient = citizen;
		}

		if (IsValid(chosenPatient)) {
			healer->AIController->AIMoveTo(chosenPatient);

			ill.Remove(chosenPatient);
		}
		else
			healer->AIController->DefaultAction();
	}
}