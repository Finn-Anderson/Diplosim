#pragma once

#include "CoreMinimal.h"
#include "Universal/DiplosimUniversalTypes.h"
#include "Components/ActorComponent.h"
#include "DiplosimTimerManager.generated.h"

USTRUCT()
struct FTimerParameterStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		UObject* Object;

	UPROPERTY()
		FString ObjectName;

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		TArray<FVector> Locations;

	UPROPERTY()
		FLinearColor Colour;

	UPROPERTY()
		bool bStatus;

	UPROPERTY()
		FFactionStruct Faction;

	UPROPERTY()
		float Value;

	UPROPERTY()
		FString String;

	UPROPERTY()
		FGuid ID;

	UPROPERTY()
		FLawStruct Bill;

	UPROPERTY()
		EAttendStatus AttendStatus;

	UPROPERTY()
		TSubclassOf<AResource> Resource;

	FTimerParameterStruct()
	{
		Object = nullptr;
		ObjectName = "";
		Location = FVector::Zero();
		Colour = FLinearColor();
		bStatus = false;
		Value = -1001.23f;
		String = "wadaddwr";
		ID = FGuid();
		Bill = FLawStruct();
		AttendStatus = EAttendStatus::Neutral;
		Resource = nullptr;
	}
};

USTRUCT()
struct FTimerStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		FString ID;

	UPROPERTY()
		AActor* Actor;

	UPROPERTY()
		float Timer;

	UPROPERTY()
		float Target;

	UPROPERTY()
		FName FuncName;

	UPROPERTY()
		TArray<FTimerParameterStruct> Parameters;

	UPROPERTY()
		bool bRepeat;

	UPROPERTY()
		bool bOnGameThread;

	UPROPERTY()
		bool bPaused;

	UPROPERTY()
		bool bModifying;

	UPROPERTY()
		bool bDone;

	UPROPERTY()
		double LastUpdateTime;

	FTimerStruct()
	{
		ID = "";
		Actor = nullptr;
		Timer = 0;
		Target = 0;
		bRepeat = false;
		bOnGameThread = false;
		bPaused = false;
		bModifying = false;
		bDone = false;
		LastUpdateTime = 0.0f;

		FuncName = "";
		Parameters.Empty();
	}

	void CreateTimer(FString Identifier, AActor* Acta, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread = false)
	{
		ID = Identifier;
		Actor = Acta;
		Target = Time;
		FuncName = FunctionName;
		Parameters = Params;
		bRepeat = Repeat;
		bOnGameThread = OnGameThread;
	}

	bool operator==(const FTimerStruct& other) const
	{
		return (other.ID == ID && other.Actor == Actor);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DIPLOSIM_API UDiplosimTimerManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDiplosimTimerManager();

	void TimerLoop(class ACamera* Camera);

	void CreateTimer(FString Identifier, AActor* Actor, float Time, FName FunctionName, TArray<FTimerParameterStruct> Params, bool Repeat, bool OnGameThread = false);

	FTimerStruct* FindTimer(FString ID, AActor* Actor);

	void RemoveTimer(FString ID, AActor* Actor);

	void RemoveAllTimers(AActor* Actor);

	void ResetTimer(FString ID, AActor* Actor);

	void PauseTimer(FString ID, AActor* Actor, bool bPause);

	void UpdateTimerLength(FString ID, AActor* Actor, int32 NewTarget);

	UFUNCTION(BlueprintCallable)
		int32 GetElapsedTime(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		float GetElapsedPercentage(FString ID, AActor* Actor);

	UFUNCTION(BlueprintCallable)
		bool DoesTimerExist(FString ID, AActor* Actor);

	void CallTimerFunction(FTimerStruct* Timer);

	template<typename T>
	void SetParameter(T Value, TArray<FTimerParameterStruct>& Array)
	{
		FTimerParameterStruct param;

		if constexpr (std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
			param.Object = Value;
		else if constexpr (std::is_same_v<T, FVector>)
			param.Location = Value;
		else if constexpr (std::is_same_v<T, TArray<FVector>>)
			param.Locations = Value;
		else if constexpr (std::is_same_v<T, FLinearColor>)
			param.Colour = Value;
		else if constexpr (std::is_same_v<T, bool>)
			param.bStatus = Value;
		else if constexpr (std::is_same_v<T, FFactionStruct>)
			param.Faction = Value;
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, float>)
			param.Value = Value;
		else if constexpr (std::is_same_v<T, FString> || std::is_same_v<T, const char*>)
			param.String = Value;
		else if constexpr (std::is_same_v<T, FGuid>)
			param.ID = Value;
		else if constexpr (std::is_same_v<T, FLawStruct>)
			param.Bill = Value;
		else if constexpr (std::is_same_v<T, EAttendStatus>)
			param.AttendStatus = Value;
		else if constexpr (std::is_same_v<T, TSubclassOf<AResource>>)
			param.Resource;
		else
			static_assert(false, "Not a valid type");

		Array.Add(param);
	}

	TDoubleLinkedList<FTimerStruct> Timers;

private:
	TTuple<UObject*, UFunction*> GetFunction(FTimerStruct* Timer);

	template<typename T>
	T GetParameter(FTimerStruct* Timer, int32 Index)
	{
		if constexpr (std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
			return Cast<std::remove_pointer_t<T>>(Timer->Parameters[Index].Object);
		else if constexpr (std::is_same_v<T, FVector>)
			return Timer->Parameters[Index].Location;
		else if constexpr (std::is_same_v<T, TArray<FVector>>)
			return Timer->Parameters[Index].Locations;
		else if constexpr (std::is_same_v<T, FLinearColor>)
			return Timer->Parameters[Index].Colour;
		else if constexpr (std::is_same_v<T, bool>)
			return Timer->Parameters[Index].bStatus;
		else if constexpr (std::is_same_v<T, FFactionStruct>)
			return Timer->Parameters[Index].Faction;
		else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, float>)
			return Timer->Parameters[Index].Value;
		else if constexpr (std::is_same_v<T, FString>)
			return Timer->Parameters[Index].String;
		else if constexpr (std::is_same_v<T, FGuid>)
			return Timer->Parameters[Index].ID;
		else if constexpr (std::is_same_v<T, FLawStruct>)
			return Timer->Parameters[Index].Bill;
		else if constexpr (std::is_same_v<T, EAttendStatus>)
			return Timer->Parameters[Index].AttendStatus;
		else if constexpr (std::is_same_v<T, TSubclassOf<AResource>>)
			return Timer->Parameters[Index].Resource;
		else
			static_assert(false, "Not a valid type");
	}

	FCriticalSection TimerLoopLock;

	FCriticalSection TimerLock;
};
