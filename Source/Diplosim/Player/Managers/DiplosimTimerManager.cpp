#include "Player/Managers/DiplosimTimerManager.h"

#include "Misc/ScopeTryLock.h"

#include "AI/AI.h"
#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/BuildingComponent.h"
#include "Buildings/Work/Service/Orphanage.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Player/Camera.h"
#include "Player/Components/SaveGameComponent.h"
#include "Universal/HealthComponent.h"
#include "Universal/DiplosimGameModeBase.h"

UDiplosimTimerManager::UDiplosimTimerManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDiplosimTimerManager::TimerLoop(ACamera* Camera)
{
	if (Timers.IsEmpty()) {
		if (Camera->SaveGameComponent->IsLoading())
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Timer);

		return;
	}

	Async(EAsyncExecution::TaskGraph, [this, Camera]() {
		FScopeTryLock loopLock(&TimerLoopLock);
		if (!loopLock.IsLocked())
			return;

		if (Camera->SaveGameComponent->IsLoading()) {
			Camera->SaveGameComponent->LoadGameCallback(EAsyncLoop::Timer);

			return;
		}

		for (auto node = Timers.GetHead(); node != nullptr; node = node->GetNextNode()) {
			if (Camera->SaveGameComponent->IsLoading())
				return;

			FTimerStruct& timer = node->GetValue();

			if (!IsValid(this))
				return;

			if (!IsValid(timer.Actor)) {
				FScopeLock lock(&TimerLock);
				Timers.RemoveNode(node);

				continue;
			}

			if (timer.bPaused)
				continue;

			double currentTime = GetWorld()->GetTimeSeconds();

			timer.Timer += (currentTime - timer.LastUpdateTime);
			timer.LastUpdateTime = currentTime;

			if (timer.Timer >= timer.Target && !timer.bModifying) {
				if (timer.bOnGameThread) {
					timer.bModifying = true;

					Async(EAsyncExecution::TaskGraphMainTick, [this, &timer]() { CallTimerFunction(&timer); });
				}
				else {
					timer.bModifying = true;

					CallTimerFunction(&timer);
				}
			}

			if (!timer.bDone)
				continue;

			if (timer.bRepeat) {
				timer.Timer = 0;
				timer.bModifying = false;
				timer.bDone = false;
			}
			else {
				FScopeLock lock(&TimerLock);
				Timers.RemoveNode(node);
			}
		}
	});
}

void UDiplosimTimerManager::CreateTimer(FString Identifier, AActor* Actor, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread)
{
	FScopeLock lock(&TimerLock);

	FTimerStruct timer;
	timer.CreateTimer(Identifier, Actor, Time, FunctionName, Params, Repeat, OnGameThread);
	timer.LastUpdateTime = GetWorld()->GetTimeSeconds();

	Timers.AddTail(timer);
}

FTimerStruct* UDiplosimTimerManager::FindTimer(FString ID, AActor* Actor)
{
	FScopeLock lock(&TimerLock);

	if (!IsValid(Actor))
		return nullptr;

	FTimerStruct timer;
	timer.ID = ID;
	timer.Actor = Actor;

	auto node = Timers.FindNode(timer);

	if (node == nullptr)
		return nullptr;

	return &node->GetValue();
}

void UDiplosimTimerManager::RemoveTimer(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	timer->Actor = nullptr;
}

void UDiplosimTimerManager::ResetTimer(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	timer->Timer = 0;
}

void UDiplosimTimerManager::UpdateTimerLength(FString ID, AActor* Actor, int32 NewTarget)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return;

	timer->Target = NewTarget;
}

int32 UDiplosimTimerManager::GetElapsedTime(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return 0;

	return timer->Target - timer->Timer;
}

float UDiplosimTimerManager::GetElapsedPercentage(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return 0.0f;

	return (timer->Target - timer->Timer) / timer->Target;
}

