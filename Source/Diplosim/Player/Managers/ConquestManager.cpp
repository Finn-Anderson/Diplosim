#include "Player/Managers/ConquestManager.h"

#include "Components/WidgetComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Misc/ScopeTryLock.h"

#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ResearchManager.h"
#include "Player/Components/BuildComponent.h"
#include "Map/Grid.h"
#include "AI/Citizen.h"
#include "AI/Clone.h"
#include "AI/AttackComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Universal/HealthComponent.h"
#include "Buildings/Misc/Portal.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/House.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Buildings/Work/Service/Trader.h"
#include "Buildings/Misc/Road.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	AINum = 2;
	MaxAINum = 2;
	FailedBuild = 0;
}

void UConquestManager::BeginPlay()
{
	Super::BeginPlay();
	
	Camera = Cast<ACamera>(GetOwner());
}

FFactionStruct UConquestManager::InitialiseFaction(FString Name)
{
	FFactionStruct faction;
	faction.Name = Name;

	faction.ResearchStruct = Camera->ResearchManager->InitResearchStruct;

	faction.Politics.Parties = Camera->CitizenManager->InitParties;
	faction.Politics.Laws = Camera->CitizenManager->InitLaws;
	faction.Events = Camera->CitizenManager->InitEvents;

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		FFactionResourceStruct factionResource;
		factionResource.Type = resource.Type;

		faction.Resources.Add(factionResource);
	}

	return faction;
}

void UConquestManager::CreatePlayerFaction()
{
	Factions.Add(InitialiseFaction(Camera->ColonyName));
}

void UConquestManager::FinaliseFactions(ABroch* EggTimer)
{
	Factions[0].Name = Camera->ColonyName;
	Factions[0].EggTimer = EggTimer;
	EggTimer->FactionName = Camera->ColonyName;

	TArray<FString> factions;

	FString factionNames;
	FFileHelper::LoadFileToString(factionNames, *(FPaths::ProjectDir() + "/Content/Custom/Colony/ColonyNames.txt"));

	TArray<FString> factionsParsed;
	factionNames.ParseIntoArray(factionsParsed, TEXT(","));

	for (int32 i = factionsParsed.Num() - 1; i > -1; i--)
		if (factions.Contains(factionsParsed[i]))
			factions.RemoveAt(i);

	TArray<FTileStruct*> validLocations;

	for (int32 i = 0; i < AINum; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, factionsParsed.Num() - 1);
		factions.Add(factionsParsed[index]);

		if (factionsParsed[index] != "")
			factionsParsed.RemoveAt(index);
	}

	for (FString name : factions) {
		FFactionStruct f = InitialiseFaction(name);

		FActorSpawnParameters params;
		params.bNoFail = true;

		ABroch* eggTimer = GetWorld()->SpawnActor<ABroch>(EggTimer->GetClass(), FVector::Zero(), FRotator::ZeroRotator, params);

		for (TArray<FTileStruct*> tiles : Camera->Grid->ValidMineralTiles) {
			FTileStruct* tile = tiles[0];

			eggTimer->SetActorLocation(Camera->Grid->GetTransform(tile).GetLocation());

			if (!Camera->BuildComponent->IsValidLocation(eggTimer))
				continue;

			bool bTooCloseToAnotherFaction = false;

			for (FFactionStruct& faction : Factions) {
				double dist = FVector::Dist(faction.EggTimer->GetActorLocation(), eggTimer->GetActorLocation());

				if (dist > 3000.0f)
					continue;

				bTooCloseToAnotherFaction = true;

				break;
			}

			if (!bTooCloseToAnotherFaction)
				validLocations.Add(tile);
		}

		int32 index = Camera->Grid->Stream.RandRange(0, validLocations.Num() - 1);
		FTileStruct* chosenTile = validLocations[index];

		eggTimer->SetActorLocation(Camera->Grid->GetTransform(chosenTile).GetLocation());
		eggTimer->FactionName = f.Name;

		f.EggTimer = eggTimer;

		Factions.Add(f);
	}

	for (FFactionStruct& faction : Factions) {
		faction.EggTimer->SpawnCitizens();

		Camera->CitizenManager->Election(&faction);

		SetFactionFlagColour(&faction);
		SetFactionCulture(&faction);

		InitialiseTileLocationDistances(&faction);
	}

	Camera->SetFactionsInDiplomacyUI();
}

ABuilding* UConquestManager::DoesFactionContainUniqueBuilding(FString FactionName, TSubclassOf<class ABuilding> BuildingClass)
{
	FFactionStruct faction = GetFactionFromName(FactionName);

	for (ABuilding* building : faction.Buildings) {
		if (!building->IsA(BuildingClass))
			continue;

		return building;
	}

	return nullptr;
}

