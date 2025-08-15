#include "Player/Managers/ConquestManager.h"

#include "Components/WidgetComponent.h"
#include "Misc/ScopeTryLock.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "AI/Clone.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Misc/Portal.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	EnemiesNum = 2;
}

void UConquestManager::BeginPlay()
{
	Super::BeginPlay();
	
	Camera = Cast<ACamera>(GetOwner());
}

void UConquestManager::CreateFactions()
{
	TArray<FString> factions;
	factions.Add(Camera->ColonyName);

	FString factionNames;
	FFileHelper::LoadFileToString(factionNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/ColonyNames.txt"));

	TArray<FString> factionsParsed;
	factionNames.ParseIntoArray(factionsParsed, TEXT(","));

	for (int32 i = 0; i < EnemiesNum; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, factionsParsed.Num() - 1);
		factions.Add(factionsParsed[index]);

		if (factionsParsed[index] != "")
			factionsParsed.RemoveAt(index);
	}

	for (FString name : factions) {
		FFactionStruct f;
		f.Name = name;

		Factions.Add(f);

		// Get all valid tile placements for broch, then check if another broch within 3000 units. Add buildings, rebels and citizens to faction struct as storage.

		SetFactionCulture(f);
	}

	Camera->SetFactionsInDiplomacyUI();
}

int32 UConquestManager::GetFactionIndexFromOwner(FString Owner)
{
	for (int32 i = 0; i < Factions.Num(); i++) {
		if (Factions[i].Name != Owner)
			continue;

		return i;
	}

	UE_LOGFMT(LogTemp, Fatal, "Faction not found");
}

FFactionStruct UConquestManager::GetFactionFromOwner(FString Owner)
{
	return Factions[GetFactionIndexFromOwner(Owner)];
}

UTexture2D* UConquestManager::GetTextureFromCulture(FString Type)
{
	UTexture2D* texture = nullptr;

	for (FCultureImageStruct culture : CultureTextureList) {
		if (culture.Type != Type)
			continue;

		texture = culture.Texture;

		break;
	}
		
	return texture;
}

void UConquestManager::ComputeAI()
{
	FScopeTryLock lock(&ConquestLock);
	if (!lock.IsLocked())
		return;

	for (FFactionStruct& faction : Factions) {
		if (faction.AtWar.IsEmpty() && faction.WarFatigue > 0)
			faction.WarFatigue--;
		else if (!faction.AtWar.IsEmpty())
			faction.WarFatigue++;

		SetFactionsHappiness(faction);

		if (faction.Name == Camera->ColonyName)
			continue;

		SetFactionCulture(faction);

		Camera->UpdateFactionHappiness();

		EvaluateDiplomacy(faction);

		EvaluateAI(faction);
	}

	for (FFactionStruct faction : FactionsToRemove) {
		for (FFactionStruct& f : Factions) {
			if (f == faction)
				continue;

			for (int32 i = f.Happiness.Num() - 1; i > -1; i--)
				if (f.Happiness[i].Owner == faction.Name)
					f.Happiness.RemoveAt(i);
		}

		int32 index = GetFactionIndexFromOwner(faction.Name);

		Camera->RemoveFactionBtn(index);

		Factions.RemoveAt(index);
	}
}

bool UConquestManager::CanTravel(class ACitizen* Citizen)
{
	if (Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue("Work Age") || !Citizen->WillWork())
		return false;

	return true;
}

FFactionStruct UConquestManager::GetCitizenFaction(ACitizen* Citizen)
{
	FFactionStruct faction;

	/*Give faction struct citizens, rebels and buildings array*/

	return faction;
}

//
// Diplomacy
//
void UConquestManager::SetFactionCulture(FFactionStruct Faction)
{
	int32 index = Factions.Find(Faction);

	TMap<FString, int32> partyCount;

	for (ACitizen* Citizen : Camera->CitizenManager->Representatives) {
		FString party = Camera->CitizenManager->GetCitizenParty(Citizen);

		if (partyCount.Contains(party)) {
			int32* count = partyCount.Find(party);
			count++;
		}
		else {
			partyCount.Add(party, 1);
		}
	}

	TTuple<FString, int32> biggestParty;

	for (auto& element : partyCount) {
		if (biggestParty.Value >= element.Value)
			continue;

		biggestParty = element;
	}

	TMap<FString, int32> religionCount;

	for (ACitizen* citizen : Camera->CitizenManager->Citizens) {
		FString faith = citizen->Spirituality.Faith;

		if (religionCount.Contains(faith)) {
			int32* count = religionCount.Find(faith);
			count++;
		}
		else {
			religionCount.Add(faith, 1);
		}
	}

	TTuple<FString, int32> biggestReligion;

	for (auto& element : religionCount) {
		if (biggestReligion.Value >= element.Value)
			continue;

		biggestReligion = element;
	}

	if (Factions[index].PartyInPower == biggestParty.Key && Factions[index].LargestReligion == biggestReligion.Key)
		return;

	Factions[index].PartyInPower = biggestParty.Key;
	Factions[index].LargestReligion = biggestReligion.Key;

	Camera->UpdateFactionIcons(index);
}

