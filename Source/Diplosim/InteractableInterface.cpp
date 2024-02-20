#include "InteractableInterface.h"

#include "Kismet/GameplayStatics.h"

#include "HealthComponent.h"
#include "Buildings/Building.h"
#include "AI/Citizen.h"
#include "Map/Mineral.h"
#include "Player/Camera.h"

UInteractableComponent::UInteractableComponent()
{
	SetTickableWhenPaused(true);
}

void UInteractableComponent::BeginPlay()
{
	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();
}

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

void UInteractableComponent::SetEnergy()
{
	if (!TextStruct.Information.Contains("Energy"))
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	TextStruct.Information.Add("Energy", FString::SanitizeFloat((float)citizen->Energy / 100.0f));
}

void UInteractableComponent::SetHunger()
{
	if (!TextStruct.Information.Contains("Hunger"))
		return;

	ACitizen* citizen = Cast<ACitizen>(GetOwner());

	TextStruct.Information.Add("Hunger", FString::SanitizeFloat((float)citizen->Hunger / 100.0f));
}

void UInteractableComponent::SetOccupied()
{
	if (!TextStruct.Information.Contains("Occupied"))
		return;

	ABuilding* building = Cast<ABuilding>(GetOwner());

	TextStruct.Citizens = building->GetOccupied();

	TextStruct.Information.Add("Occupied", FString::SanitizeFloat((float)building->GetCapacity()));
}

void UInteractableComponent::ExecuteEditEvent(FString Key)
{
	if (Camera->SelectedInteractableComponent != this)
		return;

	Camera->EditInteractableText(Key, *TextStruct.Information.Find(Key));
}