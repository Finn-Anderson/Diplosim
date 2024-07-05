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
		bool bEnemies;

	UPROPERTY(Config)
		bool bRenderClouds;

public:
	UFUNCTION(BlueprintCallable)
		static UDiplosimUserSettings* GetDiplosimUserSettings();

	UFUNCTION(BlueprintCallable, Category = "Enemies")
		void SetSpawnEnemies(bool Value);

	UFUNCTION(BlueprintPure, Category = "Enemies")
		bool GetSpawnEnemies() const;

	UFUNCTION(BlueprintCallable, Category = "Clouds")
		void SetRenderClouds(bool Value);

	UFUNCTION(BlueprintPure, Category = "Clouds")
		bool GetRenderClouds() const;

	UPROPERTY()
		class AClouds* Clouds;

	UPROPERTY()
		class ADiplosimGameModeBase* GameMode;
};
