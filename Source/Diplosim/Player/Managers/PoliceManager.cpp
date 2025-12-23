#include "Player/Managers/PoliceManager.h"

#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Misc/ScopeTryLock.h"

#include "AI/Citizen.h"
#include "AI/AttackComponent.h"
#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/BuildingComponent.h"
#include "AI/HappinessComponent.h"
#include "Buildings/House.h"
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

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Vandalism);

			return;
		}

		for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
			TArray<ACitizen*> citizens = faction.Citizens;

			if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty() || !faction.Rebels.IsEmpty() || faction.Buildings.Num() < 2)
				continue;

			for (ACitizen* citizen : citizens) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || citizen->IsHidden() || faction.Police.Arrested.Contains(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
					continue;

				FPartyStruct* partyStruct = Camera->PoliticsManager->GetMembersParty(citizen);

				if (citizen->bConversing || citizen->bSleep || IsValid(citizen->BuildingComponent->BuildingAt) || partyStruct == nullptr || partyStruct->Party != "Shell Breakers")
					continue;

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bBuildings = true;

				int32 reach = citizen->Range / 15.0f;
				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, reach, requestedOverlaps, EFactionType::Same, &faction);

				for (AActor* actor : actors) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					if (actor->IsA<ABroch>())
						continue;

					int32 aggressiveness = 0;
					TArray<FPersonality*> personalities = Camera->CitizenManager->GetCitizensPersonalities(citizen);

					for (FPersonality* personality : personalities)
						aggressiveness += personality->Aggressiveness / personalities.Num();

					int32 max = (1000 + (citizen->HappinessComponent->GetHappiness() - 50) * 16) / aggressiveness;

					if (Camera->Stream.RandRange(1, max) != max)
						continue;

					Async(EAsyncExecution::TaskGraphMainTick, [this, citizen, actor]() { Camera->Grid->AtmosphereComponent->SetOnFire(actor); });

					int32 index = INDEX_NONE;

					FOverlapsStruct rqstdOvrlps;
					rqstdOvrlps.bCitizens = true;

					TArray<AActor*> actrs = Camera->Grid->AIVisualiser->GetOverlaps(Camera, citizen, citizen->Range, rqstdOvrlps, EFactionType::Same, &faction);

					for (AActor* a : actrs) {
						if (Camera->SaveGameComponent->IsLoading())
							return;

						ACitizen* witness = Cast<ACitizen>(a);

						if (witness->BuildingComponent->BuildingAt == actor || witness->BuildingComponent->House == actor) {
							if (!witness->AttackComponent->OverlappingEnemies.Contains(citizen))
								witness->AttackComponent->OverlappingEnemies.Add(citizen);

							if (!citizen->AttackComponent->OverlappingEnemies.Contains(witness))
								citizen->AttackComponent->OverlappingEnemies.Add(witness);

							if (witness->AttackComponent->OverlappingEnemies.Num() == 1)
								witness->AttackComponent->SetComponentTickEnabled(true);

							if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
								citizen->AttackComponent->SetComponentTickEnabled(true);

							witness->AttackComponent->bShowMercy = false;
							citizen->AttackComponent->bShowMercy = false;
						}
						else {
							CreatePoliceReport(&faction, witness, citizen, EReportType::Vandalism, index);
						}
					}
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
			for (int32 i = 0; i < faction.Police.PoliceReports.Num(); i++) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				FPoliceReport report = faction.Police.PoliceReports[i];

				FOverlapsStruct requestedOverlaps;
				requestedOverlaps.bCitizens = true;

				TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, report.Team1.Instigator, report.Team1.Instigator->InitialRange, requestedOverlaps, EFactionType::Same, &faction, report.Location);

				for (AActor* actor : actors) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					ACitizen* citizen = Cast<ACitizen>(actor);

					if (!IsValid(citizen) || citizen->HealthComponent->GetHealth() <= 0 || faction.Police.Arrested.Contains(citizen) || IsCarelessWitness(citizen) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty() || IsInAPoliceReport(citizen, &faction))
						continue;

					ACitizen* citizenToAttack = nullptr;

					int32 count1 = 0;
					int32 count2 = 0;

					float citizenAggressiveness = 0;
					float c1Aggressiveness = 0;
					float c2Aggressiveness = 0;

					Camera->CitizenManager->PersonalityComparison(citizen, report.Team1.Instigator, count1, citizenAggressiveness, c1Aggressiveness);

					if (citizenAggressiveness >= 1.0f) {
						Camera->CitizenManager->PersonalityComparison(citizen, report.Team2.Instigator, count2, citizenAggressiveness, c2Aggressiveness);

						if (count1 > 1 && count2 < 1)
							citizenToAttack = report.Team2.Instigator;
						else if (count1 < 1 && count2 > 1)
							citizenToAttack = report.Team1.Instigator;
					}

					if (IsValid(citizenToAttack)) {
						if (!citizen->AttackComponent->OverlappingEnemies.Contains(citizenToAttack))
							citizen->AttackComponent->OverlappingEnemies.Add(citizenToAttack);

						if (!citizenToAttack->AttackComponent->OverlappingEnemies.Contains(citizen))
							citizenToAttack->AttackComponent->OverlappingEnemies.Add(citizen);

						if (citizen->AttackComponent->OverlappingEnemies.Num() == 1)
							citizen->AttackComponent->SetComponentTickEnabled(true);

						citizen->AttackComponent->bShowMercy = citizenToAttack->AttackComponent->bShowMercy;

						if (faction.Police.PoliceReports[i].Team1.Instigator == citizenToAttack)
							faction.Police.PoliceReports[i].Team2.Assistors.Add(citizen);
						else
							faction.Police.PoliceReports[i].Team1.Assistors.Add(citizen);
					}
					else {
						CreatePoliceReport(&faction, citizen, report.Team1.Instigator, EReportType::Fighting, i);
					}
				}
			}
		}
	});
}

