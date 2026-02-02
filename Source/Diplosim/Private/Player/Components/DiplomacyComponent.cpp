#include "Player/Components/DiplomacyComponent.h"

#include "Components/WidgetComponent.h"

#include "AI/Citizen/Citizen.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Broch.h"
#include "Map/Grid.h"
#include "Player/Camera.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/ResourceManager.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"

UDiplomacyComponent::UDiplomacyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitCultureList();
}

void UDiplomacyComponent::InitCultureList()
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	TArray<FString> dirList = { FPaths::ProjectDir() + "/Content/Custom/Structs/Religions.json",  FPaths::ProjectDir() + "/Content/Custom/Structs/Parties.json" };

	for (FString dir : dirList) {
		FString fileContents;
		FFileHelper::LoadFileToString(fileContents, *dir);
		TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

		if (!FJsonSerializer::Deserialize(jsonReader, jsonObject) || !jsonObject.IsValid())
			continue;

		for (auto& element : jsonObject->Values)
			for (auto& e : element.Value->AsArray())
				for (auto& v : e->AsObject()->Values)
					if (v.Value->Type == EJson::String)
						CultureTextureList.Add(v.Value->AsString(), nullptr);
	}
}

UMaterialInstanceDynamic* UDiplomacyComponent::GetTextureFromCulture(FString Party, FString Religion)
{
	UTexture2D* partyTexture = *CultureTextureList.Find(Party);
	UTexture2D* religionTexture = *CultureTextureList.Find(Religion);

	UMaterialInstanceDynamic* material = UMaterialInstanceDynamic::Create(CultureMaterial, this);
	material->SetTextureParameterValue("Party", partyTexture);
	material->SetTextureParameterValue("Religion", religionTexture);

	return material;
}

FFactionStruct* UDiplomacyComponent::GetFactionPtr(FFactionStruct Faction)
{
	int32 index = Camera->ConquestManager->Factions.Find(Faction);
	return &Camera->ConquestManager->Factions[index];
}

void UDiplomacyComponent::SetFactionCulture(FFactionStruct* Faction)
{
	TArray<ACitizen*> partyCitizens = Faction->Politics.Representatives;
	if (partyCitizens.IsEmpty())
		partyCitizens = Faction->Citizens;

	TMap<FString, int32> partyCount;
	for (ACitizen* citizen : partyCitizens) {
		FString party = Camera->PoliticsManager->GetCitizenParty(citizen);

		if (partyCount.Contains(party)) {
			int32* count = partyCount.Find(party);
			count++;
		}
		else {
			partyCount.Add(party, 1);
		}
	}

	TTuple<FString, int32> biggestParty = TTuple<FString, int32>("Undecided", 0);
	for (auto& element : partyCount) {
		if (biggestParty.Value >= element.Value)
			continue;

		biggestParty = element;
	}

	TMap<FString, int32> religionCount;
	for (ACitizen* citizen : Faction->Citizens) {
		FString faith = citizen->Spirituality.Faith;

		if (religionCount.Contains(faith)) {
			int32* count = religionCount.Find(faith);
			count++;
		}
		else {
			religionCount.Add(faith, 1);
		}
	}

	TTuple<FString, int32> biggestReligion = TTuple<FString, int32>("Atheist", 0);
	for (auto& element : religionCount) {
		if (biggestReligion.Value >= element.Value)
			continue;

		biggestReligion = element;
	}

	if (Faction->PartyInPower == biggestParty.Key && Faction->LargestReligion == biggestReligion.Key)
		return;

	Faction->PartyInPower = biggestParty.Key;
	Faction->LargestReligion = biggestReligion.Key;

	Faction->Flag = GetTextureFromCulture(Faction->PartyInPower, Faction->LargestReligion);

	Camera->UpdateFactionIcons(Camera->ConquestManager->Factions.Find(*Faction));
}

void UDiplomacyComponent::SetFactionFlagColour(FFactionStruct* Faction)
{
	FTileStruct* tile = Camera->Grid->GetTileFromLocation(Faction->EggTimer->GetActorLocation());
	auto bound = Camera->Grid->GetMapBounds();

	Faction->FlagColour = FLinearColor(tile->X / bound, FMath::Abs(tile->Y / bound - 1.0f), tile->Level / Camera->Grid->MaxLevel);
}

