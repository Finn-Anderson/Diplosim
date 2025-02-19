#pragma once

#include "CoreMinimal.h"
#include "Buildings/Work/Work.h"
#include "School.generated.h"

UCLASS()
class DIPLOSIM_API ASchool : public AWork
{
	GENERATED_BODY()
	
public:
	ASchool();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Students")
		TArray<class ACitizen*> Students;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 StudentCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
		int32 StudentMaxCapacity;

	UFUNCTION(BlueprintCallable)
		void AddStudentCapacity();

	UFUNCTION(BlueprintCallable)
		void RemoveStudentCapacity();

	int32 GetStudentCapacity();

	TArray<class ACitizen*> GetStudents();

	void AddStudent(class ACitizen* Citizen);

	void RemoveStudent(class ACitizen* Citizen);

	TArray<class ACitizen*> GetStudentsAtSchool();

	void AddProgress();
};