void UPoliceManager::PoliceInteraction(FFactionStruct* Faction, ACitizen* Citizen, float Reach)
{
	if (!Citizen->CanReach(Citizen->AIController->MoveRequest.GetGoalActor(), Reach) || !Citizen->AIController->MoveRequest.GetGoalActor()->IsA<ACitizen>())
		return;

	ACitizen* c = Cast<ACitizen>(Citizen->AIController->MoveRequest.GetGoalActor());

	bool bInterrogate = true;

	for (FPoliceReport report : Faction->Police.PoliceReports) {
		if (Camera->SaveGameComponent->IsLoading())
			return;

		if (report.RespondingOfficer != Citizen)
			continue;

		if (report.Witnesses.Num() == report.Impartial.Num() + report.AcussesTeam1.Num() + report.AcussesTeam2.Num())
			bInterrogate = false;

		break;
	}

	if (bInterrogate) {
		if (!c->AttackComponent->OverlappingEnemies.IsEmpty())
			StopFighting(c);

		Camera->CitizenManager->StartConversation(Faction, Citizen, c, true);
	}
	else {
		Arrest(Citizen, c);
	}
}

void UPoliceManager::CalculateIfFight(FFactionStruct* Faction, ACitizen* Citizen1, ACitizen* Citizen2, float Citizen1Aggressiveness, float Citizen2Aggressiveness)
{
	FVector midPoint = (Camera->GetTargetActorLocation(Citizen1) + Camera->GetTargetActorLocation(Citizen2)) / 2;
	float distance = 1000;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA(Camera->PoliceManager->PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
			continue;

		float dist = FVector::Dist(midPoint, building->GetActorLocation());

		if (distance > dist)
			distance = dist;
	}

	ACitizen* aggressor = Citizen1;

	if (Citizen1Aggressiveness < Citizen2Aggressiveness)
		aggressor = Citizen2;

	FOverlapsStruct requestedOverlaps;
	requestedOverlaps.GetCitizenInteractions(false, false);

	TArray<AActor*> actors = Camera->Grid->AIVisualiser->GetOverlaps(Camera, aggressor, aggressor->Range, requestedOverlaps, EFactionType::Same);

	int32 chance = Camera->Stream.RandRange(0, 100);
	int32 fightChance = 25 * Citizen1Aggressiveness * Citizen2Aggressiveness - FMath::RoundHalfFromZero(300 / distance) - (10 * actors.Num());

	if (fightChance > chance) {
		if (!Citizen1->AttackComponent->OverlappingEnemies.Contains(Citizen2))
			Citizen1->AttackComponent->OverlappingEnemies.Add(Citizen2);

		if (!Citizen2->AttackComponent->OverlappingEnemies.Contains(Citizen1))
			Citizen2->AttackComponent->OverlappingEnemies.Add(Citizen1);

		if (Citizen1->AttackComponent->OverlappingEnemies.Num() == 1)
			Citizen1->AttackComponent->SetComponentTickEnabled(true);

		if (Citizen2->AttackComponent->OverlappingEnemies.Num() == 1)
			Citizen2->AttackComponent->SetComponentTickEnabled(true);

		if (fightChance < 66) {
			Citizen1->AttackComponent->bShowMercy = true;
			Citizen2->AttackComponent->bShowMercy = true;
		}

		int32 index = INDEX_NONE;

		Camera->PoliceManager->CreatePoliceReport(Faction, nullptr, Citizen1, EReportType::Fighting, index);
	}
}

