#include "AI/Citizen/Components/HappinessComponent.h"

#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/House.h"
#include "Buildings/Work/Booster.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/EventsManager.h"

UHappinessComponent::UHappinessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MassStatus = EAttendStatus::Neutral;
	FestivalStatus = EAttendStatus::Neutral;

	ConversationHappiness = 0;
	FamilyDeathHappiness = 0;
	WitnessedDeathHappiness = 0;
	DivorceHappiness = 0;

	SadTimer = 0;
}

void UHappinessComponent::SetAttendStatus(EAttendStatus Status, bool bMass)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (bMass)
		MassStatus = Status;
	else
		FestivalStatus = Status;

	int32 timeToCompleteDay = citizen->Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	TArray<FTimerParameterStruct> params;
	citizen->Camera->TimerManager->SetParameter(EAttendStatus::Neutral, params);
	citizen->Camera->TimerManager->SetParameter(bMass, params);
	citizen->Camera->TimerManager->CreateTimer("Mass", citizen, timeToCompleteDay * 2, "SetAttendStatus", params, false);
}

void UHappinessComponent::SetDecayingHappiness(int32* HappinessToDecay, int32 Amount, int32 Min, int32 Max)
{
	int32 value = FMath::Clamp(*HappinessToDecay + Amount, Min, Max);

	HappinessToDecay = &value;
}

void UHappinessComponent::DecayHappiness()
{
	TArray<int32*> happinessToDecay = { &ConversationHappiness, &FamilyDeathHappiness, &WitnessedDeathHappiness, &DivorceHappiness };

	for (int32* happiness : happinessToDecay) {
		if (*happiness < 0)
			happiness++;
		else if (*happiness > 0)
			happiness--;
	}
}

int32 UHappinessComponent::GetHappiness()
{
	int32 value = 50;

	TArray<int32> values;
	Modifiers.GenerateValueArray(values);

	for (int32 v : values)
		value += v;

	value = FMath::Clamp(value, 0, 100);

	return value;
}

void UHappinessComponent::SetHappiness()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	Modifiers.Empty();

	if (faction->Police.Arrested.Contains(citizen)) {
		Modifiers.Add("Arrested", -50);

		return;
	}

	SetHousingHappiness(citizen, faction);
	SetWorkHappiness(citizen, faction);
	SetPoliticsHappiness(citizen, faction);
	SetSexualityHappiness(citizen, faction);

	if (citizen->Hunger < 20)
		Modifiers.Add("Hungry", -30);
	else if (citizen->Hunger > 70)
		Modifiers.Add("Well Fed", 10);

	if (citizen->Energy < 20)
		Modifiers.Add("Tired", -15);
	else if (citizen->Energy > 70)
		Modifiers.Add("Rested", 10);

	if (citizen->HoursSleptToday.Num() < citizen->IdealHoursSlept - 2)
		Modifiers.Add("Disturbed Sleep", -15);
	else if (citizen->HoursSleptToday.Num() >= citizen->IdealHoursSlept)
		Modifiers.Add("Slept Like A Baby", 10);

	if (MassStatus == EAttendStatus::Missed)
		Modifiers.Add("Missed Mass", -25);
	else if (MassStatus == EAttendStatus::Attended)
		Modifiers.Add("Attended Mass", 15);

	if (FestivalStatus == EAttendStatus::Missed)
		Modifiers.Add("Missed Festival", -5);
	else if (FestivalStatus == EAttendStatus::Attended)
		Modifiers.Add("Attended Festival", 15);

	if (citizen->Camera->EventsManager->IsHolliday(citizen))
		Modifiers.Add("Holliday", 10);

	if (ConversationHappiness < 0)
		Modifiers.Add("Recent arguments", ConversationHappiness);
	else if (ConversationHappiness > 0)
		Modifiers.Add("Recent conversations", ConversationHappiness);

	if (FamilyDeathHappiness != 0)
		Modifiers.Add("Recent family death", FamilyDeathHappiness);

	if (WitnessedDeathHappiness != 0)
		Modifiers.Add("Witnessed death", WitnessedDeathHappiness);

	if (DivorceHappiness != 0)
		Modifiers.Add("Recently divorced", DivorceHappiness);

	if (citizen->Camera->ConquestManager->GetCitizenFaction(citizen).WarFatigue >= 120)
		Modifiers.Add("High War Fatigue", -15);
}

