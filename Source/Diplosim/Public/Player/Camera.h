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

UENUM()
enum class EInfoUpdate : uint8
{
	Party,
	Religion,
	Personality
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
		void UpdateResourceCapacityText(const FString& Category);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateUI(float Happiness, float Rebels);

	UFUNCTION(BlueprintImplementableEvent)
		void SaveSettings();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateResolutionText();

	UFUNCTION(BlueprintImplementableEvent)
		void LawPassed(bool bPassed, const FString& Description, int32 For, int32 Against);

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
		void SetCurrentResearchUI();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMapSeed(const FString& Seed);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMapSpecialBuildings();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateMapAIUI();

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
		void UpdateLoadingText(const FString& Message);

	UFUNCTION(BlueprintImplementableEvent)
		void SetArmyWidgetUI(const FString& FactionName, class UUserWidget* Widget, int32 Index);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateArmyWidgetSelectionUI(class UUserWidget* Widget, bool bShow);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateArmyCountUI(int32 Index, int32 Amount);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateCitizenInfoDisplay(EInfoUpdate Type, const FString& Name);

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateBuildingInfoDisplay(class ABuilding* Building, bool bOccupants);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer Manager")
		class UDiplosimTimerManager* TimerManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disease Manager")
		class UDiseaseManager* DiseaseManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events Manager")
		class UEventsManager* EventsManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Politics Manager")
		class UPoliticsManager* PoliticsManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Police Manager")
		class UPoliceManager* PoliceManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Research Manager")
		class UResearchManager* ResearchManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest Manager")
		class UConquestManager* ConquestManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Army Manager")
		class UArmyManager* ArmyManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UBuildComponent* BuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		class UCameraMovementComponent* MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Game")
		class USaveGameComponent* SaveGameComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
		class AGrid* Grid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
		class UDiplosimUserSettings* Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString ColonyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString TypedColonyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		int32 CitizenNum;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Start")
		bool Start;

	UPROPERTY()
		bool bStartMenu;

	UPROPERTY()
		FRandomStream Stream;

	UPROPERTY()
		APlayerController* PController;

	void Tick(float DeltaTime) override;

	void OnEggTimerPlace(class ABuilding* EggTimer);

	void IntroUI();

	UFUNCTION(BlueprintCallable)
		void Quit(bool bMenu);

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* InteractAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* MusicAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* InteractSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* EventSound;

	void PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound, float Pitch = -1.0f);

	UFUNCTION(BlueprintCallable)
		void PlayInteractSound(USoundBase* Sound, float Pitch = 1.0f);

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
		TSubclassOf<class UUserWidget> SaveLoadGameUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* SaveLoadGameUIInstance;

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
		TSubclassOf<class UUserWidget> DiplomacyUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* DiplomacyUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> HoursUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* HoursUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> RentUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* RentUIInstance;

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
		TSubclassOf<class UUserWidget> ArmyEditorUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* ArmyEditorUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> InfoUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* InfoUIInstance;

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
		void ShowEvent(FString Descriptor, FString Event);

	UFUNCTION(BlueprintCallable)
		void NotifyLog(FString Type, FString Message, FString IslandName);

	UFUNCTION(BlueprintCallable)
		void ClearPopupUI();

	UFUNCTION(BlueprintCallable)
		void SetPause(bool bPause, bool bAlterViewport = true);

	UFUNCTION(BlueprintCallable)
		void SetGameSpeed(float Speed);

	UFUNCTION(BlueprintCallable)
		float GetGameSpeed();

	void SetTimeDilation(float Dilation);

	UFUNCTION(BlueprintCallable)
		void DisplayInteractOnAI(AAI* AI);

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

	UFUNCTION()
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputGameSpeedNumbers;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		bool bBulldoze;

	UPROPERTY()
		FHoverStruct HoveredActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
		bool bQuick;

	UPROPERTY()
		bool bBlockPause;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<class ABuilding> StartBuilding;

	UPROPERTY()
		int32 GameSpeedCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loop")
		float LoopInterval;

	UPROPERTY()
		float LoopCount;

	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "UI")
		void Menu();

private:
	// Commands
	void Action(const struct FInputActionInstance& Instance);

	void Bulldoze();

	void Pause();

	void NewMap();

	// Building
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

	void DirectSetGameSpeed(const struct FInputActionInstance& Instance);
};
