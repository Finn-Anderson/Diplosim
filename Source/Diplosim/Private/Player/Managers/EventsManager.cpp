#include "Player/Managers/EventsManager.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"

#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Building.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Misc/Festival.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/PoliticsManager.h"

UEventsManager::UEventsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEventsManager::CreateEvent(FString FactionName, EEventType Type, TSubclassOf<class ABuilding> Building, class ABuilding* Venue, FString Period, int32 Day, TArray<int32> Hours, bool bRecurring, TArray<ACitizen*> Whitelist, bool bFireFestival)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	FEventStruct event;
	event.Type = Type;
	event.Venue = Venue;
	event.Period = Period;
	event.Day = Day;
	event.Hours = Hours;
	event.bRecurring = bRecurring;
	event.bFireFestival = bFireFestival;
	event.Building = Building;
	event.Whitelist = Whitelist;

	if (event.Type == EEventType::Protest) {
		if (faction->Buildings.IsEmpty())
			return;

		ABuilding* building = faction->Buildings[Camera->Stream.RandRange(0, faction->Buildings.Num() - 1)];

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		FNavLocation location;
		nav->GetRandomPointInNavigableRadius(building->GetActorLocation(), 400, location);

		event.Location = location;
	}

	faction->Events.Add(event);
}

void UEventsManager::ExecuteEvent(FString Period, int32 Day, int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (FEventStruct& event : faction.Events) {
			FString command = "";

			if (event.Period != "" && event.Day != 0 && event.Period != Period && event.Day != Day)
				continue;

			if (event.Hours.Contains(Hour) && !event.bStarted)
				command = "start";
			else if (event.Hours.Contains(Hour) && event.bStarted)
				command = "end";

			event.Hours.Remove(Hour);

			if (command == "")
				continue;

			if (command == "start")
				StartEvent(&faction, &event, Hour);
			else
				EndEvent(&faction, &event, Hour);
		}
	}
}

bool UEventsManager::IsAttendingEvent(ACitizen* Citizen)
{
	if (Camera->ArmyManager->IsCitizenInAnArmy(Citizen))
		return true;

	for (auto& element : OngoingEvents())
		for (FEventStruct* event : element.Value)
			if (event->Attendees.Contains(Citizen))
				return true;

	return false;
}

bool UEventsManager::IsHolliday(ACitizen* Citizen)
{
	for (auto& element : OngoingEvents())
		for (FEventStruct* event : element.Value)
			if (event->Type == EEventType::Holliday || event->Type == EEventType::Festival)
				return true;

	return false;
}

void UEventsManager::RemoveFromEvent(ACitizen* Citizen)
{
	for (auto& element : OngoingEvents()) {
		for (FEventStruct* event : element.Value) {
			if (!event->Attendees.Contains(Citizen))
				continue;

			event->Attendees.Remove(Citizen);

			return;
		}
	}
}

TMap<FFactionStruct*, TArray<FEventStruct*>> UEventsManager::OngoingEvents()
{
	TMap<FFactionStruct*, TArray<FEventStruct*>> factionEvents;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		for (FEventStruct& event : faction.Events) {
			UAtmosphereComponent* atmosphere = Camera->Grid->AtmosphereComponent;

			if (!event.bStarted)
				continue;

			if (factionEvents.Contains(&faction)) {
				TArray<FEventStruct*>* events = factionEvents.Find(&faction);
				events->Add(&event);
			}
			else
				factionEvents.Add(&faction, { &event });
		}
	}

	return factionEvents;
}