void UPoliceManager::RespondToReports(FFactionStruct* Faction)
{
	if (Faction->Police.PoliceReports.IsEmpty() || !Faction->Rebels.IsEmpty() || !GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty())
		return;

	for (int32 i = Faction->Police.PoliceReports.Num() - 1; i > -1; i--) {
		if (Camera->SaveGameComponent->IsLoading())
			return;

		FPoliceReport report = Faction->Police.PoliceReports[i];

		if (report.Witnesses.IsEmpty()) {
			bool bStillFighting = false;

			TArray<ACitizen*> team1;
			team1.Add(report.Team1.Instigator);
			team1.Append(report.Team1.Assistors);

			TArray<ACitizen*> team2;
			team1.Add(report.Team2.Instigator);
			team1.Append(report.Team2.Assistors);

			for (ACitizen* citizen : team1) {
				if (Camera->SaveGameComponent->IsLoading())
					return;

				for (ACitizen* c : team2) {
					if (Camera->SaveGameComponent->IsLoading())
						return;

					if (!citizen->AttackComponent->OverlappingEnemies.Contains(c))
						continue;

					bStillFighting = true;

					break;
				}

				if (bStillFighting)
					break;
			}

			if (bStillFighting)
				continue;
		}

		if (IsValid(report.RespondingOfficer)) {
			if (report.Witnesses.IsEmpty()) {
				Faction->Police.PoliceReports.RemoveAt(i);

				report.RespondingOfficer->AIController->DefaultAction();
			}

			continue;
		}
		else if (report.Witnesses.IsEmpty()) {
			Faction->Police.PoliceReports.RemoveAt(i);

			continue;
		}

		float distance = 1000;
		ACitizen* officer = nullptr;

		for (ABuilding* building : Faction->Buildings) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			if (!building->IsA(PoliceStationClass) || building->GetCitizensAtBuilding().IsEmpty())
				continue;

			float dist = FVector::Dist(report.Location, building->GetActorLocation());

			if (distance > dist) {
				distance = dist;

				officer = building->GetCitizensAtBuilding()[Camera->Stream.RandRange(0, building->GetCitizensAtBuilding().Num() - 1)];
			}
		}

		TArray<ACitizen*> witnesses;
		report.Witnesses.GenerateKeyArray(witnesses);

		if (IsValid(officer)) {
			Faction->Police.PoliceReports[i].RespondingOfficer = officer;

			Camera->Grid->AIVisualiser->ToggleOfficerLights(officer, 1.0f);

			officer->AIController->AIMoveTo(witnesses[0]);
		}
	}
}

