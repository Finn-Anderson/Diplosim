#include "Player/Managers/PoliceManager.h"

#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Misc/ScopeTryLock.h"

#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Buildings/Work/Service/School.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Components/SaveGameComponent.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"
#include "DebugManager.h"

UPoliceManager::UPoliceManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPoliceManager::CalculateVandalism()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&VandalismInteractionsLock);
		if (!lock.IsLocked())
			return;

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty() || !faction.Rebels.IsEmpty() || faction.Buildings.Num() < 2)
				continue;

			for (ACitizen* citizen : citizens) {
				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				FPartyStruct* partyStruct = Camera->PoliticsManager->GetMembersParty(citizen);

				if (citizen->bConversing || citizen->bSleep || IsValid(citizen->BuildingComponent->BuildingAt) || partyStruct == nullptr || partyStruct->Party != "Shell Breakers")
					continue;

				int32 aggressiveness = 0;
				TArray<FPersonality*> personalities = Camera->CitizenManager->GetCitizensPersonalities(citizen);

				for (FPersonality* personality : personalities)
					aggressiveness += personality->Aggressiveness / personalities.Num();

				if (aggressiveness == 0)
					continue;

				int32 max = citizen->HappinessComponent->GetHappiness() / aggressiveness + 50;

				if (Camera->Stream.RandRange(1, max) != max)
					continue;

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bBuildings = true;

				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->GetReach(), requestedOverlaps, EFactionType::Same, &faction);

				for (AActor* actor : actors) {
					if (actor->IsA<ABroch>() || actor->IsA<ARoad>() || actor->IsA<AFestival>())
						continue;

					Async(EAsyncExecution::TaskGraphMainTick, [this, citizen, actor]() { Camera->Grid->AtmosphereComponent->SetOnFire(actor); });

					CreatePoliceReport(&faction, EReportType::Vandalism, citizen, false, actor);
				}
			}
		}
	});
}

void UPoliceManager::ProcessReports()
{
	Async(EAsyncExecution::TaskGraph, [this]() {
		FScopeTryLock lock(&ReportInteractionsLock);
		if (!lock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Report);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> availableOfficers;
			TArray<ACitizen*> pursuitedWanted;

			for (ABuilding* building : faction.Buildings) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!building->IsA(PoliceStationClass) || building->GetOccupied().IsEmpty())
					continue;

				for (ACitizen* officer : building->GetOccupied()) {
					if (!IsValid(officer))
						continue;

					if (IsInAPoliceReport(officer, &faction)) {
						pursuitedWanted.Add(Cast<ACitizen>(officer->AIController->MoveRequest.GetGoalActor()));

						continue;
					}

					availableOfficers.Add(officer);
				}
			}

			if (availableOfficers.IsEmpty())
				return;

			for (int32 i = faction.Police.PoliceReports.Num() - 1; i > -1; i--) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				FPoliceReport report = faction.Police.PoliceReports[i];

				if (report.Wanted.IsEmpty()) {
					faction.Police.PoliceReports.RemoveAt(i);

					continue;
				}

				for (int32 j = report.Wanted.Num() - 1; j > -1; j--)
					if (pursuitedWanted.Contains(report.Wanted[j]) || Camera->TimerManager->FindTimer(report.Wanted[j]->GetName() + "Jail", Camera))
						report.Wanted.RemoveAt(j);

				if (report.Wanted.IsEmpty())
					continue;

				for (ACitizen* wanted : report.Wanted) {
					ACitizen* chosenOfficer = nullptr;

					for (ACitizen* officer : availableOfficers) {
						if (!IsValid(chosenOfficer)) {
							chosenOfficer = officer;

							continue;
						}

						double magnitude = chosenOfficer->AIController->GetClosestActor(1.0f, Camera->GetTargetActorLocation(wanted), Camera->GetTargetActorLocation(chosenOfficer), Camera->GetTargetActorLocation(officer));

						if (magnitude < 0.0f)
							continue;

						chosenOfficer = officer;
					}

					if (!IsValid(chosenOfficer))
						return;

					Camera->Grid->AIVisualiser->ToggleOfficerLights(chosenOfficer, 1.0f);

					chosenOfficer->AIController->AIMoveTo(wanted);

					availableOfficers.Remove(chosenOfficer);
				}
			}
		}
	});
}