int32 UConquestManager::GetFactionIndexFromName(FString FactionName)
{
	for (int32 i = 0; i < Factions.Num(); i++) {
		if (Factions[i].Name != FactionName)
			continue;

		return i;
	}

	UE_LOGFMT(LogTemp, Fatal, "Faction not found");
}

FFactionStruct UConquestManager::GetFactionFromName(FString FactionName)
{
	return Factions[GetFactionIndexFromName(FactionName)];
}

TArray<ACitizen*> UConquestManager::GetCitizensFromFactionName(FString FactionName)
{
	return GetFactionFromName(FactionName).Citizens;
}

void UConquestManager::SetFactionFlagColour(FFactionStruct* Faction)
{
	FTileStruct* tile = Camera->Grid->GetTileFromLocation(Faction->EggTimer->GetActorLocation());
	auto bound = Camera->Grid->GetMapBounds();

	Faction->FlagColour = FLinearColor(tile->X / bound, FMath::Abs(tile->Y / bound - 1.0f), tile->Level / Camera->Grid->MaxLevel);
}

UTexture2D* UConquestManager::GetTextureFromCulture(FString Party, FString Religion)
{
	UTexture2D* texture = nullptr;

	for (FCultureImageStruct culture : CultureTextureList) {
		if (culture.Party != Party || culture.Religion != Religion)
			continue;

		texture = culture.Texture;

		break;
	}
		
	return texture;
}

FPoliticsStruct UConquestManager::GetFactionPoliticsStruct(FString FactionName)
{
	FFactionStruct faction = GetFactionFromName(FactionName);

	return faction.Politics;
}

TArray<FResearchStruct> UConquestManager::GetFactionResearch(FString FactionName)
{
	FFactionStruct faction = GetFactionFromName(FactionName);

	return faction.ResearchStruct;
}

FFactionStruct UConquestManager::GetFactionFromActor(AActor* Actor)
{
	FFactionStruct faction;

	for (FFactionStruct f : Factions) {
		if (!DoesFactionContainActor(faction.Name, Actor))
			continue;

		faction = f;

		break;
	}

	return faction;
}

bool UConquestManager::DoesFactionContainActor(FString FactionName, AActor* Actor)
{
	if (Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->FactionName == "")
		return true;

	FFactionStruct faction = GetFactionFromName(FactionName);

	return faction.Buildings.Contains(Actor) || faction.Citizens.Contains(Actor) || faction.Clones.Contains(Actor) || faction.Rebels.Contains(Actor);
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

		SetFactionCulture(&faction);

		Camera->UpdateFactionHappiness();

		EvaluateDiplomacy(faction);

		EvaluateAIBuild(&faction);

		EvaluateAIArmy(&faction);
	}

	for (FFactionStruct faction : FactionsToRemove) {
		for (FFactionStruct& f : Factions) {
			if (f == faction)
				continue;

			for (int32 i = f.Happiness.Num() - 1; i > -1; i--)
				if (f.Happiness[i].Owner == faction.Name)
					f.Happiness.RemoveAt(i);
		}

		int32 index = GetFactionIndexFromName(faction.Name);

		Camera->RemoveFactionBtn(index);

		Factions.RemoveAt(index);
	}
}

FFactionStruct UConquestManager::GetCitizenFaction(ACitizen* Citizen)
{
	FFactionStruct faction;

	for (FFactionStruct f : Factions) {
		if (!f.Citizens.Contains(Citizen))
			continue;

		faction = f;

		break;
	}

	return faction;
}

FFactionStruct* UConquestManager::GetFaction(FString Name, AActor* Actor)
{
	FFactionStruct* faction = nullptr;

	for (FFactionStruct& f : Factions) {
		if (f.Name != Name && !f.Citizens.Contains(Actor) && !f.Buildings.Contains(Actor) && !f.Rebels.Contains(Actor))
			continue;

		faction = &f;

		break;
	}

	return faction;
}

