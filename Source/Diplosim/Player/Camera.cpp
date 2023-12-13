#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"

#include "Map/Grid.h"
#include "BuildComponent.h"
#include "CameraMovementComponent.h"
#include "ResourceManager.h"
#include "Buildings/Building.h"
#include "AI/Enemy.h"
#include "DiplosimGameModeBase.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = false;
	SetTickableWhenPaused(true);

	MovementComponent = CreateDefaultSubobject<UCameraMovementComponent>(TEXT("CameraMovementComponent"));
	MovementComponent->SetTickableWhenPaused(true);

	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));
	BuildComponent->SetTickableWhenPaused(true);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetTickableWhenPaused(true);
	SpringArmComponent->TargetArmLength = MovementComponent->TargetLength;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->bDoCollisionTest = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CameraComponent->SetTickableWhenPaused(true);

	ResourceManagerComponent = CreateDefaultSubobject<UResourceManager>(TEXT("ResourceManagerComponent"));
	ResourceManagerComponent->SetTickableWhenPaused(true);

	start = true;

	bQuick = false;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	if (start) {
		BuildComponent->SetBuildingClass(StartBuilding);
	}

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	GetWorld()->bIsCameraMoveableWhenPaused = true;

	UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pcontroller->GetLocalPlayer());

	if (InputSystem && InputMapping) {
		InputSystem->AddMappingContext(InputMapping, 0);
	}

	BuildUIInstance = CreateWidget<UUserWidget>(pcontroller, BuildUI);

	MapUIInstance = CreateWidget<UUserWidget>(pcontroller, MapUI);
	MapUIInstance->AddToViewport();
}

void ACamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	// Camera movement
	Input->BindAction(InputLook, ETriggerEvent::Triggered, this, &ACamera::Look);

	Input->BindAction(InputMove, ETriggerEvent::Triggered, this, &ACamera::Move);

	Input->BindAction(InputSpeed, ETriggerEvent::Started, this, &ACamera::Speed);
	Input->BindAction(InputSpeed, ETriggerEvent::Completed, this, &ACamera::Speed);

	Input->BindAction(InputScroll, ETriggerEvent::Triggered, this, &ACamera::Scroll);

	// Build
	Input->BindAction(InputAction, ETriggerEvent::Started, this, &ACamera::Action);

	Input->BindAction(InputCancel, ETriggerEvent::Started, this, &ACamera::Cancel);

	Input->BindAction(InputRender, ETriggerEvent::Started, this, &ACamera::NewMap);

	Input->BindAction(InputGrid, ETriggerEvent::Started, this, &ACamera::GridStatus);

	Input->BindAction(InputRotate, ETriggerEvent::Triggered, this, &ACamera::Rotate);
	Input->BindAction(InputRotate, ETriggerEvent::Completed, this, &ACamera::Rotate);

	// Other
	Input->BindAction(InputPause, ETriggerEvent::Started, this, &ACamera::Pause);

	Input->BindAction(InputDebug, ETriggerEvent::Started, this, &ACamera::Debug);
}

void ACamera::Action()
{
	if (BuildComponent->IsComponentTickEnabled()) {
		if (bQuick) {
			BuildComponent->QuickPlace();
		}
		else {
			BuildComponent->Place();

			ClearMinusText();
		}
	}
}

void ACamera::Cancel()
{
	if (BuildComponent->IsComponentTickEnabled() && !start) {
		BuildComponent->SetBuildingClass(BuildComponent->Building->GetClass());

		ClearMinusText();
	}
}

void ACamera::NewMap()
{
	if (start) {
		Grid->Clear();
		Grid->Render();
	}
}

void ACamera::GridStatus()
{
	BuildComponent->SetGridStatus();
}

void ACamera::Pause()
{
	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetPause(!pcontroller->IsPaused());
}

void ACamera::Debug()
{
	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->SpawnEnemies();
}

void ACamera::Rotate(const struct FInputActionInstance& Instance)
{
	BuildComponent->RotateBuilding(Instance.GetValue().Get<bool>());
}

void ACamera::Look(const struct FInputActionInstance& Instance)
{
	MovementComponent->Look(Instance);
}

void ACamera::Move(const struct FInputActionInstance& Instance)
{
	MovementComponent->Move(Instance);
}

void ACamera::Speed(const struct FInputActionInstance& Instance)
{
	MovementComponent->Speed(Instance);

	bQuick = !bQuick;
}

void ACamera::Scroll(const struct FInputActionInstance& Instance)
{
	MovementComponent->Scroll(Instance);
}