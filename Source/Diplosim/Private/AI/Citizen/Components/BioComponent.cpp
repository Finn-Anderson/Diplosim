#include "AI/Citizen/Components/BioComponent.h"

#include "AI/AIMovementComponent.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/House.h"
#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/PoliticsManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Universal/HealthComponent.h"

UBioComponent::UBioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Mother = nullptr;
	Father = nullptr;

	Partner = nullptr;
	HoursTogetherWithPartner = 0;
	bMarried = false;

	Sex = ESex::NaN;
	Sexuality = ESexuality::NaN;

	Age = 0;
	Name = "Citizen";

	EducationLevel = 0;
	EducationProgress = 0;
	PaidForEducationLevel = 0;

	bAdopted = false;

	SpeedBeforeOld = 100.0f;
	MaxHealthBeforeOld = 100.0f;
}

void UBioComponent::Birthday()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	Age++;

	if (Age == 5)
		citizen->GivePersonalityTrait(Mother.Get());
	else if (Age == 10)
		citizen->GivePersonalityTrait(Father.Get());

	citizen->BuildingComponent->RemoveFromHouse();

	if (Age > 50) {
		float ratio = FMath::Clamp(FMath::Pow(FMath::LogX(50.0f, Age - 50.0f), 3.0f), 0.0f, 1.0f);
		float odds = ratio * 90.0f;

		citizen->HealthComponent->MaxHealth = MaxHealthBeforeOld / (ratio * 5.0f);
		citizen->HealthComponent->AddHealth(0);

		citizen->MovementComponent->InitialSpeed = SpeedBeforeOld / (ratio * 2.0f);

		int32 chance = citizen->Camera->Stream.RandRange(1, 100);

		if (chance < odds)
			citizen->HealthComponent->TakeHealth(citizen->HealthComponent->MaxHealth, citizen);
	}
	else if (Age == 50) {
		MaxHealthBeforeOld = citizen->HealthComponent->MaxHealth;
		SpeedBeforeOld = citizen->MovementComponent->InitialSpeed;
	}

	if (Age <= 18) {
		citizen->HealthComponent->MaxHealth += 5 * citizen->HealthComponent->HealthMultiplier;
		citizen->HealthComponent->AddHealth(5 * citizen->HealthComponent->HealthMultiplier);

		float multiply = 1.0f;
		if (citizen->Hunger <= 25)
			multiply = 0.75f;

		float scale = (Age * 0.04f) + 0.28f;

		citizen->MovementComponent->Transform.SetScale3D(FVector(scale, scale, scale) * FVector(multiply, multiply, 1.0f) * citizen->ReachMultiplier);
	}
	else if (Partner != nullptr && Sex == ESex::Female)
		AsyncTask(ENamedThreads::GameThread, [this]() { HaveChild(); });

	if (faction == nullptr)
		return;

	if (Age == 18) {
		SetSexuality(faction->Citizens);

		citizen->SetReligion(faction);
	}

	if (Age >= 18 && Partner == nullptr)
		FindPartner(faction);

	citizen->BuildingComponent->RemoveOnReachingWorkAge(faction);

	if (Age >= citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Vote Age"))
		citizen->SetPoliticalLeanings();
}

void UBioComponent::SetSex(TArray<ACitizen*> Citizens)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	int32 choice = citizen->Camera->Stream.RandRange(1, 100);

	float male = 0.0f;
	float total = 0.0f;

	for (AActor* actor : Citizens) {
		ACitizen* c = Cast<ACitizen>(actor);

		if (c == citizen)
			continue;

		if (c->BioComponent->Sex == ESex::Male)
			male += 1.0f;

		total += 1.0f;
	}

	float mPerc = 50.0f;

	if (total > 0)
		mPerc = (male / total) * 100.0f;

	if (choice > mPerc)
		Sex = ESex::Male;
	else
		Sex = ESex::Female;

	SetName();
}

void UBioComponent::SetName()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FString names;

	if (Sex == ESex::Male)
		FFileHelper::LoadFileToString(names, *(FPaths::ProjectDir() + "/Content/Custom/Citizen/MaleNames.txt"));
	else
		FFileHelper::LoadFileToString(names, *(FPaths::ProjectDir() + "/Content/Custom/Citizen/FemaleNames.txt"));

	TArray<FString> parsed;
	names.ParseIntoArray(parsed, TEXT(","));

	int32 index = citizen->Camera->Stream.RandRange(0, parsed.Num() - 1);

	Name = parsed[index];
}

