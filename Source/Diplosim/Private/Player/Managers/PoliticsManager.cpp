#include "Player/Managers/PoliticsManager.h"

#include "Components/WidgetComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/House.h"
#include "Buildings/Work/Work.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/School.h"
#include "Buildings/Misc/Special/CloneLab.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/PoliceManager.h"
#include "Universal/DiplosimGameModeBase.h"

UPoliticsManager::UPoliticsManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	AIProposeTimer = 0;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Parties.json");

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Laws.json");
}

void UPoliticsManager::ReadJSONFile(FString path)
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	FString fileContents;
	FFileHelper::LoadFileToString(fileContents, *path);
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

	if (!FJsonSerializer::Deserialize(jsonReader, jsonObject) || !jsonObject.IsValid())
		return;

	for (auto& element : jsonObject->Values) {
		for (auto& e : element.Value->AsArray()) {
			FPartyStruct party;
			FLawStruct law;

			for (auto& v : e->AsObject()->Values) {
				uint8 index = 0;

				if (v.Value->Type == EJson::Object) {
					for (auto& bv : v.Value->AsObject()->Values) {
						if (bv.Value->Type == EJson::Array) {
							for (auto& bev : bv.Value->AsArray()) {
								if (bv.Key == "Descriptions") {
									FDescriptionStruct description;

									for (auto& pv : bev->AsObject()->Values) {
										if (pv.Key == "Description")
											description.Description = pv.Value->AsString();
										else if (pv.Key == "Min")
											description.Min = FCString::Atoi(*pv.Value->AsString());
										else
											description.Max = FCString::Atoi(*pv.Value->AsString());
									}

									law.Description.Add(description);
								}
								else {
									FLeanStruct lean;

									for (auto& pv : bev->AsObject()->Values) {
										if (pv.Key == "Party")
											lean.Party = pv.Value->AsString();
										else if (pv.Key == "Personality")
											lean.Personality = pv.Value->AsString();
										else {
											for (auto& pev : pv.Value->AsArray()) {
												int32 value = FCString::Atoi(*pev->AsString());

												if (pv.Key == "ForRange")
													lean.ForRange.Add(value);
												else
													lean.AgainstRange.Add(value);
											}
										}
									}

									law.Lean.Add(lean);
								}
							}
						}
						else if (bv.Value->Type == EJson::Number) {
							index = FCString::Atoi(*bv.Value->AsString());

							law.Value = index;
						}
						else if (bv.Value->Type == EJson::String) {
							law.Warning = bv.Value->AsString();
						}
					}
				}
				else if (v.Value->Type == EJson::Array) {
					for (auto& ev : v.Value->AsArray())
						party.Agreeable.Add(ev->AsString());
				}
				else if (v.Value->Type == EJson::String) {
					FString value = v.Value->AsString();

					if (element.Key == "Parties")
						party.Party = value;
					else
						law.BillType = value;
				}
			}

			if (element.Key == "Parties")
				InitParties.Add(party);
			else
				InitLaws.Add(law);
		}
	}
}

void UPoliticsManager::PoliticsLoop(FFactionStruct* Faction)
{
	for (FPartyStruct& party : Faction->Politics.Parties) {
		if (party.Leader != nullptr)
			continue;

		SelectNewLeader(&party);
	}

	for (FLawStruct& law : Faction->Politics.Laws) {
		if (law.Cooldown == 0)
			continue;

		law.Cooldown--;
	}

	AIProposeBill(Faction);
}

void UPoliticsManager::AIProposeBill(FFactionStruct* Faction)
{
	if (Faction->Politics.Representatives.IsEmpty())
		return;

	AIProposeTimer++;

	if (AIProposeTimer < Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay())
		return;

	ACitizen* proposer = Faction->Politics.Representatives[Camera->Stream.RandRange(0, Faction->Politics.Representatives.Num() - 1)];
	FPartyStruct* party = GetMembersParty(proposer);
	
	TArray<FLawStruct> changeableLaws;

	for (FLawStruct law : Faction->Politics.Laws) {
		if (law.Cooldown != 0)
			continue;

		FLeanStruct lean;
		lean.Party = party->Party;
		
		int32 index = law.Lean.Find(lean);
		
		if (index == INDEX_NONE)
			continue;

		law.Value = Camera->Stream.RandRange(law.Lean[index].ForRange[0], law.Lean[index].ForRange[1]);
	}

	if (changeableLaws.IsEmpty())
		return;

	FLawStruct lawToPropose = changeableLaws[Camera->Stream.RandRange(0, changeableLaws.Num() - 1)];
	ProposeBill(Faction->Name, lawToPropose);
}