int32 UDiplomacyComponent::GetHappinessWithFaction(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionHappinessStruct happiness;
	happiness.Owner = Target.Name;

	FFactionStruct* faction = GetFactionPtr(Faction);

	int32 index = faction->Happiness.Find(happiness);

	if (index == INDEX_NONE)
		index = faction->Happiness.Add(happiness);

	happiness = faction->Happiness[index];

	return index;
}

int32 UDiplomacyComponent::GetHappinessValue(FFactionHappinessStruct Happiness)
{
	int32 value = 0;

	for (auto& element : Happiness.Modifiers)
		value += element.Value;

	return value;
}

void UDiplomacyComponent::SetFactionsHappiness(FFactionStruct Faction)
{
	for (FFactionStruct& f : Camera->ConquestManager->Factions) {
		if (Camera->ConquestManager->FactionsToRemove.Contains(f) || f == Faction)
			continue;

		FFactionStruct* faction = GetFactionPtr(Faction);

		int32 index = GetHappinessWithFaction(Faction, f);
		faction->Happiness[index].ProposalTimer = FMath::Max(faction->Happiness[index].ProposalTimer - 1, 0);

		if (Faction.LargestReligion != f.LargestReligion) {
			int32 value = -18;

			if ((Faction.LargestReligion == "Egg" || Faction.LargestReligion == "Chicken") && (f.LargestReligion == "Egg" || f.LargestReligion == "Chicken"))
				value = -6;

			if (faction->Happiness[index].Contains("Same religion"))
				faction->Happiness[index].RemoveValue("Same religion");

			faction->Happiness[index].SetValue("Different religion", value);
		}
		else {
			if (faction->Happiness[index].Contains("Different religion"))
				faction->Happiness[index].RemoveValue("Different religion");

			faction->Happiness[index].SetValue("Same religion", 18);
		}

		if (Faction.PartyInPower != f.PartyInPower) {
			if (faction->Happiness[index].Contains("Same party"))
				faction->Happiness[index].RemoveValue("Same party");

			faction->Happiness[index].SetValue("Different party", -18);
		}
		else {
			if (faction->Happiness[index].Contains("Different party"))
				faction->Happiness[index].RemoveValue("Different party");

			faction->Happiness[index].SetValue("Same party", 18);
		}

		if (faction->Happiness[index].Contains("Recently enemies"))
			faction->Happiness[index].Decay("Recently enemies");

		if (faction->Happiness[index].Contains("Recently allies"))
			faction->Happiness[index].Decay("Recently allies");

		if (faction->Happiness[index].Contains("Insulted"))
			faction->Happiness[index].Decay("Insulted");

		if (faction->Happiness[index].Contains("Praised"))
			faction->Happiness[index].Decay("Praised");

		if (f.Allies.Contains(Faction.Name)) {
			faction->Happiness[index].SetValue("Allies", 48);
		}
		else if (faction->Happiness[index].Contains("Allies")) {
			faction->Happiness[index].SetValue("Recently allies", -12);
			faction->Happiness[index].RemoveValue("Allies");
		}

		for (FString owner : Faction.Allies) {
			if (faction->Happiness[index].Contains("Recently allied with " + owner))
				faction->Happiness[index].Decay("Recently allied with " + owner);

			if (faction->Happiness[index].Contains("Recently warring with ally: " + owner))
				faction->Happiness[index].Decay("Recently warring with ally: " + owner);

			if (f.Allies.Contains(owner)) {
				faction->Happiness[index].SetValue("Allied with: " + owner, 12);
			}
			else if (faction->Happiness[index].Contains("Allied with " + owner)) {
				faction->Happiness[index].SetValue("Recently allied with " + owner, -6);
				faction->Happiness[index].RemoveValue("Allied with " + owner);
			}

			if (f.AtWar.Contains(owner)) {
				faction->Happiness[index].SetValue("Warring with ally: " + owner, -12);
			}
			else if (faction->Happiness[index].Contains("Warring with ally: " + owner)) {
				faction->Happiness[index].SetValue("Recently wrring with ally: " + owner, -6);
				faction->Happiness[index].RemoveValue("Warring with ally: " + owner);
			}
		}

		if (f.AtWar.Contains(Faction.Name)) {
			faction->Happiness[index].SetValue("Enemies", -48);
		}
		else if (faction->Happiness[index].Contains("Enemies")) {
			faction->Happiness[index].SetValue("Recently enemies", -24);
			faction->Happiness[index].RemoveValue("Enemies");
		}

		for (FString owner : Faction.AtWar) {
			if (faction->Happiness[index].Contains("Recently enemies with " + owner))
				faction->Happiness[index].Decay("Recently enemies with " + owner);

			if (faction->Happiness[index].Contains("Recently allied with enemy: " + owner))
				faction->Happiness[index].Decay("Recently allied with enemy: " + owner);

			if (f.AtWar.Contains(owner)) {
				faction->Happiness[index].SetValue("Enemies with " + owner, 12);
			}
			else if (faction->Happiness[index].Contains("Enemies with " + owner)) {
				faction->Happiness[index].SetValue("Recently enemies with " + owner, 6);
				faction->Happiness[index].RemoveValue("Enemies with " + owner);
			}

			if (f.Allies.Contains(owner)) {
				faction->Happiness[index].SetValue("Allied with enemy: " + owner, -12);
			}
			else if (faction->Happiness[index].Contains("Allied with enemy: " + owner)) {
				faction->Happiness[index].SetValue("Recently allied with enemy: " + owner, 6);
				faction->Happiness[index].RemoveValue("Allied with enemy: " + owner);
			}
		}

		bool bTooClose = false;

		for (ABuilding* building : faction->Buildings) {
			TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(building, 3.0f);

			for (FHitResult hit : hits) {
				if (!hit.GetActor()->IsA<ABuilding>() || Cast<ABuilding>(hit.GetActor())->FactionName != Faction.Name)
					continue;

				bTooClose = true;

				break;
			}
		}

		if (!bTooClose && faction->Happiness[index].Contains(f.Name + "is too close to settlement"))
			faction->Happiness[index].RemoveValue(f.Name + "is too close to settlement");
		else if (bTooClose)
			faction->Happiness[index].SetValue(f.Name + "is too close to settlement", -6);
	}
}

