#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera.generated.h"

USTRUCT(BlueprintType)
struct FHoverStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
		AActor* Actor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
		USceneComponent* Component;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
		int32 Instance;

	FHoverStruct()
	{
		Reset();
	}

	void Reset()
	{
		Actor = nullptr;
		Component = nullptr;
		Instance = -1;
	}
};

UCLASS()
class DIPLOSIM_API ACamera : public APawn
{
	GENERATED_BODY()

public:
	ACamera();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintImplementableEvent)
		void SetInteractableText(AActor* Actor, int32 Instance);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateDisplayName();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateHealthIssues();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateDayText();

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayEvent(const FString& Descriptor, const FString& Event);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateResourceText(const FString& Category);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateUI();

	UFUNCTION(BlueprintImplementableEvent)
		void SaveSettings();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateResolutionText();

	UFUNCTION(BlueprintImplementableEvent)
		void LawPassed(bool bPassed, int32 For, int32 Against);

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayWarning(const FString& Warning);

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayNewBill();

	UFUNCTION(BlueprintImplementableEvent)
		void RefreshRepresentatives();

	UFUNCTION(BlueprintImplementableEvent)
		void SetSeedVisibility(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateTrends();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateSpeedUI(float Speed);

	UFUNCTION(BlueprintImplementableEvent)
		void ResearchComplete(int32 Index);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMapSeed(const FString& Seed);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMapSpecialBuildings();

	UFUNCTION(BlueprintImplementableEvent)
		void SetFactionsInDiplomacyUI();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateFactionIcons(int32 Index);

	UFUNCTION(BlueprintImplementableEvent)
		void RemoveFactionBtn(int32 Index);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateFactionHappiness();

	UFUNCTION(BlueprintImplementableEvent)
		void NotifyConquestEvent(const FString& Message, const FString& FactionName, bool bChoice);

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayNotifyLog(const FString& Type, const FString& Message, const FString& IslandName);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateWorkHours(class ABuilding* Building, int32 Index);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateLoadingText(const FString& Message);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Manager")
		class UResourceManager* ResourceManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Manager")
		class UConstructionManager* ConstructionManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Citizen Manager")
		class UCitizenManager* CitizenManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Research Manager")
		class UResearchManager* ResearchManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest Manager")
		class UConquestManager* ConquestManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UBuildComponent* BuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		class UCameraMovementComponent* MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
		class AGrid* Grid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString ColonyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		int32 CitizenNum;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Start")
		bool Start;

	UPROPERTY()
		bool bStartMenu;

	UPROPERTY()
		APlayerController* PController;

	void Tick(float DeltaTime) override;

	void OnBrochPlace(class ABuilding* Broch);

	UFUNCTION(BlueprintCallable)
		void Quit(bool bMenu);

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* InteractAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* MusicAudioComponent;

	void PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound, float Pitch = -1.0f);

	void SetInteractAudioSound(USoundBase* Sound, float Volume, float Pitch = 1.0f);

	void PlayInteractSound();

	// UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> MainMenuUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* MainMenuUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> BuildUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* BuildUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> PauseUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* PauseUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> MenuUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* MenuUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> LostUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* LostUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> SettingsUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* SettingsUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> WikiUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* WikiUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> EventUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* EventUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> WarningUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* WarningUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ParliamentUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* ParliamentUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> LawPassedUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* LawPassedUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> BribeUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* BribeUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> BuildingColourUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* BuildingColourUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ResearchUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* ResearchUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> ResearchHoverUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* ResearchHoverUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> DiplomacyUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* DiplomacyUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> HoursUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* HoursUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> GiftUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* GiftUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> DiplomacyNotifyUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* DiplomacyNotifyUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> LogUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* LogUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		class UWidgetComponent* WidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		FString WikiURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		class UUserWidget* HoveredWidget;

	UPROPERTY()
		bool bInMenu;

	UPROPERTY()
		bool bLost;

	UPROPERTY()
		class ACitizen* FocusedCitizen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		bool bMouseCapture;

	UPROPERTY()
		FVector2D MousePosition;

	UPROPERTY()
		FVector MouseHitLocation;

	UPROPERTY()
		FHoverStruct AttachedTo;

	UPROPERTY()
		float GameSpeed;

	UPROPERTY()
		bool bWasClosingWindow;

	UFUNCTION(BlueprintCallable)
		void SetMouseCapture(bool bCapture, bool bUI = false, bool bOverwrite = false);

	UFUNCTION(BlueprintCallable)
		void StartGame();

	void DisplayBuildUI();

	UFUNCTION(BlueprintCallable)
		void ShowWarning(FString Warning);

	UFUNCTION(BlueprintCallable)
		void NotifyLog(FString Type, FString Message, FString IslandName);

	UFUNCTION(BlueprintCallable)
		void ClearPopupUI();

	UFUNCTION(BlueprintCallable)
		void SetPause(bool bPause, bool bAlterViewport = true);

	UFUNCTION(BlueprintCallable)
		void SetGameSpeed(float Speed);

	void SetTimeDilation(float Dilation);

	UFUNCTION(BlueprintCallable)
		void DisplayInteract(AActor* Actor, USceneComponent* Component = nullptr, int32 Instance = -1);

	UFUNCTION(BlueprintCallable)
		void SetInteractStatus(AActor* Actor, bool bStatus, USceneComponent* Component = nullptr, int32 Instance = -1);

	void Attach(AActor* Actor, USceneComponent* Component = nullptr, int32 Instance = -1);

	void Detach();

	FVector GetTargetActorLocation(AActor* Actor);

	void Lose();

	// Smite
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics")
		TSubclassOf<class AResource> Crystal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
		class UNiagaraComponent* SmiteComponent;

	int32 Smites;

	UFUNCTION(BlueprintCallable)
		void Smite(class AAI* AI);

	void IncrementSmites(int32 Increment);

	UFUNCTION(BlueprintCallable)
		int32 GetSmiteCost();

	// Inputs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputMappingContext* MouseInputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputMappingContext* MovementInputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputLook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputLookChord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputScroll;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputCancel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputRender;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputPause;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputGameSpeed;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		bool bBulldoze;

	UPROPERTY()
		FHoverStruct HoveredActor;

	// Commands
	void Action(const struct FInputActionInstance& Instance);

	void Bulldoze();

	void Cancel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
		bool bQuick;

	// Map
	void NewMap();

	// Other
	UPROPERTY()
		bool bBlockPause;

	void Pause();

	UFUNCTION(BlueprintCallable, Category = "UI")
		void Menu();

	// Building
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<class ABuilding> StartBuilding;

	void Rotate(const struct FInputActionInstance& Instance);

	// Camera Movement
	void ActivateLook(const struct FInputActionInstance& Instance);

	void Look(const struct FInputActionInstance& Instance);

	void Move(const struct FInputActionInstance& Instance);
	
	void Speed(const struct FInputActionInstance& Instance);

	void Scroll(const struct FInputActionInstance& Instance);

	// Game Speed
	void IncrementGameSpeed(const struct FInputActionInstance& Instance);

	void ResetGameSpeedCounter();

	UPROPERTY()
		int32 GameSpeedCounter;

	// Debugging
	UFUNCTION(Exec)
		void SpawnEnemies();

	UFUNCTION(Exec)
		void AddEnemies(FString Category, int32 Amount);

	UFUNCTION(Exec)
		void ChangeSeasonAffect(FString Season);

	UFUNCTION(Exec)
		void CompleteResearch();

	UFUNCTION(Exec)
		void TurnOnInstantBuild(bool Value);

	UFUNCTION(Exec)
		void SpawnCitizen(int32 Amount, bool bAdult);

	UFUNCTION(Exec)
		void SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival = false);

	UFUNCTION(Exec)
		void DamageActor(int32 Amount);

	UPROPERTY()
		bool bInstantBuildCheat;

	UPROPERTY()
		bool bInstantEnemies;
};