FPartyStruct* UPoliticsManager::GetMembersParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = nullptr;
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Citizen);

	if (faction != nullptr) {
		for (FPartyStruct& party : faction->Politics.Parties) {
			if (!party.Members.Contains(Citizen))
				continue;

			partyStruct = &party;

			break;
		}
	}

	return partyStruct;
}

FString UPoliticsManager::GetCitizenParty(ACitizen* Citizen)
{
	FPartyStruct* partyStruct = GetMembersParty(Citizen);

	if (partyStruct == nullptr)
		return "Undecided";

	return partyStruct->Party;
}

FPartyStruct UPoliticsManager::GetPartyFromName(FString FactionName, FString PartyName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	for (FPartyStruct& party : faction->Politics.Parties) {
		if (party.Party != PartyName)
			continue;

		return party;
	}

	UE_LOGFMT(LogTemp, Fatal, "Party not found");
}

void UPoliticsManager::SelectNewLeader(FPartyStruct* Party)
{
	TArray<ACitizen*> candidates;

	for (auto& element : Party->Members) {
		if (!IsValid(element.Key) || element.Key->bHasBeenLeader)
			continue;

		if (candidates.Num() < 3)
			candidates.Add(element.Key);
		else {
			ACitizen* lowest = nullptr;

			for (ACitizen* candidate : candidates)
				if (lowest == nullptr || Party->Members.Find(lowest) > Party->Members.Find(candidate))
					lowest = candidate;

			if (Party->Members.Find(element.Key) > Party->Members.Find(lowest)) {
				candidates.Remove(lowest);

				candidates.Add(element.Key);
			}
		}
	}

	if (candidates.IsEmpty())
		return;

	auto value = Async(EAsyncExecution::TaskGraph, [this, candidates]() { return Camera->Stream.RandRange(0, candidates.Num() - 1); });

	ACitizen* chosen = candidates[value.Get()];

	Party->Leader = chosen;

	chosen->bHasBeenLeader = true;
	Party->Members.Emplace(chosen, ESway::Radical);

	if (Camera->InfoUIInstance->IsInViewport())
		Async(EAsyncExecution::TaskGraphMainTick, [this, Party]() { Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Party, Party->Party); });
}

void UPoliticsManager::StartElectionTimer(FFactionStruct* Faction)
{
	Camera->TimerManager->RemoveTimer(Faction->Name + " Election", GetOwner());

	int32 timeToCompleteDay = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(*Faction, params);

	Camera->TimerManager->CreateTimer(Faction->Name + " Election", Camera, timeToCompleteDay * GetLawValue(Faction->Name, "Election Timer"), "Election", params, false);
}

void UPoliticsManager::Election(FFactionStruct Faction)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

	faction->Politics.Representatives.Empty();

	TMap<FString, TArray<ACitizen*>> tally;

	int32 representativeType = GetLawValue(faction->Name, "Representative Type");
	int32 representatives = GetLawValue(faction->Name, "Representatives");

	for (FPartyStruct party : faction->Politics.Parties) {
		TArray<ACitizen*> citizens;

		for (TPair<ACitizen*, TEnumAsByte<ESway>> pair : party.Members) {
			if ((representativeType == 1 && pair.Key->BuildingComponent->Employment == nullptr) || (representativeType == 2 && pair.Key->Balance < 15))
				continue;

			citizens.Add(pair.Key);
		}

		tally.Add(party.Party, citizens);
	}

	for (TPair<FString, TArray<ACitizen*>>& pair : tally) {
		int32 number = FMath::RoundHalfFromZero(pair.Value.Num() / (float)faction->Citizens.Num() * 100.0f / representatives);

		if (number == 0 || faction->Politics.Representatives.Num() == representatives)
			continue;

		number -= 1;

		FPartyStruct partyStruct;
		partyStruct.Party = pair.Key;

		int32 index = faction->Politics.Parties.Find(partyStruct);

		FPartyStruct* party = &faction->Politics.Parties[index];

		faction->Politics.Representatives.Add(party->Leader);

		pair.Value.Remove(party->Leader);

		for (int32 i = 0; i < number; i++) {
			if (pair.Value.IsEmpty())
				continue;

			auto value = Async(EAsyncExecution::TaskGraph, [this, pair]() { return Camera->Stream.RandRange(0, pair.Value.Num() - 1); });

			ACitizen* citizen = pair.Value[value.Get()];

			faction->Politics.Representatives.Add(citizen);

			pair.Value.Remove(citizen);

			if (faction->Politics.Representatives.Num() == representatives)
				break;
		}
	}

	for (FPartyStruct party : faction->Politics.Parties)
		if (Camera->InfoUIInstance->IsInViewport())
			Camera->UpdateCitizenInfoDisplay(EInfoUpdate::Party, party.Party);

	StartElectionTimer(faction);
	SetupBill(faction);
}

