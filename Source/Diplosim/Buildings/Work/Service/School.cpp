#include "Buildings/Work/Service/School.h"

#include "AI/Citizen.h"
#include "AI/BuildingComponent.h"
#include "AI/BioComponent.h"

ASchool::ASchool()
{
	Capacity = 3;
	
	Space = 30;
}

void ASchool::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->School = this;

	Visitor->PayForEducationLevels();

	Super::AddVisitor(Occupant, Visitor);
}

void ASchool::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	Visitor->BuildingComponent->School = nullptr;

	Super::RemoveVisitor(Occupant, Visitor);
}

TArray<ACitizen*> ASchool::GetStudentsAtSchool(ACitizen* Occupant)
{
	TArray<ACitizen*> citizens;

	for (ACitizen* student : GetVisitors(Occupant)) {
		if (student->BuildingComponent->BuildingAt != this)
			continue;

		citizens.Add(student);
	}

	return citizens;
}

void ASchool::AddProgress()
{
	TArray<ACitizen*> citizens;
	float efficiency = 0.0f;

	for (ACitizen* occupant : GetCitizensAtBuilding()) {
		TArray<ACitizen*> students = GetStudentsAtSchool(occupant);
		citizens.Append(students);

		efficiency += occupant->GetProductivity() - (0.5f * (students.Num() / Space));
	}

	efficiency /= GetOccupied().Num();

	for (int32 i = citizens.Num(); i > -1; i--) {
		ACitizen* citizen = citizens[i];

		citizen->BioComponent->EducationProgress += (50 * efficiency);

		if (citizen->BioComponent->EducationProgress >= 100) {
			float progress = citizen->BioComponent->EducationProgress / 100.0f;
			int32 levels = FMath::FloorToInt(progress);

			progress = progress - levels;

			citizen->BioComponent->EducationProgress = progress * 100.0f;

			citizen->BioComponent->EducationLevel = FMath::Clamp(citizen->BioComponent->EducationLevel + levels, 0, citizen->BioComponent->PaidForEducationLevel);

			if (citizen->BioComponent->EducationLevel == 5 || !citizen->CanAffordEducationLevel())
				RemoveVisitor(GetOccupant(citizen), citizen);
		}
	}
 }