void UDiplomacyComponent::EvaluateDiplomacy(FFactionStruct Faction)
{
	TArray<FFactionStruct> potentialEnemies;
	TArray<FFactionStruct> potentialAllies;

	int32 index = Camera->ConquestManager->Factions.Find(Faction);
	FFactionStruct* faction = &Camera->ConquestManager->Factions[index];

	for (FFactionStruct& f : Camera->ConquestManager->Factions) {
		if (f.Name == Faction.Name || f.Name == Camera->ColonyName)
			continue;

		int32 i1 = GetHappinessWithFaction(Faction, f);
		int32 i2 = GetHappinessWithFaction(f, Faction);

		int32 value = GetHappinessValue(Faction.Happiness[i1]);
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
					Camera->ConquestManager->DisplayConquestNotification(Faction.Name + " offers peace", Faction.Name, true);

					faction->Happiness[i1].ProposalTimer = 24;
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
		int32 i = Camera->Stream.RandRange(0, potentialEnemies.Num() - 1);
		FFactionStruct f = potentialEnemies[i];

		DeclareWar(Faction, f);

		if (f.Name == Camera->ColonyName)
			Camera->ConquestManager->DisplayConquestNotification(Faction.Name + " has declared war on you", Faction.Name, false);
	}

	if (!potentialAllies.IsEmpty()) {
		int32 i = Camera->Stream.RandRange(0, potentialAllies.Num() - 1);
		FFactionStruct f = potentialAllies[i];

		if (f.Name == Camera->ColonyName) {
			Camera->ConquestManager->DisplayConquestNotification(Faction.Name + " wants to be your ally", Faction.Name, true);

			int32 i1 = GetHappinessWithFaction(Faction, f);
			faction->Happiness[i1].ProposalTimer = 24;
		}
		else
			Ally(Faction, f);
	}
}

int32 UDiplomacyComponent::GetStrengthScore(FFactionStruct Faction)
{
	int32 value = 0;
	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	for (ACitizen* citizen : Faction.Citizens)
		value += (citizen->HealthComponent->GetHealth() + citizen->AttackComponent->Damage);

	for (ACitizen* citizen : Faction.Rebels)
		value -= (citizen->HealthComponent->GetHealth() + citizen->AttackComponent->Damage);

	if (!gamemode->Enemies.IsEmpty()) {
		AAI* enemy = gamemode->Enemies[0];

		value -= (gamemode->Enemies.Num() * enemy->HealthComponent->GetHealth() + gamemode->Enemies.Num() * enemy->AttackComponent->Damage);
	}

	return value;
}