void UEventsManager::GotoEvent(ACitizen* Citizen, FEventStruct* Event, FFactionStruct* Faction)
{
	if (IsAttendingEvent(Citizen) || Citizen->bSleep || (Event->Type != EEventType::Protest && IsValid(Citizen->BuildingComponent->Employment) && !Citizen->BuildingComponent->Employment->bCanAttendEvents && Citizen->BuildingComponent->Employment->IsWorking(Citizen)))
		return;

	if (Event->Type == EEventType::Protest) {
		if (Citizen->HappinessComponent->GetHappiness() >= 35)
			return;

		UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		const ANavigationData* navData = nav->GetDefaultNavDataInstance();

		UNavigationPath* path = nav->FindPathToLocationSynchronously(GetWorld(), Camera->GetTargetActorLocation(Citizen), Event->Location, Citizen, Citizen->NavQueryFilter);

		Citizen->MovementComponent->SetPoints(path->PathPoints);

		return;
	}
	else if (Event->Type == EEventType::Mass) {
		if (IsValid(Citizen->BuildingComponent->Employment) && Citizen->BuildingComponent->Employment->IsA(Event->Building)) {
			if (Citizen->BuildingComponent->BuildingAt != Citizen->BuildingComponent->Employment)
				Citizen->AIController->AIMoveTo(Citizen->BuildingComponent->Employment);

			Event->Attendees.Add(Citizen);

			return;
		}

		ABooster* church = Cast<ABooster>(Event->Building->GetDefaultObject());

		bool validReligion = church->DoesPromoteFavouringValues(Citizen);

		if (Citizen->BioComponent->Age < 18 && (Citizen->BioComponent->Mother != nullptr && church->DoesPromoteFavouringValues(Citizen->BioComponent->Mother.Get()) || Citizen->BioComponent->Father != nullptr && church->DoesPromoteFavouringValues(Citizen->BioComponent->Father.Get())))
			validReligion = true;

		if (!validReligion)
			return;
	}

	ABuilding* chosenBuilding = nullptr;
	ACitizen* chosenOccupant = nullptr;

	TArray<AActor*> actors;

	if (IsValid(Event->Venue))
		actors.Add(Event->Venue);
	else if (IsValid(Event->Building)) {
		if (Faction == nullptr)
			Faction = Camera->ConquestManager->GetFaction("", Citizen);

		for (ABuilding* building : Faction->Buildings)
			if (building->IsA(Event->Building))
				actors.Add(building);
	} 

	for (AActor* actor : actors) {
		ABuilding* building = Cast<ABuilding>(actor);
		ACitizen* occupant = nullptr;

		if (!Citizen->AIController->CanMoveTo(building->GetActorLocation()) || (Event->Type == EEventType::Mass && building->GetOccupied().IsEmpty()) || (Event->Type == EEventType::Festival && !Cast<AFestival>(building)->bCanHostFestival))
			continue;

		bool bSpace = false;

		for (ACitizen* occpnt : building->GetOccupied()) {
			if (building->GetVisitors(occpnt).Num() == building->Space)
				continue;

			occupant = occpnt;
			bSpace = true;

			break;
		}

		if (!bSpace)
			continue;

		if (chosenBuilding == nullptr) {
			chosenBuilding = building;
			chosenOccupant = occupant;

			continue;
		}

		double magnitude = Citizen->AIController->GetClosestActor(400.0f, Camera->GetTargetActorLocation(Citizen), chosenBuilding->GetActorLocation(), building->GetActorLocation());

		if (magnitude <= 0.0f)
			continue;

		chosenBuilding = building;
		chosenOccupant = occupant;
	}

	if (chosenBuilding != nullptr) {
		Event->Attendees.Add(Citizen);

		chosenBuilding->AddVisitor(chosenOccupant, Citizen);

		Citizen->AIController->AIMoveTo(chosenBuilding);
	}
}

void UEventsManager::StartEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour)
{
	Event->bStarted = true;

	if (Event->Type != EEventType::Protest) {
		for (ABuilding* building : Faction->Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
			else if (building->IsA<AFestival>() && Event->Type == EEventType::Festival)
				Cast<AFestival>(building)->StartFestival(Event->bFireFestival);
		}
	}

	TArray<ACitizen*> citizens = Faction->Citizens;
	if (!Event->Whitelist.IsEmpty())
		citizens = Event->Whitelist;

	for (ACitizen* citizen : citizens)
		GotoEvent(citizen, Event, Faction);

	if (Event->Type == EEventType::Protest && Camera->PoliticsManager->GetLawValue(Faction->Name, "Protest Length") > 0) {
		FPoliceReport report;
		report.Type = EReportType::Protest;
		report.Team1.Instigator = Event->Attendees[0];
		report.Team1.Assistors = Event->Attendees;
		report.Team1.Assistors.RemoveAt(0);

		Faction->Police.PoliceReports.Add(report);
	}

	Camera->ShowEvent("Event", EnumToString<EEventType>(Event->Type));
}

void UEventsManager::EndEvent(FFactionStruct* Faction, FEventStruct* Event, int32 Hour)
{
	Event->Attendees.Empty();
	Event->bStarted = false;

	if (Event->Type == EEventType::Holliday || Event->Type == EEventType::Festival) {
		for (ABuilding* building : Faction->Buildings) {
			if (building->IsA<AWork>())
				Cast<AWork>(building)->CheckWorkStatus(Hour);
			else if (building->IsA<AFestival>() && Event->Type == EEventType::Festival)
				Cast<AFestival>(building)->StopFestival();
		}
	}

	for (ACitizen* citizen : Faction->Citizens) {
		if (Event->Type == EEventType::Festival) {
			if (IsValid(citizen->BuildingComponent->BuildingAt) && citizen->BuildingComponent->BuildingAt->IsA(Event->Building))
				citizen->HappinessComponent->SetAttendStatus(EAttendStatus::Attended, false);
			else
				citizen->HappinessComponent->SetAttendStatus(EAttendStatus::Missed, false);
		}
		else if (Event->Type == EEventType::Mass) {
			ABooster* church = Cast<ABooster>(Event->Building->GetDefaultObject());

			TArray<FString> faiths;
			church->BuildingsToBoost.GenerateValueArray(faiths);

			if (faiths[0] != citizen->Spirituality.Faith)
				continue;

			if (citizen->bWorshipping)
				citizen->HappinessComponent->SetAttendStatus(EAttendStatus::Attended, true);
			else
				citizen->HappinessComponent->SetAttendStatus(EAttendStatus::Missed, true);
		}

		if (!IsValid(Event->Building))
			citizen->AIController->DefaultAction();
	}

	if (!Event->bRecurring && Event->Hours.IsEmpty())
		Faction->Events.Remove(*Event);
}