//
// Diplomacy
//
void UConquestManager::SetFactionCulture(FFactionStruct* Faction)
{
	TMap<FString, int32> partyCount;

	for (ACitizen* Citizen : Faction->Politics.Representatives) {
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

	TTuple<FString, int32> biggestReligion;

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

	Camera->UpdateFactionIcons(Factions.Find(*Faction));
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

		for (ABuilding* building : Factions[i].Buildings) {
			TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(building, 3.0f);

			for (FHitResult hit : hits) {
				if (!hit.GetActor()->IsA<ABuilding>() || Cast<ABuilding>(hit.GetActor())->FactionName != Faction.Name)
					continue;

				bTooClose = true;

				break;
			}
		}

		if (!bTooClose && Factions[i].Happiness[index].Contains(f.Name + "is too close to settlement"))
			Factions[i].Happiness[index].RemoveValue(f.Name + "is too close to settlement");
		else if (bTooClose)
			Factions[i].Happiness[index].SetValue(f.Name + "is too close to settlement", -6);
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

int32 UConquestManager::GetStrengthScore(FFactionStruct Faction)
{
	int32 value = 0;

	for (ACitizen* citizen : Faction.Citizens)
		value += (citizen->HealthComponent->GetHealth() + citizen->AttackComponent->Damage);

	for (ACitizen* citizen : Faction.Rebels)
		value -= (citizen->HealthComponent->GetHealth() + citizen->AttackComponent->Damage);

	if (!Camera->CitizenManager->Enemies.IsEmpty()) {
		AAI* enemy = Camera->CitizenManager->Enemies[0];

		value -= (Camera->CitizenManager->Enemies.Num() * enemy->HealthComponent->GetHealth() + Camera->CitizenManager->Enemies.Num() * enemy->AttackComponent->Damage);
	}

	return value;
}

TTuple<bool, bool> UConquestManager::IsWarWinnable(FFactionStruct Faction, FFactionStruct& Target)
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

	if (Faction1.Name == Camera->ColonyName)
		for (FArmyStruct army : Faction2.Armies)
			Camera->SetArmyWidgetUI(Faction2.Name, army.WidgetComponent->GetWidget());
	else if (Faction2.Name == Camera->ColonyName)
		for (FArmyStruct army : Faction1.Armies)
			Camera->SetArmyWidgetUI(Faction1.Name, army.WidgetComponent->GetWidget());
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
		if (!Camera->ResourceManager->TakeUniversalResource(GetFaction(Faction.Name), gift.Resource, gift.Amount, 0))
			continue;

		int32 i = Factions.Find(Faction);

		FFactionStruct playerFaction = GetFactionFromName(Camera->ColonyName);

		int32 index = GetHappinessWithFaction(Faction, playerFaction);

		int32 value = Camera->ResourceManager->GetMarketValue(gift.Resource) * gift.Amount / 12;

		if (Factions[i].Happiness[index].Contains("Received a gift"))
			Factions[i].Happiness[index].SetValue("Received a gift", Factions[i].Happiness[index].GetValue("Received a gift") + value);
		else
			Factions[i].Happiness[index].SetValue("Received a gift", value);
	}
}

//
// Tile Distance Calulation
//

double UConquestManager::CalculateTileDistance(FVector EggTimerLocation, FVector TileLocation)
{
	UNavigationSystemV1* nav = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	FNavLocation targetLoc;
	nav->ProjectPointToNavigation(EggTimerLocation, targetLoc, FVector(20.0f, 20.0f, 20.0f));

	FNavLocation ownerLoc;
	nav->ProjectPointToNavigation(TileLocation, ownerLoc, FVector(20.0f, 20.0f, 20.0f));

	double length;
	nav->GetPathLength(GetWorld(), targetLoc.Location, ownerLoc.Location, length);

	return length;
}

void UConquestManager::InitialiseTileLocationDistances(FFactionStruct* Faction)
{
	for (TArray<FTileStruct>& row : Camera->Grid->Storage) {
		for (FTileStruct& tile : row) {
			if (tile.bMineral || tile.bRamp)
				continue;

			FTransform transform = Camera->Grid->GetTransform(&tile);

			if (Faction->Citizens[0]->AIController->CanMoveTo(transform.GetLocation()))
				Faction->AccessibleBuildLocations.Add(transform.GetLocation(), CalculateTileDistance(Faction->EggTimer->GetActorLocation(), transform.GetLocation()));
			else	
				Faction->InaccessibleBuildLocations.Add(transform.GetLocation());
		}
	}

	RemoveTileLocations(Faction, Faction->EggTimer);

	SortTileDistances(Faction);
}

void UConquestManager::RecalculateTileLocationDistances(FFactionStruct* Faction)
{
	for (int32 i = Faction->InaccessibleBuildLocations.Num() - 1; i > -1; i--) {
		FVector location = Faction->InaccessibleBuildLocations[i];

		if (!Faction->Citizens[0]->AIController->CanMoveTo(location))
			continue;

		Faction->AccessibleBuildLocations.Add(location, CalculateTileDistance(Faction->EggTimer->GetActorLocation(), location));
		Faction->InaccessibleBuildLocations.RemoveAt(i);
	}

	FailedBuild = 0;

	SortTileDistances(Faction);
}

