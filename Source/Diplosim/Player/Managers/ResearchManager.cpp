#include "Player/Managers/ResearchManager.h"

#include "ImageUtils.h"

#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "AI/Citizen.h"

UResearchManager::UResearchManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	ReadJSONFile(FPaths::ProjectDir() + "/Content/Custom/Structs/Research.json");
}

void UResearchManager::ReadJSONFile(FString Path)
{
	TSharedPtr<FJsonObject> jsonObject = MakeShareable(new FJsonObject());

	FString fileContents;
	FFileHelper::LoadFileToString(fileContents, *Path);
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(fileContents);

	if (!FJsonSerializer::Deserialize(jsonReader, jsonObject) || !jsonObject.IsValid())
		return;

	for (auto& element : jsonObject->Values) {
		for (auto& e : element.Value->AsArray()) {
			FResearchStruct research;

			for (auto& v : e->AsObject()->Values) {
				uint8 index = 0;

				if (v.Value->Type == EJson::Array)
					for (auto& ev : v.Value->AsArray())
						for (auto& bev : ev->AsObject()->Values)
								research.Modifiers.Add(bev.Key, FCString::Atof(*bev.Value->AsString()));
				else if (v.Value->Type == EJson::String)
					if (v.Key == "Name")
						research.ResearchName = v.Value->AsString();
					else
						research.Texture = FImageUtils::ImportFileAsTexture2D(FPaths::ProjectDir() + v.Value->AsString());
				else {
					int32 value = FCString::Atoi(*v.Value->AsString());

					if (v.Key == "Target")
						research.Target = value;
					else
						research.MaxLevel = value;
				}
			}

			InitResearchStruct.Add(research);
		}
	}
}

bool UResearchManager::IsResearched(FString Name, FString FactionName)
{
	if (FactionName == "")
		return false;

	FResearchStruct r;
	r.ResearchName = Name;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	int32 index = faction->ResearchStruct.Find(r);

	return faction->ResearchStruct[index].Level == faction->ResearchStruct[index].MaxLevel;
}

bool UResearchManager::IsMaxResearched(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	return faction->ResearchStruct[Index].Level + GetAmountResearchIndexInQueue(Index, FactionName) == faction->ResearchStruct[Index].MaxLevel;
}

bool UResearchManager::IsBeingResearched(int32 Index, FString FactionName)
{
	int32 currentIndex = GetCurrentResearchIndex(FactionName);

	if (currentIndex == Index)
		return true;

	return false;
}

int32 UResearchManager::GetCurrentResearchIndex(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	return faction->ResearchIndices.IsEmpty() ? INDEX_NONE : faction->ResearchIndices[0];
}

FResearchStruct UResearchManager::GetCurrentResearch(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	FResearchStruct defaultStruct;
	defaultStruct.Texture = DefaultTexture;

	return faction->ResearchIndices.IsEmpty() ? defaultStruct : faction->ResearchStruct[faction->ResearchIndices[0]];
}

FResearchStruct UResearchManager::GetResearchFromIndex(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	return faction->ResearchStruct[Index];
}

int32 UResearchManager::GetAmountResearchIndexInQueue(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	int32 amount = 0;

	for (int32 i : faction->ResearchIndices)
		if (i == Index)
			amount++;

	return amount;
}

TArray<int32> UResearchManager::GetQueueList(FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	
	return faction->ResearchIndices;
}

void UResearchManager::GetResearchAmount(int32 Index, FString FactionName, float& Amount, int32& Target)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	Amount = faction->ResearchStruct[Index].AmountResearched;
	Target = faction->ResearchStruct[Index].Target;
}

int32 UResearchManager::GetResearchLevel(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	return faction->ResearchStruct[Index].Level;
}

void UResearchManager::SetResearch(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->ResearchIndices.Add(Index);

	if (FactionName == Camera->ColonyName)
		Camera->SetCurrentResearchUI();
}

void UResearchManager::RemoveResearch(int32 Index, FString FactionName, bool bLast, int32 QueueIndex)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (QueueIndex != INDEX_NONE) {
		faction->ResearchIndices.RemoveAt(QueueIndex);
	}
	else {
		int32 i = INDEX_NONE;

		if (!bLast)
			i = faction->ResearchIndices.Find(Index);
		else
			i = faction->ResearchIndices.FindLast(Index);

		if (i == INDEX_NONE)
			return;

		faction->ResearchIndices.RemoveAt(i);
	}

	if (FactionName == Camera->ColonyName)
		Camera->SetCurrentResearchUI();
}

void UResearchManager::ReorderResearch(int32 OldIndex, int32 NewIndex, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (NewIndex < 0 || NewIndex > faction->ResearchIndices.Num() - 1)
		return;

	faction->ResearchIndices.Swap(OldIndex, NewIndex);

	if (FactionName == Camera->ColonyName)
		Camera->SetCurrentResearchUI();
}

void UResearchManager::Research(float Amount, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (faction->ResearchIndices.IsEmpty())
		return;

	int32 currentIndex = faction->ResearchIndices[0];

	FResearchStruct* research = &faction->ResearchStruct[currentIndex];
	
	research->AmountResearched += Amount;

	if (research->AmountResearched < research->Target) {
		if (FactionName == Camera->ColonyName)
			Camera->SetCurrentResearchUI();

		return;
	}

	research->Level++;
	research->AmountResearched = 0;

	if (research->Level != research->MaxLevel)
		research->Target *= 1.25f;

	RemoveResearch(currentIndex, FactionName, false);

	for (auto& element : research->Modifiers)
		for (ACitizen* citizen : faction->Citizens)
			citizen->ApplyToMultiplier(element.Key, element.Value);

	if (FactionName == Camera->ColonyName)
		Camera->NotifyLog("Good", "Research Complete: " + research->ResearchName, faction->Name);
}