int32 UConquestManager::GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionHappinessStruct happiness;
	happiness.Owner = Target.Name;

	int32 i = Factions.Find(Faction);

	int32 index = Factions[i].Happiness.Find(happiness);

	if (index == INDEX_NONE)
		index = Factions[i].Happiness.Add(happiness);

	happiness = Factions[i].Happiness[index];

	return index;
}

int32 UConquestManager::GetHappinessValue(FFactionHappinessStruct Happiness)
{
	int32 value = 0;

	for (auto& element : Happiness.Modifiers)
		value += element.Value;

	return value;
}

void UConquestManager::SetFactionsHappiness(FFactionStruct Faction)
{
	for (FFactionStruct& f : Factions) {
		if (f.Name == Faction.Name)
			continue;

		int32 i = Factions.Find(Faction);

		int32 index = GetHappinessWithFaction(Faction, f);
		Factions[i].Happiness[index].ProposalTimer = FMath::Max(Factions[i].Happiness[index].ProposalTimer - 1, 0);

		if (Faction.LargestReligion != f.LargestReligion) {
			int32 value = -18;

			if ((Faction.LargestReligion == "Egg" || Faction.LargestReligion == "Chicken") && (f.LargestReligion == "Egg" || f.LargestReligion == "Chicken"))
				value = -6;

			if (Factions[i].Happiness[index].Contains("Same religion"))
				Factions[i].Happiness[index].RemoveValue("Same religion");

			Factions[i].Happiness[index].SetValue("Different religion", value);
		}
		else {
			if (Factions[i].Happiness[index].Contains("Different religion"))
				Factions[i].Happiness[index].RemoveValue("Different religion");

			Factions[i].Happiness[index].SetValue("Same religion", 18);
		}

		if (Faction.PartyInPower != f.PartyInPower) {
			if (Factions[i].Happiness[index].Contains("Same party"))
				Factions[i].Happiness[index].RemoveValue("Same party");

			Factions[i].Happiness[index].SetValue("Different party", -18);
		}
		else {
			if (Factions[i].Happiness[index].Contains("Different party"))
				Factions[i].Happiness[index].RemoveValue("Different party");

			Factions[i].Happiness[index].SetValue("Same party", 18);
		}

		if (Factions[i].Happiness[index].Contains("Recently enemies"))
			Factions[i].Happiness[index].Decay("Recently enemies");

		if (Factions[i].Happiness[index].Contains("Recently allies"))
			Factions[i].Happiness[index].Decay("Recently allies");

		if (Factions[i].Happiness[index].Contains("Insulted"))
			Factions[i].Happiness[index].Decay("Insulted");

		if (Factions[i].Happiness[index].Contains("Praised"))
			Factions[i].Happiness[index].Decay("Praised");

		if (f.Allies.Contains(Faction.Name)) {
			Factions[i].Happiness[index].SetValue("Allies", 48);
		}
		else if (Factions[i].Happiness[index].Contains("Allies")) {
			Factions[i].Happiness[index].SetValue("Recently allies", -12);
			Factions[i].Happiness[index].RemoveValue("Allies");
		}

		for (FString owner : Faction.Allies) {
			if (Factions[i].Happiness[index].Contains("Recently allied with " + owner))
				Factions[i].Happiness[index].Decay("Recently allied with " + owner);

			if (Factions[i].Happiness[index].Contains("Recently warring with ally: " + owner))
				Factions[i].Happiness[index].Decay("Recently warring with ally: " + owner);

			if (f.Allies.Contains(owner)) {
				Factions[i].Happiness[index].SetValue("Allied with: " + owner, 12);
			}
			else if (Factions[i].Happiness[index].Contains("Allied with " + owner)) {
				Factions[i].Happiness[index].SetValue("Recently allied with " + owner, -6);
				Factions[i].Happiness[index].RemoveValue("Allied with " + owner);
			}

			if (f.AtWar.Contains(owner)) {
				Factions[i].Happiness[index].SetValue("Warring with ally: " + owner, -12);
			}
			else if (Factions[i].Happiness[index].Contains("Warring with ally: " + owner)) {
				Factions[i].Happiness[index].SetValue("Recently wrring with ally: " + owner, -6);
				Factions[i].Happiness[index].RemoveValue("Warring with ally: " + owner);
			}
		}

		if (f.AtWar.Contains(Faction.Name)) {
			Factions[i].Happiness[index].SetValue("Enemies", -48);
		}
		else if (Factions[i].Happiness[index].Contains("Enemies")) {
			Factions[i].Happiness[index].SetValue("Recently enemies", -24);
			Factions[i].Happiness[index].RemoveValue("Enemies");
		}

		for (FString owner : Faction.AtWar) {
			if (Factions[i].Happiness[index].Contains("Recently enemies with " + owner))
				Factions[i].Happiness[index].Decay("Recently enemies with " + owner);

			if (Factions[i].Happiness[index].Contains("Recently allied with enemy: " + owner))
				Factions[i].Happiness[index].Decay("Recently allied with enemy: " + owner);

			if (f.AtWar.Contains(owner)) {
				Factions[i].Happiness[index].SetValue("Enemies with " + owner, 12);
			}
			else if (Factions[i].Happiness[index].Contains("Enemies with " + owner)) {
				Factions[i].Happiness[index].SetValue("Recently enemies with " + owner, 6);
				Factions[i].Happiness[index].RemoveValue("Enemies with " + owner);
			}

			if (f.Allies.Contains(owner)) {
				Factions[i].Happiness[index].SetValue("Allied with enemy: " + owner, -12);
			}
			else if (Factions[i].Happiness[index].Contains("Allied with enemy: " + owner)) {
				Factions[i].Happiness[index].SetValue("Recently allied with enemy: " + owner, 6);
				Factions[i].Happiness[index].RemoveValue("Allied with enemy: " + owner);
			}
		}

		bool bTooClose = false;

		// Loop through factions buildings

		if (!bTooClose && Factions[i].Happiness[index].Contains(f.Name + "settled on island close to capital"))
			Factions[i].Happiness[index].RemoveValue(f.Name + "settled on island close to capital");
	}
}

