#include "Player/Managers/ResearchManager.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"
#include "ConquestManager.h"

UResearchManager::UResearchManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentIndex = INDEX_NONE;

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

					if (v.Value->Type == EJson::Array) {
						for (auto& ev : v.Value->AsArray()) {
							if (v.Key == "Dependants")
								research.Dependants.Add(ev->AsString());
							else
								for (auto& bev : ev->AsObject()->Values)
									research.Modifiers.Add(bev.Key, FCString::Atof(*bev.Value->AsString()));
						}
					}
					else if (v.Value->Type == EJson::String)
						research.ResearchName = v.Value->AsString();
					else
						research.Target = FCString::Atoi(*v.Value->AsString());
				}

				ResearchStruct.Add(research);
			}
		}
	}

}

bool UResearchManager::CanResearch(FResearchStruct Research)
{
	for (FString name : Research.Dependants) {
		FResearchStruct r;
		r.ResearchName = name;

		int32 index = ResearchStruct.Find(r);

		if (!ResearchStruct[index].bResearched)
			continue;

		return true;
	}

	return false;
}

bool UResearchManager::IsReseached(FString Name)
{
	FResearchStruct r;
	r.ResearchName = Name;
	
	int32 index = ResearchStruct.Find(r);

	return ResearchStruct[index].bResearched;
}

void UResearchManager::Research(float Amount)
{
	if (CurrentIndex == INDEX_NONE)
		return;
	
	ResearchStruct[CurrentIndex].AmountResearched += Amount;

	if (ResearchStruct[CurrentIndex].AmountResearched < ResearchStruct[CurrentIndex].Target)
		return;

	ACamera* camera = Cast<ACamera>(GetOwner());

	ResearchStruct[CurrentIndex].bResearched = true;

	for (auto& element : ResearchStruct[CurrentIndex].Modifiers) {
		for (ACitizen* citizen : camera->CitizenManager->Citizens)
			citizen->ApplyToMultiplier(element.Key, element.Value);

		for (FWorldTileStruct& tile : camera->ConquestManager->World) {
			if (!tile.bIsland || tile.Occupier.Owner != camera->ConquestManager->EmpireName || tile.bCapital)
				continue;
			
			for (ACitizen* citizen : tile.Citizens)
				citizen->ApplyToMultiplier(element.Key, element.Value);
		}
	}

	camera->ResearchComplete(CurrentIndex);
	camera->DisplayEvent(ResearchStruct[CurrentIndex].ResearchName, "Research Complete");

	CurrentIndex = INDEX_NONE;
}