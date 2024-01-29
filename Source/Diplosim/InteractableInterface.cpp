#include "InteractableInterface.h"

#include "HealthComponent.h"
#include "Buildings/Building.h"

void UInteractableComponent::SetHP()
{
	if (!TextStruct.Information.Contains("HP"))
		return;

	UHealthComponent* healthComp = GetOwner()->GetComponentByClass<UHealthComponent>();

	TextStruct.Information.Add("HP", FString::SanitizeFloat((float)healthComp->GetHealth() / healthComp->MaxHealth));
}

void UInteractableComponent::SetStorage()
{
	if (!TextStruct.Information.Contains("Storage"))
		return;

	ABuilding* building = Cast<ABuilding>(GetOwner());

	TextStruct.Information.Add("Storage", FString::SanitizeFloat((float)building->Storage / building->StorageCap));
}