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
		int32 Instance;

	FHoverStruct()
	{
		Reset();
	}

	void Reset()
	{
		Actor = nullptr;
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
		void LawPassed(bool bPassed, int32 For, int32 Against);

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayWarning(const FString& Warning);

	UFUNCTION(BlueprintImplementableEvent)
		void DisplayNewBill();

	UFUNCTION(BlueprintImplementableEvent)
		void RefreshRepresentatives();

	UFUNCTION(BlueprintImplementableEvent)
		void SetSeedVisibility(bool bVisible);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UBuildComponent* BuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		class UCameraMovementComponent* MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
		class AGrid* Grid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colony")
		FString ColonyName;

	UPROPERTY()
		bool Start;

	UPROPERTY()
		bool bStartMenu;

	void Tick(float DeltaTime) override;

	void OnBrochPlace(class ABuilding* Broch);

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* InteractAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class UAudioComponent* MusicAudioComponent;

	void PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound);

	void SetInteractAudioSound(USoundBase* Sound, float Volume);

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
		TSubclassOf<class UUserWidget> EventUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* EventUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<class UUserWidget> TLDRUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* TLDRUIInstance;

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
		class USpringArmComponent* WidgetSpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		class UWidgetComponent* WidgetComponent;

	UPROPERTY()
		bool bInMenu;

	UPROPERTY()
		bool bLost;

	UPROPERTY()
		class ACitizen* FocusedCitizen;

	UFUNCTION(BlueprintCallable)
		void StartGame();

	void DisplayBuildUI();

	UFUNCTION(BlueprintCallable)
		void ShowWarning(FString Warning);

	void Pause(bool bPause, bool bTickWhenPaused);

	UFUNCTION(BlueprintCallable)
		void DisplayInteract(AActor* Actor, int32 Instance = -1);

	UFUNCTION(BlueprintCallable)
		void SetInteractStatus(AActor* Actor, bool bStatus, FString SocketName = "InfoSocket");

	void Detach();

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
		class UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
		class UInputAction* InputLook;

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
		class UInputAction* InputDebug;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
		FHoverStruct HoveredActor;

	// Commands
	void Action();

	void Cancel();

	UPROPERTY()
		bool bQuick;

	// Map
	void NewMap();

	// Other
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "UI")
		void Menu();

	void Debug();

	// Building
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
		TSubclassOf<class ABuilding> StartBuilding;

	void Rotate(const struct FInputActionInstance& Instance);

	// Camera Movement
	void Look(const struct FInputActionInstance& Instance);

	void Move(const struct FInputActionInstance& Instance);
	
	void Speed(const struct FInputActionInstance& Instance);

	void Scroll(const struct FInputActionInstance& Instance);
};