TTuple<bool, bool> UDiplomacyComponent::IsWarWinnable(FFactionStruct Faction, FFactionStruct& Target)
{
	int32 factionStrength = GetStrengthScore(Faction);
	int32 targetStrength = GetStrengthScore(Faction);

	int32 total = FMath::Max(factionStrength + targetStrength, 1.0f);

	bool bFactionCanWin = false;
	bool bTargetCanWin = false;

	if (factionStrength / total > 0.4f)
		bFactionCanWin = true;

	if (targetStrength / total > 0.4f)
		bTargetCanWin = true;

	return TTuple<bool, bool>(bFactionCanWin, bTargetCanWin);
}

void UDiplomacyComponent::Peace(FFactionStruct Faction1, FFactionStruct Faction2)
{
	FFactionStruct* f1 = GetFactionPtr(Faction1);
	FFactionStruct* f2 = GetFactionPtr(Faction2);

	f1->AtWar.Remove(f2->Name);
	f2->AtWar.Remove(f1->Name);
}

void UDiplomacyComponent::Ally(FFactionStruct Faction1, FFactionStruct Faction2)
{
	FFactionStruct* f1 = GetFactionPtr(Faction1);
	FFactionStruct* f2 = GetFactionPtr(Faction2);

	f1->Allies.Add(f2->Name);
	f2->Allies.Add(f1->Name);
}

void UDiplomacyComponent::BreakAlliance(FFactionStruct Faction1, FFactionStruct Faction2)
{
	FFactionStruct* f1 = GetFactionPtr(Faction1);
	FFactionStruct* f2 = GetFactionPtr(Faction2);

	f1->Allies.Remove(f2->Name);
	f2->Allies.Remove(f1->Name);
}

void UDiplomacyComponent::DeclareWar(FFactionStruct Faction1, FFactionStruct Faction2)
{
	FFactionStruct* f1 = GetFactionPtr(Faction1);
	FFactionStruct* f2 = GetFactionPtr(Faction2);

	f1->AtWar.Add(f2->Name);
	f2->AtWar.Add(f1->Name);

	if (Faction1.Name == Camera->ColonyName)
		for (int32 i = 0; i < Faction2.Armies.Num(); i++)
			Camera->SetArmyWidgetUI(Faction2.Name, Faction2.Armies[i].WidgetComponent->GetWidget(), i);
	else if (Faction2.Name == Camera->ColonyName)
		for (int32 i = 0; i < Faction1.Armies.Num(); i++)
			Camera->SetArmyWidgetUI(Faction1.Name, Faction1.Armies[i].WidgetComponent->GetWidget(), i);
}

void UDiplomacyComponent::Insult(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionStruct* faction = GetFactionPtr(Faction);
	int32 index = GetHappinessWithFaction(Target, Faction);

	faction->Happiness[index].SetValue("Insulted", -12);
}

void UDiplomacyComponent::Praise(FFactionStruct Faction, FFactionStruct Target)
{
	FFactionStruct* faction = GetFactionPtr(Faction);
	int32 index = GetHappinessWithFaction(Target, Faction);

	faction->Happiness[index].SetValue("Praised", 12);
}

void UDiplomacyComponent::Gift(FFactionStruct Faction, TArray<FGiftStruct> Gifts)
{
	FFactionStruct* faction = GetFactionPtr(Faction);

	for (FGiftStruct gift : Gifts) {
		if (!Camera->ResourceManager->TakeUniversalResource(faction, gift.Resource, gift.Amount, 0))
			continue;

		FFactionStruct playerFaction = Camera->ConquestManager->GetFactionFromName(Camera->ColonyName);

		int32 index = GetHappinessWithFaction(Faction, playerFaction);

		int32 value = Camera->ResourceManager->GetMarketValue(gift.Resource) * gift.Amount / 12;

		if (faction->Happiness[index].Contains("Received a gift"))
			faction->Happiness[index].SetValue("Received a gift", faction->Happiness[index].GetValue("Received a gift") + value);
		else
			faction->Happiness[index].SetValue("Received a gift", value);
	}
}