void UPoliticsManager::Bribe(class ACitizen* Representative, bool bAgree)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", Representative);

	if (faction->Politics.BribeValue.IsEmpty())
		return;

	int32 index = faction->Politics.Representatives.Find(Representative);

	int32 bribe = faction->Politics.BribeValue[index];

	bool bPass = Camera->ResourceManager->TakeUniversalResource(faction, Camera->ResourceManager->Money, bribe, 0);

	if (!bPass) {
		Camera->ShowWarning("Cannot afford");

		return;
	}

	if (faction->Politics.Votes.For.Contains(Representative))
		faction->Politics.Votes.For.Remove(Representative);
	else if (faction->Politics.Votes.Against.Contains(Representative))
		faction->Politics.Votes.For.Remove(Representative);

	Representative->Balance += bribe;

	if (bAgree)
		faction->Politics.Votes.For.Add(Representative);
	else
		faction->Politics.Votes.Against.Add(Representative);
}

void UPoliticsManager::ProposeBill(FString FactionName, FLawStruct Bill)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 index = faction->Politics.Laws.Find(Bill);

	if (faction->Politics.Laws[index].Cooldown != 0) {
		FString string = "You must wait " + faction->Politics.Laws[index].Cooldown;

		Camera->ShowWarning(string + " seconds");

		return;
	}

	faction->Politics.ProposedBills.Add(Bill);

	faction->Politics.Laws[index].Cooldown = Camera->Grid->AtmosphereComponent->GetTimeToCompleteDay();

	Camera->DisplayNewBill();

	if (faction->Politics.ProposedBills.Num() > 1)
		return;

	SetupBill(faction);
}

void UPoliticsManager::SetElectionBillLeans(FFactionStruct* Faction, FLawStruct* Bill)
{
	if (Bill->BillType != "Election")
		return;

	for (FPartyStruct party : Faction->Politics.Parties) {
		int32 representativeCount = 0;

		for (ACitizen* citizen : Faction->Politics.Representatives)
			if (party.Members.Contains(citizen))
				representativeCount++;

		float representPerc = representativeCount / Faction->Citizens.Num() * 100.0f;
		float partyPerc = party.Members.Num() / Faction->Citizens.Num() * 100.0f;

		FLeanStruct lean;
		lean.Party = party.Party;

		if (partyPerc > representPerc)
			lean.ForRange.Append({ 0, 0 });
		else if (representPerc > partyPerc)
			lean.AgainstRange.Append({ 0, 0 });

		Bill->Lean.Add(lean);
	}
}

void UPoliticsManager::SetupBill(FFactionStruct* Faction)
{
	Faction->Politics.Votes.Clear();

	Faction->Politics.BribeValue.Empty();

	if (Faction->Politics.ProposedBills.IsEmpty())
		return;

	SetElectionBillLeans(Faction, &Faction->Politics.ProposedBills[0]);

	for (ACitizen* citizen : Faction->Politics.Representatives)
		GetVerdict(Faction, citizen, Faction->Politics.ProposedBills[0], true, false);

	for (ACitizen* citizen : Faction->Politics.Representatives) {
		int32 bribe = Async(EAsyncExecution::TaskGraph, [this]() { return Camera->Stream.RandRange(2, 20); }).Get();

		if (Faction->Politics.Votes.For.Contains(citizen) || Faction->Politics.Votes.Against.Contains(citizen))
			bribe *= 4;

		bribe *= (uint8)*GetMembersParty(citizen)->Members.Find(citizen);

		Faction->Politics.BribeValue.Add(bribe);
	}

	AIProposeTimer = 0;

	TArray<FTimerParameterStruct> params;
	Camera->TimerManager->SetParameter(*Faction, params);
	Camera->TimerManager->SetParameter(Faction->Politics.ProposedBills[0], params);

	Camera->TimerManager->CreateTimer(Faction->Name + " Bill", Camera, 60, "MotionBill", params, false);
}