void UPoliceManager::CalculateIfFight(FFactionStruct* Faction, ACitizen* Citizen1, ACitizen* Citizen2, float Citizen1Aggressiveness, float Citizen2Aggressiveness)
{
	if (Faction->Citizens.Num() <= 20 || Faction->Police.Arrested.Contains(Citizen1) || Faction->Police.Arrested.Contains(Citizen2))
		return;

	FVector midPoint = (Camera->GetTargetActorLocation(Citizen1) + Camera->GetTargetActorLocation(Citizen2)) / 2;
	bool closeToPoliceStation = false;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA(PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
			continue;

		float dist = FVector::Dist(midPoint, building->GetActorLocation());

		if (dist > 1000.0f)
			continue;

		closeToPoliceStation = true;

		break;
	}

	if (IsValid(Citizen1->BuildingComponent->Employment) && Citizen1->BuildingComponent->Employment->IsA(PoliceStationClass))
		Citizen1Aggressiveness *= 0.5f;

	if (IsValid(Citizen2->BuildingComponent->Employment) && Citizen2->BuildingComponent->Employment->IsA(PoliceStationClass))
		Citizen2Aggressiveness *= 0.5f;

	int32 aggressorValue = Camera->Stream.FRandRange(0.0f, Citizen1Aggressiveness + Citizen2Aggressiveness);

	ACitizen* aggressor = Citizen1;
	if (aggressorValue < Citizen2Aggressiveness)
		aggressor = Citizen2;

	FOverlapsStruct requestedOverlaps;
	requestedOverlaps.GetCitizenInteractions(false, false);

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, aggressor, aggressor->InitialRange, requestedOverlaps, EFactionType::Same);

	int32 chance = Camera->Stream.RandRange(0, 100);
	
	bool bForceFight = Cast<UDebugManager>(Camera->PController->CheatManager)->bFight;
	int32 fightChance = bForceFight ? chance + 1 : FMath::Max(25 * Citizen1Aggressiveness * Citizen2Aggressiveness - closeToPoliceStation ? 50 : 0 - (10 * actors.Num()) , 1);

	if (fightChance > chance)
		Camera->PoliceManager->CreatePoliceReport(Faction, EReportType::Fighting, aggressor, fightChance < 66, aggressor == Citizen1 ? Citizen2 : Citizen1);
}

bool UPoliceManager::IsCarelessWitness(ACitizen* Citizen)
{
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(Citizen))
		if (personality->Trait == "Careless")
			return true;

	return false;
}

bool UPoliceManager::IsPoliceOfficer(ACitizen* Citizen)
{
	return IsValid(Citizen->BuildingComponent->Employment) && Citizen->BuildingComponent->Employment->IsA(Camera->PoliceManager->PoliceStationClass);
}