void UConquestManager::RemoveTileLocations(FFactionStruct* Faction, ABuilding* Building)
{
	TArray<FVector> roadLocations;

	TArray<FVector> locations;
	Faction->AccessibleBuildLocations.GenerateKeyArray(locations);

	for (FVector location : locations) {
		FVector hitLocation;
		Building->BuildingMesh->GetClosestPointOnCollision(location, hitLocation);

		double distance = FVector::Dist(location, hitLocation);

		if (distance < 100.0f) {
			Faction->AccessibleBuildLocations.Remove(location);

			if (distance > 40.0f)
				roadLocations.Add(location);
		}
	}

	FVector closestCurrentRoad = FVector::Zero();
	for (FVector location : Faction->RoadBuildLocations) {
		if (closestCurrentRoad == FVector::Zero()) {
			closestCurrentRoad = location;

			continue;
		}

		if (FVector::Dist(Building->GetActorLocation(), location) > FVector::Dist(Building->GetActorLocation(), closestCurrentRoad))
			continue;

		closestCurrentRoad = location;
	}

	FVector closestNewRoad = FVector::Zero();
	for (FVector location : roadLocations) {
		if (closestNewRoad == FVector::Zero()) {
			closestNewRoad = location;

			continue;
		}

		if (FVector::Dist(closestCurrentRoad, location) > FVector::Dist(closestCurrentRoad, closestNewRoad))
			continue;

		closestNewRoad = location;
	}

	FTileStruct* tile = Camera->Grid->GetTileFromLocation(closestNewRoad);

	roadLocations.Append(ConnectRoadTiles(Faction, tile, closestCurrentRoad));

	for (FVector location : roadLocations) {
		if (Faction->RoadBuildLocations.Contains(location))
			continue;

		Faction->RoadBuildLocations.Add(location);
	}

	SortTileDistances(Faction);
}

void UConquestManager::SortTileDistances(FFactionStruct* Faction)
{
	Faction->AccessibleBuildLocations.ValueSort([](double s1, double s2)
	{
		return s1 < s2;
	});
}

TArray<FVector> UConquestManager::ConnectRoadTiles(FFactionStruct* Faction, FTileStruct* Tile, FVector Location)
{
	TArray<FVector> locations;

	FTransform transform = Camera->Grid->GetTransform(Tile);

	if (transform.GetLocation() == Location)
		return locations;

	locations.Add(transform.GetLocation());

	FTileStruct* closestTile = nullptr;

	for (auto& element : Tile->AdjacentTiles) {
		if (closestTile == nullptr) {
			closestTile = element.Value;

			continue;
		}

		FTransform closestTransform = Camera->Grid->GetTransform(closestTile);
		FTransform newTransform = Camera->Grid->GetTransform(element.Value);

		if (FVector::Dist(Location, newTransform.GetLocation()) > FVector::Dist(Location, closestTransform.GetLocation()))
			continue;

		closestTile = element.Value;
	}

	locations.Append(ConnectRoadTiles(Faction, closestTile, Location));

	return locations;
}

//
// Building
//
void UConquestManager::EvaluateAIBuild(FFactionStruct* Faction)
{
	int32 numBuildings = Faction->Buildings.Num();

	AITrade(Faction);

	BuildFirstBuilder(Faction);

	for (ABuilding* building : Faction->RuinedBuildings)
		building->Rebuild(Faction->Name);

	BuildAIBuild(Faction);

	BuildAIHouse(Faction);

	BuildAIRoads(Faction);
	
	if (Faction->Buildings.Num() > numBuildings)
		RecalculateTileLocationDistances(Faction);
	else
		FailedBuild++;

	BuildAIAccessibility(Faction);
}

void UConquestManager::AITrade(FFactionStruct* Faction)
{
	ATrader* trader = nullptr;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<ATrader>())
			continue;

		trader = Cast<ATrader>(building);

		break;
	}

	if (!IsValid(trader) || !trader->Orders.IsEmpty())
		return;

	FQueueStruct order;

	for (FFactionResourceStruct resourceStruct : Faction->Resources) {
		if (resourceStruct.Type == Camera->ResourceManager->Money)
			continue;

		int32 amount = Camera->ResourceManager->GetResourceAmount(Faction->Name, resourceStruct.Type);

		if (amount <= 100)
			continue;

		FItemStruct item;
		item.Resource = resourceStruct.Type;
		item.Amount = amount - 100;

		order.SellingItems.Add(item);
	}

	trader->SetNewOrder(order);
}

void UConquestManager::BuildFirstBuilder(FFactionStruct* Faction)
{
	bool bContainsBuilder = false;

	for (ABuilding* building : Faction->Buildings) {
		if (!building->IsA<ABuilder>())
			continue;

		bContainsBuilder = true;

		break;
	}

	if (bContainsBuilder) {
		for (FAIBuildStruct aibuild : AIBuilds) {
			if (!aibuild.Building->GetDefaultObject()->IsA<ABuilder>())
				continue;

			AIBuild(Faction, aibuild.Building, nullptr);

			return;
		}
	}
}

