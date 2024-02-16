#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera.generated.h"

UCLASS()
class DIPLOSIM_API ACamera : public APawn
{
	GENERATED_BODY()

public:
	ACamera();

protected:
	virtual void BeginPlay() override;

public:
	// For Resource Manager. This is here because Resource Manager is not a blueprint.
	UFUNCTION(BlueprintImplementableEvent)
		void UpdateResourceText();

	UFUNCTION(BlueprintImplementableEvent)
		void ClearMinusText();

	UFUNCTION(BlueprintImplementableEvent)
		void SetInteractableText();

	UFUNCTION(BlueprintImplementableEvent)
		void EditInteractableText(const FString& Key, const FString& Value);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Manager")
		class UResourceManager* ResourceManagerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
		class UBuildComponent* BuildComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement")
		class UCameraMovementComponent* MovementComponent;

	UPROPERTY()
		class AGrid* Grid;

	UPROPERTY()
		bool start;

	// UI
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
		TSubclassOf<class UUserWidget> SettingsUI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
		class UUserWidget* SettingsUIInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		class UWidgetComponent* WidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		UInteractableComponent* SelectedInteractableComponent;

	void TickWhenPaused(bool bTickWhenPaused);

	bool bInMenu;

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

	// Commands
	void Action();

	void Cancel();

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