bool UDiplosimTimerManager::DoesTimerExist(FString ID, AActor* Actor)
{
	FTimerStruct* timer = FindTimer(ID, Actor);

	if (timer == nullptr)
		return false;

	return true;
}

#define SET_OBJECT(object) \
	function = object->FindFunction(Timer->FuncName); \
	if (function != nullptr) \
		return TTuple<UObject*, UFunction*>(object, function);

TTuple<UObject*, UFunction*> UDiplosimTimerManager::GetFunction(FTimerStruct* Timer)
{
	UFunction* function = nullptr;

	SET_OBJECT(Timer->Actor);

	if (Timer->Actor->IsA<AAI>()) {
		AAI* ai = Cast<AAI>(Timer->Actor);

		SET_OBJECT(ai->AIController);

		if (ai->IsA<ACitizen>() && IsValid(Cast<ACitizen>(ai)->BuildingComponent->Orphanage)) {
			SET_OBJECT(Cast<ACitizen>(ai)->BuildingComponent->Orphanage);
		}
	}

	UHealthComponent* healthComp = Timer->Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp) {
		SET_OBJECT(healthComp);
	}

	if (Timer->Actor->IsA<AGrid>()) {
		AGrid* grid = Cast<AGrid>(Timer->Actor);

		SET_OBJECT(grid->AtmosphereComponent);
		SET_OBJECT(grid->AtmosphereComponent->Clouds);
		SET_OBJECT(grid->AtmosphereComponent->NaturalDisasterComponent);
	}

	if (Timer->Actor->IsA<ACamera>()) {
		for (UActorComponent* component : Cast<ACamera>(Timer->Actor)->GetComponents()) {
			SET_OBJECT(component);
		}

		SET_OBJECT(Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode()));
	}

	UE_LOGFMT(LogTemp, Fatal, "Function '{name}' not found with '{actor}'", Timer->FuncName.ToString(), Timer->Actor->GetName());
}

#define SET_TYPE(type_) \
	if (type == #type_) \
		*functionProperty->ContainerPtrToValuePtr<type_>(buffer) = GetParameter<type_>(Timer, count);

void UDiplosimTimerManager::CallTimerFunction(FTimerStruct* Timer)
{
	TTuple<UObject*, UFunction*> objFunc = GetFunction(Timer);

	int32 count = 0;

	uint8* buffer = (uint8*)FMemory_Alloca(objFunc.Value->ParmsSize);
	FMemory::Memzero(buffer, objFunc.Value->ParmsSize);

	for (TFieldIterator<FProperty> it(objFunc.Value); it && it->HasAnyPropertyFlags(CPF_Parm); ++it) {
		FProperty* functionProperty = *it;
		FString type = functionProperty->GetCPPType();

		// Actors
		SET_TYPE(AActor*);
		SET_TYPE(ACamera*);
		SET_TYPE(ABuilding*);
		SET_TYPE(ACitizen*);

		// Objects
		SET_TYPE(USoundBase*);
		SET_TYPE(UHierarchicalInstancedStaticMeshComponent*);

		// Rest
		SET_TYPE(FVector);
		SET_TYPE(TArray<FVector>);
		SET_TYPE(FLinearColor);
		SET_TYPE(bool);
		SET_TYPE(FFactionStruct);
		SET_TYPE(int32);
		SET_TYPE(float);
		SET_TYPE(FString);
		SET_TYPE(FGuid);
		SET_TYPE(FLawStruct);
		SET_TYPE(TSubclassOf<AResource>);

		count++;
	}

	FFrame stack(objFunc.Key, objFunc.Value, buffer, NULL, objFunc.Value->ChildProperties);

	const bool bHasReturnParam = objFunc.Value->ReturnValueOffset != MAX_uint16;
	uint8* ReturnValueAddress = bHasReturnParam ? (buffer + objFunc.Value->ReturnValueOffset) : nullptr;

	objFunc.Value->Invoke(objFunc.Key, stack, nullptr);

	Timer->bDone = true;
}