void UConquestManager::EvaluateDiplomacy(FFactionStruct Faction)
{
	TArray<FFactionStruct> potentialEnemies;
	TArray<FFactionStruct> potentialAllies;

	int32 i = Factions.Find(Faction);
	
	for (FFactionStruct& f : Factions) {
		if (f.Name == Faction.Name || f.Name == Camera->ColonyName)
			continue;

		int32 i1 = GetHappinessWithFaction(Faction, f);
		int32 i2 = GetHappinessWithFaction(f, Faction);

		int32 value = GetHappinessValue(Factions[i].Happiness[i1]);
		int32 fValue = GetHappinessValue(f.Happiness[i2]);

		if (value < 12 && Faction.AtWar.IsEmpty() && Faction.Allies.Contains(f.Name)) {
			BreakAlliance(Faction, f);
		}
		else if (Faction.AtWar.Contains(f.Name)) {
			int32 newValue = value;
			int32 newFValue = fValue;

			TTuple<bool, bool> winnability = IsWarWinnable(Faction, f);

			if (winnability.Key)
				newValue -= 24;
			
			if (winnability.Value)
				newFValue -= 24;

			if (newValue + (Faction.WarFatigue / 3) >= 0) {
				if (f.Name == Camera->ColonyName) {
					DisplayConquestNotification(Faction.Name + " offers peace", Faction.Name, true);

					Factions[i].Happiness[i1].ProposalTimer = 24;
				}
				else if (newFValue + (f.WarFatigue / 3) >= 0)
					Peace(Faction, f);
			}
		}

		if (f.Name == Camera->ColonyName)
			fValue = 24;

		if (value < 6) {
			if (Faction.AtWar.IsEmpty() && !Faction.Allies.Contains(f.Name) && Faction.WarFatigue == 0 && IsWarWinnable(Faction, f).Key && !f.Happiness[i2].Contains("Recently allies"))
				potentialEnemies.Add(f);
			else if (!f.Happiness[i2].Contains("Insulted"))
				Insult(Faction, f);
		}
		else if (value >= 24 && fValue >= 24 && !Faction.Allies.Contains(f.Name) && !Faction.AtWar.Contains(f.Name) && !f.Happiness[i2].Contains("Recently allies"))
			potentialAllies.Add(f);
		else if (value > 6 && !f.Happiness[i2].Contains("Praised"))
			Praise(Faction, f);

	}

	if (!potentialEnemies.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, potentialEnemies.Num() - 1);
		FFactionStruct f = potentialEnemies[index];

		DeclareWar(Faction, f);

		if (f.Name == Camera->ColonyName)
			DisplayConquestNotification(Faction.Name + " has declared war on you", Faction.Name, false);
	}

	if (!potentialAllies.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, potentialAllies.Num() - 1);
		FFactionStruct f = potentialAllies[index];

		if (f.Name == Camera->ColonyName) {
			DisplayConquestNotification(Faction.Name + " wants to be your ally", Faction.Name, true);

			int32 i1 = GetHappinessWithFaction(Faction, f);
			Factions[i].Happiness[i1].ProposalTimer = 24;
		}
		else
			Ally(Faction, f);
	}
}