void UHappinessComponent::SetHousingHappiness(ACitizen* Citizen, FFactionStruct* Faction)
{
	if (!IsValid(Citizen->BuildingComponent->House))
		Modifiers.Add("Homeless", -20);
	else {
		Modifiers.Add("Housed", 10);

		int32 satisfaction = Citizen->BuildingComponent->House->GetSatisfactionLevel(Citizen->BuildingComponent->House->GetAmount(Citizen));
		int32 level = (satisfaction / 5 - 10) * 2;

		if (level > 0)
			level *= 0.75f;

		FString message = "";

		if (satisfaction < 25)
			message = "Awful Housing Satisfaction";
		else if (satisfaction < 50)
			message = "Substandard Housing Satisfaction";
		else if (satisfaction > 50)
			message = "Good Housing Satisfaction";
		else if (satisfaction > 75)
			message = "Very High Housing Satisfaction";

		Modifiers.Add(message, level);

		SetBoosterHappiness(Citizen, Faction);
	}
}

void UHappinessComponent::SetBoosterHappiness(ACitizen* Citizen, FFactionStruct* Faction)
{
	bool bIsCruel = false;

	for (FPersonality* personality : Citizen->Camera->CitizenManager->GetCitizensPersonalities(Citizen))
		if (personality->Trait == "Cruel")
			bIsCruel = true;

	bool isThereAnyBoosters = false;
	bool bNearbyFaith = false;
	bool bPropaganda = true;
	bool bIsPark = false;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<ABooster>())
			continue;

		ABooster* booster = Cast<ABooster>(building);

		if (!booster->GetAffectedBuildings().Contains(Citizen->BuildingComponent->House))
			continue;

		isThereAnyBoosters = true;

		if (booster->DoesPromoteFavouringValues(Citizen))
			bPropaganda = false;

		for (auto& element : booster->BuildingsToBoost) {

			if (element.Value == Citizen->Spirituality.Faith && booster->bHolyPlace)
				bNearbyFaith = true;

			if (element.Value == "Happiness")
				bIsPark = true;
		}
	}

	if (Citizen->Spirituality.Faith != "Atheist") {
		if (!bNearbyFaith)
			Modifiers.Add("No Working Holy Place Nearby", -25);
		else
			Modifiers.Add("Holy Place Nearby", 15);
	}

	if (bIsPark) {
		if (bIsCruel)
			Modifiers.Add("Park Nearby", -5);
		else
			Modifiers.Add("Park Nearby", 5);
	}
	else if (!bIsCruel)
		Modifiers.Add("No Nearby Park", -10);

	if (isThereAnyBoosters) {
		if (bPropaganda)
			Modifiers.Add("Nearby propaganda tower", -25);
		else
			Modifiers.Add("Nearby eggtastic tower", 15);
	}
}

void UHappinessComponent::SetWorkHappiness(ACitizen* Citizen, FFactionStruct* Faction)
{
	if (Citizen->BioComponent->Age < Citizen->Camera->PoliticsManager->GetLawValue(Faction->Name, "Work Age"))
		return;

	if (Citizen->BuildingComponent->Employment == nullptr)
		Modifiers.Add("Unemployed", -10);
	else
		Modifiers.Add("Employed", 5);

	int32 hours = Citizen->BuildingComponent->HoursWorked.Num();

	if (hours < Citizen->BuildingComponent->IdealHoursWorkedMin || hours > Citizen->BuildingComponent->IdealHoursWorkedMax)
		Modifiers.Add("Inadequate Hours Worked", -15);
	else
		Modifiers.Add("Ideal Hours Worked", 10);

	if (Citizen->Balance < 5)
		Modifiers.Add("Poor", -25);
	else if (Citizen->Balance >= 5 && Citizen->Balance < 15)
		Modifiers.Add("Well Off", 10);
	else
		Modifiers.Add("Rich", 20);

	if (!IsValid(Citizen->BuildingComponent->Employment))
		return;

	int32 wagePerHour = Citizen->BuildingComponent->Employment->GetAmount(Citizen);

	for (ACitizen* colleague : Citizen->BuildingComponent->Employment->GetOccupied()) {
		if (colleague == Citizen)
			continue;

		int32 wPH = colleague->BuildingComponent->Employment->GetAmount(colleague);

		if (wPH <= wagePerHour)
			continue;

		Modifiers.Add("Unequal pay", -10);

		break;
	}

	if (GetWorld()->GetTimeSeconds() < Citizen->BuildingComponent->GetAcquiredTime(1) + 300.0f)
		return;

	int32 count = 0;

	for (ACitizen* citizen : Citizen->BuildingComponent->Employment->GetOccupied()) {
		if (citizen == Citizen)
			continue;

		for (FPersonality* personality : Citizen->Camera->CitizenManager->GetCitizensPersonalities(Citizen)) {
			for (FPersonality* p : Citizen->Camera->CitizenManager->GetCitizensPersonalities(citizen)) {
				if (personality->Trait == p->Trait)
					count += 2;
				else if (personality->Likes.Contains(p->Trait))
					count++;
				else if (personality->Dislikes.Contains(p->Trait))
					count--;
			}
		}
	}

	Modifiers.Add("Work Happiness", count * 5);
}

