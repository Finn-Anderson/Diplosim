#include "Buildings/Misc/Special/GMLab.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "AI/Citizen.h"

AGMLab::AGMLab()
{
	TimeLength = 100.0f;
}

void AGMLab::Production(ACitizen* Citizen)
{
	Super::Production(Citizen);

	for (ACitizen* citizen : Camera->CitizenManager->Citizens) {
		for (FGeneticsStruct& genetic : citizen->Genetics) {
			if (genetic.Grade == EGeneticsGrade::Good)
				continue;

			genetic.Grade = EGeneticsGrade::Good;

			return;
		}
	}
}