void UConquestManager::BuildAIBuild(FFactionStruct* Faction)
{
	int32 count = 0;

	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	for (FResourceStruct resource : Camera->ResourceManager->ResourceList) {
		if (resource.Category != "Food")
			continue;

		if (count == 2)
			break;

		int32 trend = Camera->ResourceManager->GetCategoryTrend(Faction->Name, resource.Category);

		if (trend >= 0)
			continue;

		TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResourcesFromCategory(resource.Category);

		for (TSubclassOf<AResource> r : resources) {
			TArray<TSubclassOf<ABuilding>> buildings;
			Camera->ResourceManager->GetBuildings(r).GenerateKeyArray(buildings);

			buildingsClassList.Append(buildings);
		}
	}

	if (buildingsClassList.IsEmpty()) {
		for (FAIBuildStruct& aibuild : AIBuilds) {
			if (Faction->Citizens.Num() < aibuild.NumCitizens || aibuild.CurrentAmount == aibuild.Limit || !AICanAfford(Faction, aibuild.Building))
				continue;

			if (Camera->Grid->Stream.RandRange(1, 100) < 50) {
				aibuild.NumCitizens *= 1.1f;

				continue;
			}

			if (aibuild.Building->GetDefaultObject()->IsA<AInternalProduction>()) {
				AInternalProduction* internalBuilding = Cast<AInternalProduction>(aibuild.Building->GetDefaultObject());
				
				if (!internalBuilding->Intake.IsEmpty()) {
					bool bMakesResources = true;

					for (FItemStruct item : internalBuilding->Intake) {
						int32 trend = Camera->ResourceManager->GetResourceTrend(Faction->Name, item.Resource);

						if (trend > 0.0f)
							continue;

						bMakesResources = false;

						break;
					}

					if (!bMakesResources)
						continue;
				}
			}

			buildingsClassList.Add(aibuild.Building);
		}
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UConquestManager::BuildAIHouse(FFactionStruct* Faction)
{
	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	bool bHomeless = false;

	for (ACitizen* citizen : Faction->Citizens) {
		if (IsValid(citizen->Building.House))
			continue;

		bHomeless = true;

		break;
	}

	if (!bHomeless)
		return;

	for (TSubclassOf<ABuilding> house : Houses) {
		if (!AICanAfford(Faction, house))
			continue;

		buildingsClassList.Add(house);
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UConquestManager::BuildAIRoads(FFactionStruct* Faction)
{
	if (Faction->RoadBuildLocations.IsEmpty())
		return;

	int32 amount = Camera->ResourceManager->GetResourceAmount(Faction->Name, Camera->ResourceManager->Money);

	if (amount <= 100)
		return;

	int32 cost = Cast<ABuilding>(RoadClass->GetDefaultObject())->CostList[0].Amount;
	int32 numRoads = FMath::CeilToInt((amount - 100.0f) / cost);

	for (int32 i = 0; i < numRoads; i++) {
		if (Faction->RoadBuildLocations.IsEmpty())
			return;

		AIBuild(Faction, RoadClass, nullptr);
	}
}

void UConquestManager::BuildMiscBuild(FFactionStruct* Faction)
{
	if (Camera->Grid->Stream.RandRange(1, 100) != 100)
		return;

	TArray<TSubclassOf<ABuilding>> buildingsClassList;

	for (TSubclassOf<ABuilding> misc : MiscBuilds) {
		if (!AICanAfford(Faction, misc))
			continue;

		buildingsClassList.Add(misc);
	}

	ChooseBuilding(Faction, buildingsClassList);
}

void UConquestManager::BuildAIAccessibility(FFactionStruct* Faction)
{
	bool bCanAffordRamp = AICanAfford(Faction, RampClass);
	bool bCanAffordRoad = AICanAfford(Faction, RoadClass);

	if (FailedBuild <= 3 || Faction->AccessibleBuildLocations.Num() > 50 || (!bCanAffordRamp && !bCanAffordRoad))
		return;

	TSubclassOf<ABuilding> chosenClass = nullptr;

	FTileStruct* chosenTile = nullptr;
	FRotator rotation = FRotator::ZeroRotator;

	if (bCanAffordRamp) {
		for (auto& element : Faction->AccessibleBuildLocations) {
			if (chosenTile != nullptr && FVector::Dist(Faction->EggTimer->GetActorLocation(), element.Key) > FVector::Dist(Faction->EggTimer->GetActorLocation(), Camera->Grid->GetTransform(chosenTile).GetLocation()))
				continue;

			FTileStruct* tile = Camera->Grid->GetTileFromLocation(element.Key);
			bool bValid = false;

			for (auto& e : tile->AdjacentTiles) {
				if (!Faction->InaccessibleBuildLocations.Contains(Camera->Grid->GetTransform(e.Value).GetLocation()))
					continue;

				bValid = true;
			}

			if (!bValid)
				continue;

			chosenTile = tile;
		}
	}

	if (chosenTile == nullptr) {
		if (!bCanAffordRoad)
			return;

		chosenClass = RoadClass;
	}
	else {
		chosenClass = RampClass;
	}

	AIBuild(Faction, chosenClass, nullptr, true, chosenTile);
}

void UConquestManager::ChooseBuilding(FFactionStruct* Faction, TArray<TSubclassOf<ABuilding>> BuildingsClasses)
{
	if (BuildingsClasses.IsEmpty())
		return;

	int32 chosenIndex = Camera->Grid->Stream.RandRange(0, BuildingsClasses.Num() - 1);
	TSubclassOf<ABuilding> chosenBuildingClass = BuildingsClasses[chosenIndex];

	TSubclassOf<AResource> resource = nullptr;

	FAIBuildStruct aibuild;
	aibuild.Building = chosenBuildingClass;

	int32 i = AIBuilds.Find(aibuild);

	if (i != INDEX_NONE)
		resource = AIBuilds[i].Resource;

	AIBuild(Faction, chosenBuildingClass, resource);
}

bool UConquestManager::AIValidBuildingLocation(FFactionStruct* Faction, ABuilding* Building, float Extent, FVector Location)
{
	if (!Faction->Citizens[0]->AIController->CanMoveTo(Location) || !Camera->BuildComponent->IsValidLocation(Building, Extent, Location))
		return false;

	return true;
}

bool UConquestManager::AICanAfford(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, int32 Amount)
{
	TArray<FItemStruct> items = Cast<ABuilding>(BuildingClass->GetDefaultObject())->CostList;

	if (Amount > 1)
		for (FItemStruct& item : items)
			item.Amount *= Amount;

	for (FItemStruct item : items) {
		int32 maxAmount = Camera->ResourceManager->GetResourceAmount(Faction->Name, item.Resource);

		if (maxAmount < item.Amount)
			return false;
	}

	return true;
}

void UConquestManager::AIBuild(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, TSubclassOf<AResource> Resource, bool bAccessibility, FTileStruct* Tile)
{
	FActorSpawnParameters spawnParams;
	spawnParams.bNoFail = true;

	FRotator rotation = FRotator(0.0f, FMath::RoundHalfFromZero(Camera->Grid->Stream.FRandRange(0.0f, 270.0f) / 90.0f) * 90.0f, 0.0f);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0.0f, 0.0f, -1000.0f), FRotator::ZeroRotator, spawnParams);
	building->FactionName = Faction->Name;

	for (int32 i = 0; i < building->Seeds.Num(); i++) {
		if (Resource != nullptr) {
			if (building->Seeds[i].Resource != Resource)
				continue;

			building->SetSeed(i);

			break;
		}
		else if (!building->Seeds[i].Cost.IsEmpty()) {
			bool bCanAfford = true;

			for (FItemStruct item : building->Seeds[i].Cost)
				if (item.Amount > Camera->ResourceManager->GetResourceAmount(Faction->Name, item.Resource))
					bCanAfford = false;

			if (bCanAfford)
				building->SetSeed(i);
		}
		else {
			building->SetSeed(Camera->Grid->Stream.RandRange(0, building->Seeds.Num() - 1));

			break;
		}
	}

	FVector size = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;
	float extent = size.X / (size.X + 100.0f);
	float yExtent = size.Y / (size.Y + 100.0f);

	if (yExtent > extent)
		extent = yExtent;

	FVector location = FVector(10000000000.0f);
	FVector offset = building->BuildingMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 1000.0f);

	if (bAccessibility) {
		if (BuildingClass == RoadClass) {
			for (int32 i = 0; i < Camera->Grid->HISMRiver->GetInstanceCount(); i++) {
				FTransform transform;
				Camera->Grid->HISMRiver->GetInstanceTransform(i, transform);

				bool bClose = false;
				for (ABuilding* b : Faction->Buildings) {
					if (FVector::Dist(b->GetActorLocation(), transform.GetLocation()) > 500.0f)
						continue;

					bClose = true;
				}

				FTileStruct* tile = Camera->Grid->GetTileFromLocation(transform.GetLocation());

				if (AIValidBuildingLocation(Faction, building, 2.0f, transform.GetLocation()))
					continue;

				Tile = tile;

				break;
			}
		}
		else {
			FTileStruct* chosenInaccessibleTile = nullptr;

			for (auto& e : Tile->AdjacentTiles) {
				if (!Faction->InaccessibleBuildLocations.Contains(Camera->Grid->GetTransform(e.Value).GetLocation()))
					continue;

				chosenInaccessibleTile = e.Value;

				break;
			}

			if (Tile->Level > chosenInaccessibleTile->Level) {
				rotation = (Camera->Grid->GetTransform(chosenInaccessibleTile).GetLocation() - Camera->Grid->GetTransform(Tile).GetLocation()).Rotation();

				Tile = chosenInaccessibleTile;
			}
			else
				rotation = (Camera->Grid->GetTransform(Tile).GetLocation() - Camera->Grid->GetTransform(chosenInaccessibleTile).GetLocation()).Rotation();
		}

		location = Camera->Grid->GetTransform(Tile).GetLocation();
	}
	if (building->IsA<AExternalProduction>()) {
		AExternalProduction* externalBuilding = Cast<AExternalProduction>(building);

		for (auto& element : externalBuilding->GetValidResources()) {
			for (int32 inst : element.Value) {
				FTransform transform;
				element.Key->ResourceHISM->GetInstanceTransform(inst, transform);

				transform.SetLocation(transform.GetLocation() + offset);

				if (FVector::Dist(Faction->EggTimer->GetActorLocation(), location) < FVector::Dist(Faction->EggTimer->GetActorLocation(), transform.GetLocation()) || !AIValidBuildingLocation(Faction, building, extent, transform.GetLocation()))
					continue;

				location = transform.GetLocation();
			}
		}
	}
	else if (building->IsA<AInternalProduction>() && Cast<AInternalProduction>(building)->ResourceToOverlap != nullptr) {
		AInternalProduction* internalBuilding = Cast<AInternalProduction>(building);

		TArray<FResourceHISMStruct> resourceStructs;
		resourceStructs.Append(Camera->Grid->MineralStruct);

		for (FResourceHISMStruct resourceStruct : resourceStructs) {
			if (internalBuilding->ResourceToOverlap != resourceStruct.ResourceClass)
				continue;

			for (int32 i = 0; i < resourceStruct.Resource->ResourceHISM->GetInstanceCount(); i++) {
				FTransform transform;
				resourceStruct.Resource->ResourceHISM->GetInstanceTransform(i, transform);

				if (FVector::Dist(Faction->EggTimer->GetActorLocation(), location) < FVector::Dist(Faction->EggTimer->GetActorLocation(), transform.GetLocation()) || !AIValidBuildingLocation(Faction, building, extent, transform.GetLocation()))
					continue;

				location = transform.GetLocation();
			}
		}
	}
	else if (building->IsA<ARoad>()) {
		int32 index = Camera->Grid->Stream.RandRange(0, Faction->RoadBuildLocations.Num() - 1);
		
		location = Faction->RoadBuildLocations[index];

		Faction->RoadBuildLocations.RemoveAt(index);
	}
	else {
		for (auto& element : Faction->AccessibleBuildLocations) {
			if (!AIValidBuildingLocation(Faction, building, extent, element.Key))
				continue;

			location = element.Key;

			break;
		}
	}

	if (location == FVector(10000000000.0f)) {
		building->DestroyBuilding();

		return;
	}

	Camera->BuildComponent->GetBuildLocationZ(building, location);
	building->SetActorRotation(rotation);
	building->SetActorLocation(location);
	building->Build();

	if (building->IsA<ARoad>())
		Cast<ARoad>(building)->RegenerateMesh();

	RemoveTileLocations(Faction, building);

	if (Houses.Contains(BuildingClass) || MiscBuilds.Contains(BuildingClass) || RoadClass == BuildingClass || RampClass == BuildingClass)
		return;

	FAIBuildStruct aibuild;
	aibuild.Building = BuildingClass;

	int32 i = AIBuilds.Find(aibuild);

	if (i == INDEX_NONE)
		return;

	AIBuilds[i].CurrentAmount++;
	AIBuilds[i].NumCitizens += AIBuilds[i].NumCitizensIncrement;
}

//
// Army
//
bool UConquestManager::CanJoinArmy(ACitizen* Citizen)
{
	FFactionStruct faction = GetCitizenFaction(Citizen);

	if (Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue(faction.Name, "Work Age") || !Citizen->WillWork() || IsCitizenInAnArmy(Citizen))
		return false;

	return true;
}

void UConquestManager::CreateArmy(FString FactionName, TArray<ACitizen*> Citizens)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = GetFaction(FactionName);

	FArmyStruct army;
	int32 index = faction->Armies.Add(army);

	AddToArmy(index, Citizens);

	UWidgetComponent* widgetComponent = NewObject<UWidgetComponent>(this, UWidgetComponent::StaticClass());
	widgetComponent->SetupAttachment(Camera->Grid->GetRootComponent());
	widgetComponent->SetRelativeLocation(Camera->GetTargetActorLocation(Citizens[0]) + FVector(0.0f, 0.0f, 300.0f));
	widgetComponent->SetWidgetSpace(EWidgetSpace::World);
	widgetComponent->SetDrawSize(FVector2D(0.0f));
	widgetComponent->SetWidgetClass(ArmyUI);
	widgetComponent->RegisterComponent();

	faction->Armies[index].WidgetComponent = widgetComponent;

	Camera->SetArmyWidgetUI(FactionName, widgetComponent->GetWidget());
}