bool UPoliceManager::IsCarelessWitness(ACitizen* Citizen)
{
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(Citizen))
		if (personality->Trait == "Careless")
			return true;

	return false;
}

void UPoliceManager::CreatePoliceReport(FFactionStruct* Faction, ACitizen* Witness, ACitizen* Accused, EReportType ReportType, int32& Index)
{
	if (Index == INDEX_NONE) {
		FPoliceReport report;

		if (ReportType == EReportType::Vandalism) {
			report.Type = EReportType::Vandalism;

			report.Team1.Instigator = Accused;

			report.Location = Camera->GetTargetActorLocation(report.Team1.Instigator);
		}
		else {
			report.Type = EReportType::Fighting;

			for (int32 i = 0; i < Accused->AttackComponent->OverlappingEnemies.Num(); i++) {
				ACitizen* fighter1 = Cast<ACitizen>(Accused->AttackComponent->OverlappingEnemies[i]);

				if (i == 0) {
					report.Team2.Instigator = fighter1;

					for (int32 j = 0; j < fighter1->AttackComponent->OverlappingEnemies.Num(); j++) {
						ACitizen* fighter2 = Cast<ACitizen>(Accused->AttackComponent->OverlappingEnemies[i]);

						if (i == 0)
							report.Team1.Instigator = fighter2;
						else
							report.Team1.Assistors.Add(fighter2);
					}
				}
				else {
					report.Team2.Assistors.Add(fighter1);
				}
			}

			report.Location = (Camera->GetTargetActorLocation(report.Team1.Instigator) + Camera->GetTargetActorLocation(report.Team2.Instigator)) / 2;
		}

		Faction->Police.PoliceReports.Add(report);

		Index = Faction->Police.PoliceReports.Num() - 1;
	}

	if (!IsValid(Witness))
		return;

	float distance = FVector::Dist(Camera->GetTargetActorLocation(Witness), Camera->GetTargetActorLocation(Accused));
	float dist = 1000000000;

	if (ReportType == EReportType::Fighting) {
		if (Faction->Police.PoliceReports[Index].Witnesses.Contains(Witness))
			dist = *Faction->Police.PoliceReports[Index].Witnesses.Find(Witness);

		GetCloserToFight(Witness, Accused, Faction->Police.PoliceReports[Index].Location);
	}

	if (distance < dist)
		Faction->Police.PoliceReports[Index].Witnesses.Add(Witness, distance);
}

bool UPoliceManager::IsInAPoliceReport(ACitizen* Citizen, FFactionStruct* Faction)
{
	for (FPoliceReport& report : Faction->Police.PoliceReports) {
		if (!report.Witnesses.Contains(Citizen) && report.RespondingOfficer != Citizen && !report.AcussesTeam1.Contains(Citizen) && !report.AcussesTeam2.Contains(Citizen))
			continue;

		return true;
	}

	return false;
}

void UPoliceManager::ChangeReportToMurder(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Team1.GetTeam().Contains(Citizen) && !report.Team2.GetTeam().Contains(Citizen))
			continue;

		report.Type = EReportType::Murder;
	}
}

void UPoliceManager::GetCloserToFight(ACitizen* Citizen, ACitizen* Target, FVector MidPoint)
{
	FVector location = MidPoint;
	location += FRotator(0.0f, Camera->Stream.RandRange(0, 360), 0.0f).Vector() * Camera->Stream.RandRange(100.0f, 400.0f);

	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation navLoc;
	nav->ProjectPointToNavigation(location, navLoc, FVector(400.0f, 400.0f, 200.0f));

	Citizen->AIController->AIMoveTo(Target, navLoc.Location);
}