void UBioComponent::SetSexuality(TArray<ACitizen*> Citizens)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (Citizens.Num() <= 10) {
		Sexuality = ESexuality::Straight;

		return;
	}

	int32 gays = 0;

	Citizens.Remove(citizen);

	for (ACitizen* c : Citizens)
		if (c->BioComponent->Sexuality != ESexuality::NaN && c->BioComponent->Sexuality != ESexuality::Straight)
			gays++;

	float percentageGay = gays / (float)Citizens.Num();

	if (percentageGay > 0.1f)
		return;

	int32 chance = citizen->Camera->Stream.RandRange(1, 100);

	if (chance > 90) {
		if (chance > 95) {
			Sexuality = ESexuality::Homosexual;

			citizen->Fertility *= 0.5f;
		}
		else {
			Sexuality = ESexuality::Bisexual;
		}
	}
	else {
		Sexuality = ESexuality::Straight;
	}
}

void UBioComponent::FindPartner(FFactionStruct* Faction)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	ACitizen* chosenCitizen = nullptr;
	int32 curCount = 1;

	for (ACitizen* c : Faction->Citizens) {
		if (!IsValid(c) || c == citizen || c->HealthComponent->GetHealth() == 0 || c->IsPendingKillPending() || c->BioComponent->Partner != nullptr || c->BioComponent->Age < 18)
			continue;

		int32 value = citizen->Camera->PoliticsManager->GetLawValue(Faction->Name, "Same-Sex Laws");

		if (((Sexuality == ESexuality::Straight || value == 0) && c->BioComponent->Sex == Sex) || (Sexuality != ESexuality::Straight && value != 0 && c->BioComponent->Sex != Sex))
			continue;

		int32 count = 0;

		for (FPersonality* personality : citizen->Camera->CitizenManager->GetCitizensPersonalities(citizen)) {
			for (FPersonality* p : citizen->Camera->CitizenManager->GetCitizensPersonalities(c)) {
				if (personality->Trait == p->Trait)
					count += 2;
				else if (personality->Likes.Contains(p->Trait))
					count++;
				else if (personality->Dislikes.Contains(p->Trait))
					count--;
			}
		}

		if (count < 1)
			continue;

		if (chosenCitizen == nullptr) {
			chosenCitizen = c;
			curCount = count;

			continue;
		}

		double magnitude = citizen->AIController->GetClosestActor(50.0f, citizen->MovementComponent->Transform.GetLocation(), chosenCitizen->MovementComponent->Transform.GetLocation(), c->MovementComponent->Transform.GetLocation(), true, curCount, count);

		if (magnitude <= 0.0f)
			continue;

		chosenCitizen = c;
	}

	if (chosenCitizen != nullptr) {
		SetPartner(chosenCitizen);

		chosenCitizen->BioComponent->SetPartner(citizen);

		citizen->BuildingComponent->SelectPreferredPartnersHouse(citizen, chosenCitizen);
	}
}

void UBioComponent::SetPartner(ACitizen* Citizen)
{
	Partner = Citizen;
	HoursTogetherWithPartner = 0;
}

void UBioComponent::RemoveMarriage()
{
	if (Partner == nullptr)
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	bMarried = false;
	citizen->HappinessComponent->DivorceHappiness = -20;

	Partner->BioComponent->bMarried = false;
	Partner->HappinessComponent->DivorceHappiness = -20;
}

void UBioComponent::RemovePartner()
{
	if (Partner == nullptr)
		return;

	RemoveMarriage();

	Partner->BioComponent->Partner = nullptr;
	Partner = nullptr;
}

void UBioComponent::IncrementHoursTogetherWithPartner()
{
	if (Partner == nullptr)
		return;

	HoursTogetherWithPartner++;
}