void UConquestManager::AddToArmy(int32 Index, TArray<ACitizen*> Citizens)
{
	if (Citizens.IsEmpty())
		return;

	FFactionStruct* faction = GetFaction("", Citizens[0]);

	faction->Armies[Index].Citizens.Append(Citizens);

	int32 index = Camera->Grid->Stream.RandRange(0, faction->AtWar.Num() - 1);
	ABuilding* target = MoveArmyMember(GetFaction(faction->AtWar[index]), Citizens[0], true);

	for (ACitizen* citizen : Citizens)
		citizen->AIController->AIMoveTo(target);

	Camera->UpdateArmyCountUI(Index, faction->Armies[Index].Citizens.Num());
}

void UConquestManager::RemoveFromArmy(ACitizen* Citizen)
{
	FFactionStruct* faction = GetFaction("", Citizen);

	for (int32 i = faction->Armies.Num() - 1; i > -1; i--) {
		if (!faction->Armies[i].Citizens.Contains(Citizen))
			continue;

		faction->Armies[i].Citizens.Remove(Citizen);

		Camera->UpdateArmyCountUI(i, faction->Armies[i].Citizens.Num());

		if (faction->Armies[i].Citizens.IsEmpty())
			faction->Armies.RemoveAt(i);

		break;
	}
}