void UPoliceManager::StopFighting(ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (!report.Team1.HasCitizen(Citizen) && !report.Team2.HasCitizen(Citizen))
			continue;

		for (ACitizen* citizen : report.Team2.GetTeam()) {
			report.Team1.Instigator->AttackComponent->OverlappingEnemies.Remove(citizen);

			for (ACitizen* assistor : report.Team1.Assistors)
				assistor->AttackComponent->OverlappingEnemies.Remove(citizen);
		}

		for (ACitizen* citizen : report.Team1.GetTeam()) {
			report.Team2.Instigator->AttackComponent->OverlappingEnemies.Remove(citizen);

			for (ACitizen* assistor : report.Team2.Assistors)
				assistor->AttackComponent->OverlappingEnemies.Remove(citizen);
		}

		break;
	}
}

void UPoliceManager::CeaseAllInternalFighting(FFactionStruct* Faction)
{
	for (ACitizen* citizen : Faction->Citizens) {
		if (citizen->AttackComponent->OverlappingEnemies.IsEmpty()) {
			citizen->AIController->DefaultAction();

			continue;
		}

		for (int32 i = citizen->AttackComponent->OverlappingEnemies.Num() - 1; i > -1; i--) {
			AActor* actor = citizen->AttackComponent->OverlappingEnemies[i];

			if (Faction->Citizens.Contains(actor))
				citizen->AttackComponent->OverlappingEnemies.RemoveAt(i);
		}
	}
}

void UPoliceManager::InterrogateWitnesses(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	for (FPoliceReport& report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		float distance = *report.Witnesses.Find(Citizen);

		float impartial = 50.0f + distance / 40.0f;
		float accuseTeam1 = 0.0f;
		float accuseTeam2 = 0.0f;

		for (ACitizen* accused : report.Team1.GetTeam())
			accuseTeam1 += 50.0f * Camera->CitizenManager->GetAggressiveness(accused);

		for (ACitizen* accused : report.Team1.GetTeam())
			accuseTeam2 += 50.0f * Camera->CitizenManager->GetAggressiveness(accused);

		if (report.Type == EReportType::Vandalism)
			accuseTeam1 += 100.0f;

		float tally = impartial + accuseTeam1 + accuseTeam2;

		int32 choice = Camera->Stream.FRandRange(0.0f, tally);

		if (choice <= impartial)
			report.Impartial.Add(Citizen);
		else if (choice <= impartial + accuseTeam1)
			report.AcussesTeam1.Add(Citizen);
		else
			report.AcussesTeam2.Add(Citizen);

		for (auto& element : report.Witnesses) {
			if (report.Impartial.Contains(element.Key) || report.AcussesTeam1.Contains(element.Key) || report.AcussesTeam2.Contains(element.Key))
				continue;

			Officer->AIController->AIMoveTo(element.Key);

			return;
		}

		if (report.Type == EReportType::Protest || ((report.Impartial.Num() < report.AcussesTeam1.Num() || report.Impartial.Num() < report.AcussesTeam2.Num()) && (report.AcussesTeam1.Num() > report.AcussesTeam2.Num() + 1 || report.AcussesTeam2.Num() > report.AcussesTeam1.Num() + 1)))
			GotoClosestWantedMan(Officer);

		for (auto& element : report.Witnesses)
			element.Key->AIController->DefaultAction();

		break;
	}
}