TTuple<bool, bool> UConquestManager::IsWarWinnable(FFactionStruct Faction, FFactionStruct& Target)
{
	int32 factionCitizens = 0;
	int32 targetCitizens = 0;

	// Get citizens arrays from Faction and Target then compare strength AND health.

	int32 total = FMath::Max(factionCitizens + targetCitizens, 1.0f);

	bool bFactionCanWin = false;
	bool bTargetCanWin = false;

	if (factionCitizens / total > 0.4f)
		bFactionCanWin = true;

	if (targetCitizens / total > 0.4f)
		bTargetCanWin = true;

	return TTuple<bool, bool>(bFactionCanWin, bTargetCanWin);
}

void UConquestManager::Peace(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].AtWar.Remove(Factions[i2].Name);
	Factions[i2].AtWar.Remove(Factions[i1].Name);
}

void UConquestManager::Ally(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);
	
	Factions[i1].Allies.Add(Factions[i2].Name);
	Factions[i2].Allies.Add(Factions[i1].Name);
}

void UConquestManager::BreakAlliance(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].Allies.Remove(Factions[i2].Name);
	Factions[i2].Allies.Remove(Factions[i1].Name);
}

void UConquestManager::DeclareWar(FFactionStruct Faction1, FFactionStruct Faction2)
{
	int32 i1 = Factions.Find(Faction1);
	int32 i2 = Factions.Find(Faction2);

	Factions[i1].AtWar.Add(Factions[i2].Name);
	Factions[i2].AtWar.Add(Factions[i1].Name);
}

void UConquestManager::Insult(FFactionStruct Faction, FFactionStruct Target)
{
	int32 i = Factions.Find(Faction);
	int32 index = GetHappinessWithFaction(Target, Faction);

	Factions[i].Happiness[index].SetValue("Insulted", -12);
}

void UConquestManager::Praise(FFactionStruct Faction, FFactionStruct Target)
{
	int32 i = Factions.Find(Faction);
	int32 index = GetHappinessWithFaction(Target, Faction);

	Factions[i].Happiness[index].SetValue("Praised", 12);
}

void UConquestManager::Gift(FFactionStruct Faction, TArray<FGiftStruct> Gifts)
{
	for (FGiftStruct gift : Gifts) {
		if (!Camera->ResourceManager->TakeUniversalResource(gift.Resource, gift.Amount, 0))
			continue;

		int32 i = Factions.Find(Faction);

		FFactionStruct playerFaction = GetFactionFromOwner(Camera->ColonyName);

		int32 index = GetHappinessWithFaction(Faction, playerFaction);

		int32 value = Camera->ResourceManager->GetMarketValue(gift.Resource) * gift.Amount / 12;

		if (Factions[i].Happiness[index].Contains("Received a gift"))
			Factions[i].Happiness[index].SetValue("Received a gift", Factions[i].Happiness[index].GetValue("Received a gift") + value);
		else
			Factions[i].Happiness[index].SetValue("Received a gift", value);
	}
}

//
// AI
//
void UConquestManager::EvaluateAI(FFactionStruct Faction)
{
	// Use to evaluate where to move the amassed army when at war, and also, where to place which buildings
}

//
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}