void UPoliticsManager::MotionBill(FFactionStruct Faction, FLawStruct Bill)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Faction, Bill]() {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(Faction.Name);

		int32 count = 1;

		for (ACitizen* citizen : faction->Politics.Representatives) {
			if (faction->Politics.Votes.For.Contains(citizen) || faction->Politics.Votes.Against.Contains(citizen))
				continue;

			FTimerHandle verdictTimer;
			GetWorld()->GetTimerManager().SetTimer(verdictTimer, FTimerDelegate::CreateUObject(this, &UPoliticsManager::GetVerdict, faction, citizen, Bill, false, false), 0.1f * count, false);

			count++;
		}

		FTimerHandle tallyTimer;
		GetWorld()->GetTimerManager().SetTimer(tallyTimer, FTimerDelegate::CreateUObject(this, &UPoliticsManager::TallyVotes, faction, Bill), 0.1f * count + 0.1f, false);
		});
}

bool UPoliticsManager::IsInRange(TArray<int32> Range, int32 Value)
{
	if (Value == 255 || (Range[0] > Range[1] && Value >= Range[0] || Value <= Range[1]) || (Range[0] <= Range[1] && Value >= Range[0] && Value <= Range[1]))
		return true;

	return false;
}

void UPoliticsManager::GetVerdict(FFactionStruct* Faction, ACitizen* Representative, FLawStruct Bill, bool bCanAbstain, bool bPrediction)
{
	TArray<FString> verdict;

	FLeanStruct partyLean;
	partyLean.Party = GetMembersParty(Representative)->Party;

	int32 index = Bill.Lean.Find(partyLean);

	if (index != INDEX_NONE && !Bill.Lean[index].ForRange.IsEmpty() && IsInRange(Bill.Lean[index].ForRange, Bill.Value))
		verdict = { "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Agreeing", "Abstaining", "Abstaining", "Opposing" };
	else if (index != INDEX_NONE && !Bill.Lean[index].AgainstRange.IsEmpty() && IsInRange(Bill.Lean[index].AgainstRange, Bill.Value))
		verdict = { "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Opposing", "Abstaining", "Abstaining", "Agreeing" };
	else
		verdict = { "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Abstaining", "Agreeing", "Agreeing", "Opposing", "Opposing" };

	if (!bCanAbstain)
		verdict.Remove("Abstaining");

	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(Representative)) {
		FLeanStruct personalityLean;
		personalityLean.Personality = personality->Trait;

		index = Bill.Lean.Find(personalityLean);

		if (index == INDEX_NONE)
			continue;

		if (!Bill.Lean[index].ForRange.IsEmpty() && IsInRange(Bill.Lean[index].ForRange, Bill.Value))
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
		else if (!Bill.Lean[index].AgainstRange.IsEmpty() && IsInRange(Bill.Lean[index].AgainstRange, Bill.Value))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	index = Faction->Politics.Laws.Find(Bill);

	if (Bill.BillType.Contains("Cost")) {
		int32 leftoverMoney = 0;

		if (IsValid(Representative->BuildingComponent->Employment))
			leftoverMoney += Representative->BuildingComponent->Employment->GetWage(Representative);

		if (IsValid(Representative->BuildingComponent->House))
			leftoverMoney -= Representative->BuildingComponent->House->GetAmount(Representative);

		if (leftoverMoney < Bill.Value)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}
	else if (Bill.BillType.Contains("Age")) {
		if (Faction->Politics.Laws[index].Value <= Representative->BioComponent->Age) {
			if (Bill.Value > Representative->BioComponent->Age)
				verdict.Append({ "Opposing", "Opposing", "Opposing" });
			else
				verdict.Append({ "Abstaining", "Abstaining", "Abstaining" });
		}
		else if (Bill.Value <= Representative->BioComponent->Age)
			verdict.Append({ "Agreeing", "Agreeing", "Agreeing" });
	}
	else if (Bill.BillType == "Representative Type") {
		if (Bill.Value == 1 && !IsValid(Representative->BuildingComponent->Employment))
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
		else if (Bill.Value == 2 && Representative->Balance < 15)
			verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}
	else if (Bill.BillType == "Same-Sex Laws" && Bill.Value < Faction->Politics.Laws[index].Value && Representative->BioComponent->Sexuality != ESexuality::Straight) {
		verdict.Append({ "Opposing", "Opposing", "Opposing" });
	}

	auto value = Async(EAsyncExecution::TaskGraph, [this, verdict]() { return Camera->Stream.RandRange(0, verdict.Num() - 1); });

	FString result = verdict[value.Get()];

	if (bPrediction) {
		if (result == "Agreeing")
			Faction->Politics.Predictions.For.Add(Representative);
		else if (result == "Opposing")
			Faction->Politics.Predictions.Against.Add(Representative);
	}
	else {
		if (result == "Agreeing")
			Faction->Politics.Votes.For.Add(Representative);
		else if (result == "Opposing")
			Faction->Politics.Votes.Against.Add(Representative);
	}
}