void UHappinessComponent::SetPoliticsHappiness(ACitizen* Citizen, FFactionStruct* Faction)
{
	FString party = Citizen->Camera->PoliticsManager->GetCitizenParty(Citizen);

	if (party == "Undecided")
		return;

	int32 lawTally = 0;

	for (FLawStruct law : Faction->Politics.Laws) {
		if (law.BillType == "Abolish")
			continue;

		int32 count = 0;

		FLeanStruct partyLean;
		partyLean.Party = party;

		int32 index = law.Lean.Find(partyLean);

		if (index == INDEX_NONE)
			continue;

		if (!law.Lean[index].ForRange.IsEmpty() && Citizen->Camera->PoliticsManager->IsInRange(law.Lean[index].ForRange, law.Value))
			count++;
		else if (!law.Lean[index].AgainstRange.IsEmpty() && Citizen->Camera->PoliticsManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
			count--;

		for (FPersonality* personality : Citizen->Camera->CitizenManager->GetCitizensPersonalities(Citizen)) {
			FLeanStruct personalityLean;
			personalityLean.Personality = personality->Trait;

			index = law.Lean.Find(personalityLean);

			if (index == INDEX_NONE)
				continue;

			if (!law.Lean[index].ForRange.IsEmpty() && Citizen->Camera->PoliticsManager->IsInRange(law.Lean[index].ForRange, law.Value))
				count++;
			else if (!law.Lean[index].AgainstRange.IsEmpty() && Citizen->Camera->PoliticsManager->IsInRange(law.Lean[index].AgainstRange, law.Value))
				count--;
		}

		if (count < 0)
			lawTally--;
		else
			lawTally++;
	}

	if (lawTally < -1)
		Modifiers.Add("Not Represented", -20);
	else if (lawTally > 1)
		Modifiers.Add("Represented", 15);
}

void UHappinessComponent::SetSexualityHappiness(ACitizen* Citizen, FFactionStruct* Faction)
{
	if (Citizen->BioComponent->Sexuality == ESexuality::Straight)
		return;

	int32 lawValue = Citizen->Camera->PoliticsManager->GetLawValue(Faction->Name, "Same-Sex Laws");
	int32 sameSexHappiness = -20 + 10 * (lawValue + FMath::Floor(lawValue / 2));

	if (Citizen->BioComponent->Sexuality == ESexuality::Bisexual) {
		if (sameSexHappiness < 0)
			sameSexHappiness += 5;
		else
			sameSexHappiness -= 5;
	}

	Modifiers.Add("Same-Sex Laws", sameSexHappiness);
}

void UHappinessComponent::CheckSadnessTimer(ACitizen* Citizen, FFactionStruct* Faction)
{
	if (GetHappiness() < 35 && !Faction->Police.Arrested.Contains(Citizen)) {
		if (SadTimer == 0)
			Citizen->Camera->NotifyLog("Bad", Citizen->BioComponent->Name + " is sad", Faction->Name);

		SadTimer++;
	}
	else {
		SadTimer = 0;
	}

	if (SadTimer < 300 || Citizen->Camera->EventsManager->UpcomingProtest(Faction))
		return;

	int32 startHour = Citizen->Camera->Stream.RandRange(6, 9);
	int32 endHour = Citizen->Camera->Stream.RandRange(12, 18);

	TArray<int32> hours;
	for (int32 i = startHour; i < endHour; i++)
		hours.Add(i);

	Citizen->Camera->EventsManager->CreateEvent(Faction->Name, EEventType::Protest, nullptr, nullptr, "", 0, hours, false, {});
}