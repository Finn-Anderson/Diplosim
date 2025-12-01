#include "Player/Managers/ResearchManager.h"

#include "ImageUtils.h"

#include "Player/Camera.h"
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

	if (FJsonSerializer::Deserialize(jsonReader, jsonObject) && jsonObject.IsValid()) {
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
}

bool UResearchManager::IsReseached(FString Name, FString FactionName)
{
	if (FactionName == "")
		return false;
	
	FResearchStruct r;
	r.ResearchName = Name;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	int32 index = faction->ResearchStruct.Find(r);

	return faction->ResearchStruct[index].Level == faction->ResearchStruct[index].MaxLevel;
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

void UResearchManager::GetResearchAmount(int32 Index, FString FactionName, float& Amount, int32& Target)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	Amount = faction->ResearchStruct[Index].AmountResearched;
	Target = faction->ResearchStruct[Index].Target;
}

void UResearchManager::SetResearch(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->ResearchIndices.Add(Index);

	Camera->SetCurrentResearchUI();
}

void UResearchManager::RemoveResearch(int32 Index, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->ResearchIndices.RemoveAt(Index);

	Camera->SetCurrentResearchUI();
}

void UResearchManager::ReorderResearch(int32 OldIndex, int32 NewIndex, FString FactionName)
{
	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	faction->ResearchIndices.Swap(OldIndex, NewIndex);

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
		Camera->SetCurrentResearchUI();

		return;
	}

	faction->ResearchIndices.RemoveAt(0);
	Camera->SetCurrentResearchUI();

	research->Level++;

	if (research->Level != research->MaxLevel)
		research->Target *= 1.25f;
	else if (FactionName == Camera->ColonyName)
		Camera->ResearchComplete(currentIndex);

	for (auto& element : research->Modifiers)
		for (ACitizen* citizen : faction->Citizens)
			citizen->ApplyToMultiplier(element.Key, element.Value);

	if (FactionName == Camera->ColonyName)
		Camera->NotifyLog("Good", "Research Complete: " + research->ResearchName, faction->Name);
}