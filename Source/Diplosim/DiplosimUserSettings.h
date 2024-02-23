#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "DiplosimUserSettings.generated.h"

UCLASS()
class DIPLOSIM_API UDiplosimUserSettings : public UGameUserSettings
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(Config)
		int32 MaxEnemies;

	UPROPERTY(Config)
		int32 MaxCitizens;

	UPROPERTY(Config)
		bool bRenderClouds;

public:
	UFUNCTION(BlueprintCallable)
		static UDiplosimUserSettings* GetDiplosimUserSettings();

	UFUNCTION(BlueprintCallable, Category = "Enemies")
		void SetMaxEnemies(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Enemies")
		int32 GetMaxEnemies() const;

	UFUNCTION(BlueprintCallable, Category = "Citizens")
		void SetMaxCitizens(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Citizens")
		int32 GetMaxCitizens() const;

	UFUNCTION(BlueprintCallable, Category = "Clouds")
		void SetRenderClouds(bool Value);

	UFUNCTION(BlueprintPure, Category = "Clouds")
		bool GetRenderClouds() const;

	class AClouds* Clouds;
};