void UBioComponent::HaveChild()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	if (!IsValid(citizen->BuildingComponent->House) || Children.Num() >= citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Child Policy"))
		return;

	ACitizen* occupant = nullptr;

	if (IsValid(citizen->BuildingComponent->House)) {
		occupant = citizen->BuildingComponent->House->GetOccupant(citizen);

		if (citizen->BuildingComponent->House->GetVisitors(occupant).Num() == citizen->BuildingComponent->House->Space)
			return;
	}

	float chance = citizen->Camera->Stream.FRandRange(0.0f, 100.0f) * Partner->Fertility * citizen->Fertility;
	float passMark = FMath::LogX(60.0f, Age) * 100.0f;

	if (chance < passMark)
		return;

	FVector location = citizen->MovementComponent->Transform.GetLocation() + citizen->MovementComponent->Transform.GetRotation().Vector() * 10.0f;

	if (citizen->BuildingComponent->BuildingAt != nullptr)
		location = citizen->BuildingComponent->EnterLocation;

	FActorSpawnParameters params;
	params.bNoFail = true;

	FTransform transform;
	transform.SetLocation(location);
	transform.SetRotation(citizen->MovementComponent->Transform.GetRotation());
	transform.SetScale3D(FVector(0.28f));

	ACitizen* c = GetWorld()->SpawnActor<ACitizen>(citizen->GetClass(), FVector::Zero(), FRotator::ZeroRotator, params);
	citizen->Camera->Grid->AIVisualiser->AddInstance(c, citizen->Camera->Grid->AIVisualiser->HISMCitizen, transform);

	c->BioComponent->Mother = citizen;
	c->BioComponent->Father = Partner;

	c->BioComponent->SetSex(faction->Citizens);
	c->CitizenSetup(faction);

	if (IsValid(occupant))
		c->BuildingComponent->House->AddVisitor(occupant, c);

	citizen->Camera->NotifyLog("Good", c->BioComponent->Name + " is born", faction->Name);

	for (ACitizen* child : Children) {
		c->BioComponent->Siblings.Add(child);
		child->BioComponent->Siblings.Add(c);
	}

	Children.Add(c);
	Partner->BioComponent->Children.Add(c);
}

TArray<ACitizen*> UBioComponent::GetLikedFamily(bool bFactorAge)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());
	FFactionStruct* faction = citizen->Camera->ConquestManager->GetFaction("", citizen);

	TArray<ACitizen*> family;

	if (Mother != nullptr && Mother->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(Mother));

	if (Father != nullptr && Father->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(Father));

	if (Partner != nullptr && Partner->HealthComponent->GetHealth() != 0)
		family.Add(Cast<ACitizen>(Partner));

	for (ACitizen* child : Children)
		if (IsValid(child) && child->HealthComponent->GetHealth() != 0)
			family.Add(child);

	for (ACitizen* sibling : Siblings)
		if (IsValid(sibling) && sibling->HealthComponent->GetHealth() != 0)
			family.Add(sibling);

	if (bFactorAge&& Age < citizen->Camera->PoliticsManager->GetLawValue(faction->Name, "Work Age"))
		return family;

	for (int32 i = (family.Num() - 1); i > -1; i--) {
		int32 likeness = 0;

		citizen->Camera->CitizenManager->PersonalityComparison(citizen, family[i], likeness);

		if (likeness < 0)
			family.RemoveAt(i);
	}

	return family;
}

void UBioComponent::Disown()
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (Mother != nullptr) {
		Mother = nullptr;
		Mother->BioComponent->Children.Remove(citizen);
	}

	if (Father != nullptr) {
		Father = nullptr;
		Father->BioComponent->Children.Remove(citizen);
	}

	for (ACitizen* siblings : Siblings)
		siblings->BioComponent->Siblings.Remove(citizen);

	Siblings.Empty();
}

void UBioComponent::Adopt(ACitizen* Child)
{
	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	if (Sex == ESex::Female)
		Child->BioComponent->Mother = citizen;
	else
		Child->BioComponent->Father = citizen;

	if (Partner != nullptr) {
		if (Partner->BioComponent->Sex == ESex::Female)
			Child->BioComponent->Mother = Partner;
		else
			Child->BioComponent->Father = Partner;

		Partner->BioComponent->Children.Add(Child);
	}

	for (ACitizen* c : Children) {
		c->BioComponent->Siblings.Add(Child);
		Child->BioComponent->Siblings.Add(c);
	}

	Children.Add(Child);
}