void UPoliceManager::CreatePoliceReport(FFactionStruct* Faction, EReportType ReportType, ACitizen* Accused, bool bShowMercy, AActor* Defender)
{
	FPoliceReport report;
	report.Type = ReportType;

	FVector Location = (Camera->GetTargetActorLocation(Accused) + (IsValid(Defender) ? Camera->GetTargetActorLocation(Defender) : Camera->GetTargetActorLocation(Accused))) / 2;

	FOverlapsStruct overlaps;
	overlaps.bCitizens = true;

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, Accused, Accused->InitialRange, overlaps, EFactionType::Same, Faction, Location);

	if (actors.IsEmpty())
		return;

	actors.Remove(Defender);

	TArray<ACitizen*> team1 = { Accused };
	TArray<ACitizen*> team2;
	if (IsValid(Defender) && Defender->IsA<ACitizen>())
		team2.Add(Cast<ACitizen>(Defender));

	int32 lean = 0;

	for (AActor* actor : actors) {
		ACitizen* citizen = Cast<ACitizen>(actor);

		if (IsCarelessWitness(citizen))
			continue;

		int32 count1 = 0;
		int32 count2 = 0;

		float citizenAggressiveness = 0;
		float c1Aggressiveness = 0;
		float c2Aggressiveness = 0;

		Camera->CitizenManager->PersonalityComparison(citizen, Accused, count1, citizenAggressiveness, c1Aggressiveness);
		if (IsValid(Defender) && Defender->IsA<ACitizen>())
			Camera->CitizenManager->PersonalityComparison(citizen, Cast<ACitizen>(Defender), count2, citizenAggressiveness, c2Aggressiveness);

		if (ReportType == EReportType::Vandalism) {
			if (citizen->BuildingComponent->BuildingAt == Defender || citizen->BuildingComponent->House == Defender) {
				team2.Add(citizen);
				lean++;
			}
		}
		else {
			int32 value = FMath::Abs(count1 - count2);

			if (Camera->Stream.RandRange(0, 100) < citizenAggressiveness * value * 20) {
				if (count1 > count2) {
					team1.Add(citizen);
					lean--;
				}
				else {
					team2.Add(citizen);
					lean++;
				}
			}
		}

		if (team1.Contains(citizen) || team2.Contains(citizen))
			continue;

		lean += (count2 - count1) * 2;
	}

	for (ACitizen* citizen1 : team1) {
		for (ACitizen* citizen2 : team2) {
			if (!citizen1->AttackComponent->OverlappingEnemies.Contains(citizen2))
				citizen1->AttackComponent->OverlappingEnemies.Add(citizen2);

			if (!citizen2->AttackComponent->OverlappingEnemies.Contains(citizen1))
				citizen2->AttackComponent->OverlappingEnemies.Add(citizen1);

			citizen1->AttackComponent->bShowMercy = bShowMercy;
			citizen2->AttackComponent->bShowMercy = bShowMercy;

			citizen1->AIController->ClearMovementTimers();
			citizen1->AIController->StartMovement();

			citizen2->AIController->ClearMovementTimers();
			citizen2->AIController->StartMovement();
		}
	}

	if (FMath::Abs(lean) > actors.Num()) {
		if (lean > 0)
			report.Wanted.Append(team1);
		else
			report.Wanted.Append(team2);
	}

	Faction->Police.PoliceReports.Add(report);
}

bool UPoliceManager::IsInAPoliceReport(ACitizen* Citizen, FFactionStruct* Faction)
{
	for (FPoliceReport& report : Faction->Police.PoliceReports) {
		if (!report.Wanted.Contains(Citizen) && (!IsPoliceOfficer(Citizen) || !report.Wanted.Contains(Citizen->AIController->MoveRequest.GetGoalActor())))
			continue;

		return true;
	}

	return false;
}

void UPoliceManager::ChangeReportToMurder(ACitizen* Citizen, ACitizen* Attacker)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Wanted.Contains(Citizen) && !report.Wanted.Contains(Attacker))
			continue;

		if (report.Type == EReportType::Fighting)
			report.Type = EReportType::Murder;

		if (report.Wanted.Contains(Citizen)) {
			report.Wanted.Remove(Citizen);

			if (report.Wanted.IsEmpty())
				faction->Police.PoliceReports.Remove(report);
		}

		break;
	}
}

void UPoliceManager::CeaseAllInternalFighting(FFactionStruct* Faction)
{
	for (ACitizen* citizen : Faction->Citizens) {
		for (int32 i = citizen->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
			AActor* actor = citizen->AttackComponent->OverlappingEnemies[i];

			if (Faction->Citizens.Contains(actor))
				citizen->AttackComponent->OverlappingEnemies.RemoveAt(i);
		}

		citizen->AttackComponent->bShowMercy = false;
		citizen->AIController->DefaultAction();
	}
}

void UPoliceManager::RepositionCriminals(ABuilding* Building)
{
	if (!Building->IsA(Camera->PoliceManager->PoliceStationClass))
		return;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Building->FactionName);

	for (ACitizen* citizen : faction->Citizens) {
		if (citizen->BuildingComponent->BuildingAt != Building || Building->GetOccupied().Contains(citizen) || !faction->Police.Arrested.Contains(citizen))
			continue;

		Camera->PoliceManager->Jail(*faction, Building->GetOccupied().IsEmpty() ? nullptr : Building->GetOccupied()[0], citizen);
	}
}

