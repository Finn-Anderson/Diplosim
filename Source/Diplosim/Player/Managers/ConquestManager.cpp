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
#include "Universal/HealthComponent.h"
#include "Buildings/Misc/Portal.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/House.h"
#include "Buildings/Work/Production/ExternalProduction.h"
#include "Buildings/Work/Production/InternalProduction.h"

UConquestManager::UConquestManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	AINum = 2;
	MaxAINum = 2;
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

		EvaluateAI(&faction);
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

bool UConquestManager::CanJoinArmy(class ACitizen* Citizen)
{
	FFactionStruct faction = GetCitizenFaction(Citizen);

	if (Camera->CitizenManager->Injured.Contains(Citizen) || Camera->CitizenManager->Infected.Contains(Citizen) || Citizen->BioStruct.Age < Camera->CitizenManager->GetLawValue(faction.Name, "Work Age") || !Citizen->WillWork())
		return false;

	return true;
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
			FTransform transform = Camera->Grid->GetTransform(&tile);

			if (Faction->Citizens[0]->AIController->CanMoveTo(transform.GetLocation()))
				Faction->AccessibleBuildLocations.Add(transform.GetLocation(), CalculateTileDistance(Faction->EggTimer->GetActorLocation(), transform.GetLocation()));
			else	
				Faction->InaccessibleBuildLocations.Add(transform.GetLocation());
		}
	}

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

	SortTileDistances(Faction);
}

void UConquestManager::RemoveTileLocations(FFactionStruct* Faction, ABuilding* Building)
{
	TArray<FVector> locations;
	Faction->AccessibleBuildLocations.GenerateKeyArray(locations);

	for (FVector location : locations) {
		FVector hitLocation;
		Building->BuildingMesh->GetClosestPointOnCollision(location, hitLocation);

		if (FVector::Dist(location, hitLocation) < 100.0f)
			Faction->AccessibleBuildLocations.Remove(location);
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

//
// AI
//
void UConquestManager::EvaluateAI(FFactionStruct* Faction)
{
	int32 numBuildings = Faction->Buildings.Num();

	BuildFirstBuilder(Faction);

	for (ABuilding* building : Faction->RuinedBuildings)
		building->Rebuild(Faction->Name);

	BuildAIBuild(Faction);

	BuildAIHouse(Faction);

	// Separate part for building roads.
	// For ramps, if height change when building roads, make ramp i.e. if adjacent to edge and end location is on different level from start location, make ramp facing edge.
	
	if (Faction->Buildings.Num() > numBuildings)
		RecalculateTileLocationDistances(Faction);

	// Evaluate army (separate function?)
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

	for (TSubclassOf<class ABuilding> house : Houses) {
		if (!AICanAfford(Faction, house))
			continue;

		buildingsClassList.Add(house);
	}

	ChooseBuilding(Faction, buildingsClassList);
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
	if (!Faction->Citizens[0]->AIController->CanMoveTo(Location) || !Camera->BuildComponent->IsValidLocation(Building, Location))
		return false;

	TArray<FHitResult> hits = Camera->BuildComponent->GetBuildingOverlaps(Building, Extent, Location);
	bool bHitBuilding = false;

	for (FHitResult hit : hits) {
		if (!hit.GetActor()->IsA<ABuilding>())
			continue;

		bHitBuilding = true;

		break;
	}

	return !bHitBuilding;
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

void UConquestManager::AIBuild(FFactionStruct* Faction, TSubclassOf<ABuilding> BuildingClass, TSubclassOf<AResource> Resource)
{
	FActorSpawnParameters spawnParams;
	spawnParams.bNoFail = true;

	FRotator rotation = FRotator(0.0f, FMath::RoundHalfFromZero(Camera->Grid->Stream.FRandRange(0.0f, 270.0f) / 90.0f) * 90.0f, 0.0f);

	ABuilding* building = GetWorld()->SpawnActor<ABuilding>(BuildingClass, FVector(0.0f, 0.0f, -1000.0f), FRotator::ZeroRotator, spawnParams);
	building->FactionName = Faction->Name;

	for (int32 i = 0; i < building->Seeds.Num(); i++) {
		if (building->Seeds[i].Resource != Resource)
			continue;

		building->SetSeed(i);
	}

	FVector size = building->BuildingMesh->GetStaticMesh()->GetBounds().GetBox().GetSize() / 2.0f;
	float extent = size.X / (size.X + 100.0f);
	float yExtent = size.Y / (size.Y + 100.0f);

	if (yExtent > extent)
		extent = yExtent;

	FVector location = FVector(10000000000.0f);
	FVector offset = building->BuildingMesh->GetComponentLocation() + FVector(0.0f, 0.0f, 1000.0f);

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

	building->SetActorLocation(location);

	RemoveTileLocations(Faction, building);

	if (Houses.Contains(BuildingClass))
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
// UI
//
void UConquestManager::DisplayConquestNotification(FString Message, FString Owner, bool bChoice)
{
	Camera->NotifyConquestEvent(Message, Owner, bChoice);
}