void UPoliticsManager::TallyVotes(FFactionStruct* Faction, FLawStruct Bill)
{
	bool bPassed = false;

	if (Faction->Politics.Votes.For.Num() > Faction->Politics.Votes.Against.Num()) {
		int32 index = Faction->Politics.Laws.Find(Bill);

		if (Faction->Politics.Laws[index].BillType == "Abolish") {
			TArray<FTimerParameterStruct> params;
			Camera->TimerManager->SetParameter(1000, params);
			Camera->TimerManager->SetParameter(Camera, params);

			Camera->TimerManager->CreateTimer("Abolish", Faction->EggTimer, 6.0f, "TakeHealth", params, false);
		}
		else if (Faction->Politics.Laws[index].BillType == "Election") {
			Election(*Faction);
		}
		else {
			Faction->Politics.Laws[index].Value = Bill.Value;

			if (Faction->Politics.Laws[index].BillType.Contains("Age")) {
				for (ACitizen* citizen : Faction->Citizens) {
					if (IsValid(citizen->BuildingComponent->Employment) && !citizen->BuildingComponent->CanWork(citizen->BuildingComponent->Employment))
						citizen->BuildingComponent->Employment->RemoveCitizen(citizen);

					if (IsValid(citizen->BuildingComponent->School))
						citizen->BuildingComponent->RemoveOnReachingWorkAge(Faction);

					if (citizen->BioComponent->Age >= Faction->Politics.Laws[index].Value)
						continue;

					FPartyStruct* party = GetMembersParty(citizen);

					if (party == nullptr)
						continue;

					party->Members.Remove(citizen);
				}
			}
			else if (Faction->Politics.Laws[index].BillType == "Same-Sex Laws" && Bill.Value != 2) {
				for (ACitizen* citizen : Faction->Citizens) {
					if (citizen->BioComponent->Partner == nullptr || citizen->BioComponent->Sex != citizen->BioComponent->Partner->BioComponent->Sex)
						continue;

					if (Bill.Value == 0)
						citizen->BioComponent->RemovePartner();
					else
						citizen->BioComponent->RemoveMarriage();
				}
			}
		}

		if (Faction->Politics.Laws[index].BillType == "Representatives" && Camera->ParliamentUIInstance->IsInViewport())
			Camera->RefreshRepresentatives();

		bPassed = true;
	}

	if (Faction->Name == Camera->ColonyName) {
		FDescriptionStruct descriptionStruct;

		for (FDescriptionStruct desc : Bill.Description) {
			if (desc.Min > Bill.Value || desc.Max < Bill.Value)
				continue;

			descriptionStruct = desc;

			break;
		}

		FString value = FString::FromInt(Bill.Value);
		FString description = descriptionStruct.Description.Replace(TEXT("X"), *value);

		Camera->LawPassed(bPassed, description, Faction->Politics.Votes.For.Num(), Faction->Politics.Votes.Against.Num());

		Camera->LawPassedUIInstance->AddToViewport();
	}

	Faction->Politics.ProposedBills.Remove(Bill);

	SetupBill(Faction);
}