void UPoliceManager::GotoClosestWantedMan(ACitizen* Officer)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Officer);

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		TArray<ACitizen*> wanted;

		if (report.AcussesTeam1.Num() >= report.AcussesTeam2.Num())
			wanted = report.Team1.GetTeam();
		else
			wanted = report.Team2.GetTeam();

		ACitizen* target = nullptr;

		for (ACitizen* criminal : wanted) {
			if (faction->Police.Arrested.Contains(criminal))
				continue;

			if (!IsValid(target)) {
				target = criminal;

				continue;
			}

			double magnitude = Officer->AIController->GetClosestActor(Officer->Range, Camera->GetTargetActorLocation(Officer), Camera->GetTargetActorLocation(target), Camera->GetTargetActorLocation(criminal));

			if (magnitude < 0.0f)
				continue;

			target = criminal;
		}

		if (IsValid(target)) {
			Officer->AIController->AIMoveTo(target);
		}
		else {
			faction->Police.PoliceReports.Remove(report);

			Camera->Grid->AIVisualiser->ToggleOfficerLights(Officer, 0.0f);

			Officer->AIController->DefaultAction();
		}
	}
}

void UPoliceManager::Arrest(ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	Camera->DiseaseManager->Infectible.Remove(Citizen);

	if (Camera->DiseaseManager->Infected.Contains(Citizen) || Camera->DiseaseManager->Injured.Contains(Citizen))
		Camera->DiseaseManager->Cure(Citizen);

	Citizen->ClearCitizen();

	Officer->AIController->StopMovement();
	Citizen->AIController->StopMovement();

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		if (Camera->PoliticsManager->GetLawValue(faction->Name, law) == 0) {
			faction->Police.PoliceReports.Remove(report);

			Officer->AIController->DefaultAction();
			Citizen->AIController->DefaultAction();

			return;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(ArrestSystem, Citizen->GetRootComponent(), "", FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(*faction, params);
	Camera->TimerManager->SetParameter(Officer, params);
	Camera->TimerManager->SetParameter(Citizen, params);

	Camera->TimerManager->CreateTimer("Arrest", Citizen, 2.0f, "SetInNearestJail", params, false);
}

void UPoliceManager::SetInNearestJail(FFactionStruct Faction, ACitizen* Officer, ACitizen* Citizen)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	ABuilding* target = nullptr;

	for (ABuilding* building : faction->Buildings) {
		if (!building->IsA(PoliceStationClass))
			continue;

		if (!IsValid(target)) {
			target = building;

			continue;
		}

		double magnitude = Citizen->AIController->GetClosestActor(20.0f, Camera->GetTargetActorLocation(Citizen), target->GetActorLocation(), building->GetActorLocation());

		if (magnitude < 0.0f)
			continue;

		target = building;
	}

	Citizen->MovementComponent->Transform.SetLocation(target->GetActorLocation());

	Citizen->BuildingComponent->BuildingAt = target;

	Citizen->AIController->ChosenBuilding = target;
	Citizen->AIController->Idle(faction, Citizen);

	FPartyStruct* party = Camera->PoliticsManager->GetMembersParty(Citizen);

	if (party != nullptr) {
		if (party->Leader == Citizen)
			Camera->PoliticsManager->SelectNewLeader(party);

		party->Members.Remove(Citizen);
	}

	if (faction->Politics.Representatives.Contains(Citizen))
		faction->Politics.Representatives.Remove(Citizen);

	if (!IsValid(Officer))
		return;

	for (FPoliceReport report : faction->Police.PoliceReports) {
		if (report.RespondingOfficer != Officer)
			continue;

		FString law = "Fighting Length";

		if (report.Type == EReportType::Murder)
			law = "Murder Length";
		else if (report.Type == EReportType::Vandalism)
			law = "Vandalism Length";
		else if (report.Type == EReportType::Protest)
			law = "Protest Length";

		faction->Police.Arrested.Add(Citizen, Camera->PoliticsManager->GetLawValue(faction->Name, law));

		break;
	}

	GotoClosestWantedMan(Officer);
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
			Camera->DiseaseManager->Infectible.Add(citizen);

			citizen->MovementComponent->Transform.SetLocation(citizen->BuildingComponent->BuildingAt->BuildingMesh->GetSocketLocation("Entrance"));
		}
	}
}