bool UConquestManager::IsCitizenInAnArmy(ACitizen* Citizen)
{
	FFactionStruct faction = GetCitizenFaction(Citizen);

	for (FArmyStruct army : faction.Armies)
		if (army.Citizens.Contains(Citizen))
			return true;

	return false;
}

void UConquestManager::DestroyArmy(FString FactionName, int32 Index)
{
	FFactionStruct* faction = GetFaction(FactionName);

	faction->Armies[Index].WidgetComponent->DestroyComponent();
	faction->Armies.RemoveAt(Index);
}

void UConquestManager::EvaluateAIArmy(FFactionStruct* Faction)
{
	if (Faction->AtWar.IsEmpty()) {
		if (!Faction->Armies.IsEmpty())
			for (int32 i = Faction->Armies.Num() - 1; i > -1; i--)
				DestroyArmy(Faction->Name, i);

		return;
	}

	int32 currentCitizens = Faction->Citizens.Num();
	int32 enemyCitizens = 0;

	for (FString name : Faction->AtWar) {
		FFactionStruct* f = GetFaction(name);
		enemyCitizens += f->Citizens.Num();
	}

	int32 diff = currentCitizens - enemyCitizens;

	TArray<ACitizen*> availableCitizens;
	TArray<ACitizen*> chosenCitizens;

	for (ACitizen* citizen : Faction->Citizens) {
		if (!CanJoinArmy(citizen))
			continue;

		availableCitizens.Add(citizen);
	}

	for (int32 i = 0; i < diff; i++) {
		int32 index = Camera->Grid->Stream.RandRange(0, availableCitizens.Num() - 1);

		chosenCitizens.Add(availableCitizens[index]);
		availableCitizens.RemoveAt(index);
	}

	if (currentCitizens > enemyCitizens) {
		if (Faction->Armies.Num() == 5 || (diff < 20 && !Faction->Armies.IsEmpty())) {
			int32 index = Camera->Grid->Stream.RandRange(0, Faction->Armies.Num() - 1);

			AddToArmy(index, chosenCitizens);
		}
		else {
			CreateArmy(Faction->Name, chosenCitizens);
		}
	}

	for (FArmyStruct army : Faction->Armies) {
		for (ACitizen* citizen : army.Citizens) {
			if (IsValid(citizen->AIController->MoveRequest.GetGoalActor()) || !citizen->AttackComponent->OverlappingEnemies.IsEmpty())
				continue;

			int32 index = Camera->Grid->Stream.RandRange(0, Faction->AtWar.Num() - 1);

			MoveArmyMember(GetFaction(Faction->AtWar[index]), citizen);
		}
	}
}

ABuilding* UConquestManager::MoveArmyMember(FFactionStruct* Faction, AAI* AI, bool bReturn)
{
	ABuilding* target = nullptr;

	for (ABuilding* building : Faction->Buildings) {
		if (!IsValid(building) || building->HealthComponent == 0 || !AI->AIController->CanMoveTo(building->GetActorLocation()))
			continue;

		if (!IsValid(target)) {
			target = building;

			if (building->IsA<ABroch>())
				break;
			else
				continue;
		}

		int32 targetValue = 1.0f;
		int32 buildingValue = 1.0f;

		if (target->IsA<ABroch>())
			targetValue = 5.0f;

		if (building->IsA<ABroch>())
			buildingValue = 5.0f;

		double magnitude = AI->AIController->GetClosestActor(400.0f, AI->MovementComponent->Transform.GetLocation(), target->GetActorLocation(), building->GetActorLocation(), true, targetValue, buildingValue);

		if (magnitude > 0.0f)
			target = building;
	}

	if (bReturn)
		return target;
	else
		AI->AIController->AIMoveTo(target);

	return nullptr;
}

//
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}