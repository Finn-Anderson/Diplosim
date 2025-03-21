#include "Buildings/Work/Service/School.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"

ASchool::ASchool()
{
	Capacity = 3;
	MaxCapacity = 3;
	
	StudentCapacity = 30.0f;
	StudentMaxCapacity = 50.0f;
}

void ASchool::AddStudentCapacity()
{
	if (StudentCapacity == StudentMaxCapacity)
		return;

	StudentCapacity++;
}

void ASchool::RemoveStudentCapacity()
{
	if (StudentCapacity == 0)
		return;

	StudentCapacity--;

	if (GetStudents().Num() <= GetStudentCapacity())
		return;

	RemoveStudent(GetStudents().Last());
}

int32 ASchool::GetStudentCapacity()
{
	return StudentCapacity;
}

TArray<ACitizen*> ASchool::GetStudents()
{
	return Students;
}

void ASchool::AddStudent(ACitizen* Citizen)
{
	Citizen->Building.School = this;

	Citizen->PayForEducationLevels();

	Citizen->AIController->DefaultAction();
}

void ASchool::RemoveStudent(ACitizen* Citizen)
{
	Citizen->Building.School = nullptr;

	Leave(Citizen);

	Citizen->AIController->DefaultAction();
}

TArray<ACitizen*> ASchool::GetStudentsAtSchool()
{
	TArray<ACitizen*> citizens;

	for (ACitizen* student : Students) {
		if (student->Building.BuildingAt != this)
			continue;

		citizens.Add(student);
	}

	return citizens;
}

void ASchool::AddProgress()
{
	TArray<ACitizen*> citizens = GetStudentsAtSchool();

	float efficiency = GetOccupied().Num() * (1.0f - (0.5f * ((citizens.Num() - 1) / (StudentMaxCapacity - 1))));

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
				RemoveStudent(citizen);
		}
	}
 }