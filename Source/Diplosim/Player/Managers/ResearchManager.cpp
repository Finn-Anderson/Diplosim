#include "Player/Managers/ResearchManager.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "AI/Citizen.h"
#include "ConquestManager.h"

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
						research.ResearchName = v.Value->AsString();
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

	ACamera* camera = Cast<ACamera>(GetOwner());

	FFactionStruct* faction = camera->ConquestManager->GetFaction(FactionName);
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
	ACamera* camera = Cast<ACamera>(GetOwner());
	FFactionStruct* faction = camera->ConquestManager->GetFaction(FactionName);

	return faction->ResearchIndex;
}

void UResearchManager::GetResearchAmount(int32 Index, FString FactionName, float& Amount, int32& Target)
{
	ACamera* camera = Cast<ACamera>(GetOwner());
	FFactionStruct* faction = camera->ConquestManager->GetFaction(FactionName);

	Amount = faction->ResearchStruct[Index].AmountResearched;
	Target = faction->ResearchStruct[Index].Target;
}

void UResearchManager::SetResearch(int32 Index, FString FactionName)
{
	ACamera* camera = Cast<ACamera>(GetOwner());

	FFactionStruct* faction = camera->ConquestManager->GetFaction(FactionName);

	faction->ResearchIndex = Index;
}

void UResearchManager::Research(float Amount, FString FactionName)
{
	ACamera* camera = Cast<ACamera>(GetOwner());

	FFactionStruct* faction = camera->ConquestManager->GetFaction(FactionName);
	int32 currentIndex = faction->ResearchIndex;

	if (currentIndex == INDEX_NONE)
		return;

	FResearchStruct* research = &faction->ResearchStruct[currentIndex];
	
	research->AmountResearched += Amount;

	if (research->AmountResearched < research->Target)
		return;

	research->Level++;

	if (research->Level != research->MaxLevel)
		research->Target *= 1.25f;
	else if (FactionName == camera->ColonyName)
		camera->ResearchComplete(currentIndex);

	for (auto& element : research->Modifiers)
		for (ACitizen* citizen : faction->Citizens)
			citizen->ApplyToMultiplier(element.Key, element.Value);

	if (FactionName == camera->ColonyName)
		camera->ShowEvent(research->ResearchName, "Research Complete");

	faction->ResearchIndex = INDEX_NONE;
}