void UPoliceManager::Arrest(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	if (!Citizen->HealthIssues.IsEmpty())
		Camera->DiseaseManager->Cure(Citizen, Citizen);

	Citizen->ClearCitizen();

	Officer->AIController->StopMovement();
	Citizen->AIController->StopMovement();

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Wanted.Contains(Citizen))
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		if (Camera->PoliticsManager->GetLawValue(faction->Name, law) == 0) {
			report.Wanted.Remove(Citizen);

			Officer->AIController->DefaultAction();
			Citizen->AIController->StartMovement();

			return;
		}

		break;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(ArrestSystem, Citizen->GetRootComponent(), "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(*faction, params);
	Camera->TimerManager->SetParameter(Officer, params);
	Camera->TimerManager->SetParameter(Citizen, params);

	Camera->TimerManager->CreateTimer(Citizen->GetName() + "Jail", Camera, 2.0f, "Jail", params, false);
}

void UPoliceManager::Jail(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	if (!IsValid(Officer)) {
		for (ABuilding* building : faction->Buildings) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			if (!building->IsA(PoliceStationClass) || building->GetOccupied().IsEmpty())
				continue;

			for (ACitizen* officer : building->GetOccupied()) {
				if (!IsValid(officer))
					continue;

				if (!IsValid(Officer)) {
					Officer = officer;

					continue;
				}

				double magnitude = Officer->AIController->GetClosestActor(1.0f, Camera->GetTargetActorLocation(Citizen), Camera->GetTargetActorLocation(Officer), Camera->GetTargetActorLocation(officer));

				if (magnitude < 0.0f)
					continue;

				Officer = officer;
			}
		}

		if (!IsValid(Officer)) {
			TMap<ACitizen*, int32> arrested = faction->Police.Arrested;
			faction->Police.Arrested.Empty();

			for (auto& element : arrested)
				element.Key->AIController->DefaultAction();
		}
	}

	Citizen->MovementComponent->Transform.SetLocation(Officer->BuildingComponent->Employment->GetActorLocation());
	Citizen->BuildingComponent->BuildingAt = Officer->BuildingComponent->Employment;
	Citizen->AIController->Idle(faction, Citizen);

	FPartyStruct* party = Camera->PoliticsManager->GetMembersParty(Citizen);

	if (party != nullptr) {
		if (party->Leader == Citizen)
			Camera->PoliticsManager->SelectNewLeader(party);

		party->Members.Remove(Citizen);
	}

	if (faction->Politics.Representatives.Contains(Citizen))
		faction->Politics.Representatives.Remove(Citizen);

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.Wanted.Contains(Citizen))
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		faction->Police.Arrested.Add(Citizen, Camera->PoliticsManager->GetLawValue(faction->Name, law));

		if (faction->Name == Citizen->Camera->ColonyName)
			Citizen->Camera->NotifyLog(Citizen, "Bad", Citizen->BioComponent->Name + " has been arrested", faction->Name);

		report.Wanted.Remove(Citizen);

		break;
	}

	if (Officer->AIController->MoveRequest.GetGoalActor() != Citizen || Officer->HealthComponent->GetHealth() == 0)
		return;

	Camera->Grid->AIVisualiser->ToggleOfficerLights(Officer, 0.0f);

	Officer->AIController->DefaultAction();
}

void UPoliceManager::ItterateThroughSentences()
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		TArray<ACitizen*> served;

		for (auto& element : faction.Police.Arrested) {
			element.Value--;

			if (element.Value == 0)
				served.Add(element.Key);
		}

		Camera->ResourceManager->AddUniversalResource(&faction, Camera->ResourceManager->Money, faction.Police.Arrested.Num());

		for (ACitizen* citizen : served) {
			faction.Police.Arrested.Remove(citizen);

			if (faction.Name == citizen->Camera->ColonyName)
				citizen->Camera->NotifyLog(citizen, "Good", citizen->BioComponent->Name + " has been released from prison", faction.Name);

			citizen->MovementComponent->Transform.SetLocation(citizen->BuildingComponent->BuildingAt->BuildingMesh->GetSocketLocation("Entrance"));
		}
	}
}