bool UEventsManager::UpcomingProtest(FFactionStruct* Faction)
{
	for (FEventStruct& event : Faction->Events) {
		if (event.Type != EEventType::Protest)
			continue;

		return true;
	}

	return false;
}

void UEventsManager::CreateWedding(int32 Hour)
{
	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		TArray<ACitizen*> citizens = faction.Citizens;
		TArray<ACitizen*> checked;

		for (int32 i = citizens.Num() - 1; i > -1; i--) {
			ACitizen* citizen = citizens[i];
			int32 lawValue = Camera->PoliticsManager->GetLawValue(faction.Name, "Same-Sex Laws");

			if (citizen->BioComponent->Partner == nullptr || citizen->BioComponent->bMarried || checked.Contains(citizen) || (lawValue != 2 && citizen->BioComponent->Sex == citizen->BioComponent->Partner->BioComponent->Sex))
				continue;

			ACitizen* partner = citizen->BioComponent->Partner.Get();

			checked.Add(citizen);
			checked.Add(partner);

			if (IsAttendingEvent(citizen) || IsAttendingEvent(partner))
				continue;

			TArray<FPersonality*> personalities = Camera->CitizenManager->GetCitizensPersonalities(citizen);
			personalities.Append(Camera->CitizenManager->GetCitizensPersonalities(partner));

			int32 likelihood = 0;

			for (FPersonality* personality : personalities)
				if (personality->Affects.Contains("Marriage"))
					likelihood += *personality->Affects.Find("Marriage");

			int32 chance = Camera->Stream.RandRange(80, 150);
			int32 likelihoodChance = likelihood * 10 + citizen->BioComponent->HoursTogetherWithPartner * 5;

			if (likelihoodChance < chance)
				continue;

			TArray<FString> faiths;

			if (citizen->Spirituality.Faith != "Atheist")
				faiths.Add(citizen->Spirituality.Faith);

			if (partner->Spirituality.Faith != "Atheist")
				faiths.Add(partner->Spirituality.Faith);

			if (faiths.IsEmpty())
				faiths.Append({ "Chicken", "Egg" });

			int32 index = Camera->Stream.RandRange(0, faiths.Num() - 1);
			FString chosenFaith = faiths[index];

			ABooster* chosenChurch = nullptr;
			ACitizen* priest = nullptr;

			TArray<int32> weddingHours = { Hour };
			for (int32 j = 0; j < 2; j++) {
				int32 hour = Hour + j;

				if (hour > 23)
					hour -= 24;

				weddingHours.Add(hour);
			}

			for (ABuilding* building : faction.Buildings) {
				if (!building->IsA<ABooster>())
					continue;

				ABooster* church = Cast<ABooster>(building);

				church->BuildingsToBoost.GenerateValueArray(faiths);

				if (!church->bHolyPlace || faiths[0] != chosenFaith || church->GetOccupied().IsEmpty())
					continue;

				bool bAvailable = false;

				for (ACitizen* p : church->GetOccupied()) {
					if (IsAttendingEvent(p))
						continue;

					bool bWithinHours = true;
					for (int32 hour : weddingHours)
						if (!church->IsWorking(p, hour))
							bWithinHours = false;

					if (!bWithinHours)
						continue;

					bAvailable = true;
					priest = p;

					break;
				}

				if (!bAvailable)
					continue;

				if (chosenChurch == nullptr) {
					chosenChurch = church;

					continue;
				}

				double magnitude = citizen->AIController->GetClosestActor(400.0f, Camera->GetTargetActorLocation(citizen), chosenChurch->GetActorLocation(), church->GetActorLocation());

				if (magnitude <= 0.0f)
					continue;

				chosenChurch = church;
			}

			if (!IsValid(chosenChurch))
				continue;

			for (FEventStruct event : faction.Events) {
				bool bContainsWeddingHour = false;

				for (int32 hour : weddingHours) {
					if (!event.Hours.Contains(hour))
						continue;

					bContainsWeddingHour = true;

					break;
				}

				if (!bContainsWeddingHour)
					continue;

				if (chosenChurch->IsA(event.Building))
					return;
			}

			TArray<ACitizen*> whitelist = { citizen, partner, priest };
			whitelist.Append(citizen->BioComponent->GetLikedFamily(false));
			whitelist.Append(partner->BioComponent->GetLikedFamily(false));

			FCalendarStruct calendar = Camera->Grid->AtmosphereComponent->Calendar;

			CreateEvent(faction.Name, EEventType::Marriage, nullptr, chosenChurch, "", calendar.Days[calendar.Index], weddingHours, false, whitelist);
		}
	}
}