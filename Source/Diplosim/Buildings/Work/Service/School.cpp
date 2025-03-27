#include "Buildings/Work/Service/School.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

ASchool::ASchool()
{
	Capacity = 3;
	MaxCapacity = 3;
	
	Space = 30;
	MaxSpace = 50;
}

void ASchool::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->Building.School = this;

	Visitor->PayForEducationLevels();

	Super::AddVisitor(Occupant, Visitor);
}

void ASchool::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->Building.School = nullptr;

	Super::RemoveVisitor(Occupant, Visitor);
}

TArray<ACitizen*> ASchool::GetStudentsAtSchool()
{
	TArray<ACitizen*> citizens;

	for (ACitizen* occupant : GetOccupied()) {
		for (ACitizen* student : GetVisitors(occupant)) {
			if (student->Building.BuildingAt != this)
				continue;

			citizens.Add(student);
		}
	}

	return citizens;
}

void ASchool::AddProgress()
{
	TArray<ACitizen*> citizens = GetStudentsAtSchool();

	float efficiency = GetOccupied().Num() * (1.0f - (0.5f * ((citizens.Num() - 1) / (MaxSpace - 1))));

	for (ACitizen* citizen : GetCitizensAtBuilding())
		efficiency *= citizen->GetProductivity();

	for (int32 i = citizens.Num(); i > -1; i--) {
		ACitizen* citizen = citizens[i];

		citizen->BioStruct.EducationProgress += (50 * efficiency);

		if (citizen->BioStruct.EducationProgress >= 100) {
			float progress = citizen->BioStruct.EducationProgress / 100.0f;
			int32 levels = FMath::FloorToInt(progress);

			progress = progress - levels;

			citizen->BioStruct.EducationProgress = progress * 100.0f;

			citizen->BioStruct.EducationLevel = FMath::Clamp(citizen->BioStruct.EducationLevel + levels, 0, citizen->BioStruct.PaidForEducationLevel);

			if (citizen->BioStruct.EducationLevel == 5 || !citizen->CanAffordEducationLevel())
				RemoveVisitor(GetOccupant(citizen), citizen);
		}
	}
 }