FString UPoliticsManager::GetBillPassChance(FString FactionName, FLawStruct Bill)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->Politics.Predictions.Clear();

	TArray<int32> results = { 0, 0, 0 };

	SetElectionBillLeans(faction, &Bill);

	for (int32 i = 0; i < 10; i++) {
		for (ACitizen* citizen : faction->Politics.Representatives)
			GetVerdict(faction, citizen, Bill, true, true);

		int32 abstainers = (faction->Politics.Representatives.Num() - faction->Politics.Predictions.For.Num() - faction->Politics.Predictions.Against.Num());

		if (faction->Politics.Predictions.For.Num() > faction->Politics.Predictions.Against.Num() + abstainers)
			results[0]++;
		else if (faction->Politics.Predictions.Against.Num() > faction->Politics.Predictions.For.Num() + abstainers)
			results[1]++;
		else
			results[2]++;

		faction->Politics.Predictions.Clear();
	}

	if (results[0] > results[1] + results[2])
		return "High";
	else if (results[1] > results[0] + results[2])
		return "Low";
	else
		return "Random";
}

int32 UPoliticsManager::GetLawValue(FString FactionName, FString BillType)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	FLawStruct lawStruct;
	lawStruct.BillType = BillType;

	int32 index = faction->Politics.Laws.Find(lawStruct);

	return faction->Politics.Laws[index].Value;
}

int32 UPoliticsManager::GetCooldownTimer(FString FactionName, FLawStruct Law)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 index = faction->Politics.Laws.Find(Law);

	if (index == INDEX_NONE)
		return 0;

	return faction->Politics.Laws[index].Cooldown;
}

ERaidPolicy UPoliticsManager::GetRaidPolicyStatus(ACitizen* Citizen)
{
	ERaidPolicy policy = ERaidPolicy::Default;

	if (!GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>()->Enemies.IsEmpty())
		policy = ERaidPolicy(GetLawValue(Camera->ConquestManager->GetCitizenFaction(Citizen).Name, "Raid Policy"));

	return policy;
}

//
// Rebel
//
void UPoliticsManager::ChooseRebellionType(FFactionStruct* Faction)
{
	if (Faction->Citizens.IsEmpty() || IsRebellion(Faction))
		return;

	Faction->RebelCooldownTimer--;

	if (Faction->RebelCooldownTimer < 1) {
		int32 value = Camera->Stream.RandRange(1, 3);

		if (value == 3 || Faction->Politics.Representatives.IsEmpty()) {
			Overthrow(Faction);
		}
		else {
			FLawStruct lawStruct;
			lawStruct.BillType = "Abolish";

			int32 index = Faction->Politics.Laws.Find(lawStruct);

			ProposeBill(Faction->Name, Faction->Politics.Laws[index]);
		}
	}
}

void UPoliticsManager::Overthrow(FFactionStruct* Faction)
{
	Faction->RebelCooldownTimer = 1500;

	Camera->PoliceManager->CeaseAllInternalFighting(Faction);

	for (ACitizen* citizen : Faction->Citizens) {
		if (Camera->PoliticsManager->GetMembersParty(citizen) == nullptr || Camera->PoliticsManager->GetMembersParty(citizen)->Party != "Shell Breakers")
			return;

		if (Faction->Politics.Representatives.Contains(citizen))
			Faction->Politics.Representatives.Remove(citizen);

		SetupRebel(Faction, citizen);
	}

	for (ASpecial* building : Camera->Grid->SpecialBuildings) {
		if (!building->IsA<ACloneLab>())
			continue;

		Cast<ACloneLab>(building)->StartCloneLab();

		break;
	}
}

void UPoliticsManager::SetupRebel(FFactionStruct* Faction, ACitizen* Citizen)
{
	Citizen->Energy = 100;
	Citizen->Hunger = 100;

	UAIVisualiser* aiVisualiser = Camera->Grid->AIVisualiser;
	aiVisualiser->RemoveInstance(aiVisualiser->HISMCitizen, Faction->Citizens.Find(Citizen));
	Faction->Citizens.Remove(Citizen);

	Faction->Rebels.Add(Citizen);
	aiVisualiser->AddInstance(Citizen, aiVisualiser->HISMRebel, Citizen->MovementComponent->Transform);

	Citizen->MoveToBroch();
}

bool UPoliticsManager::IsRebellion(FFactionStruct* Faction)
{
	return !Faction->Rebels.IsEmpty();
}