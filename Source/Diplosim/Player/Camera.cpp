#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/WidgetComponent.h"

#include "Map/Grid.h"
#include "BuildComponent.h"
#include "CameraMovementComponent.h"
#include "ResourceManager.h"
#include "ConstructionManager.h"
#include "Buildings/Building.h"
#include "DiplosimGameModeBase.h"
#include "AI/AI.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;
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

	ConstructionManagerComponent = CreateDefaultSubobject<UConstructionManager>(TEXT("ConstructionManagerComponent"));
	ConstructionManagerComponent->SetTickableWhenPaused(true);

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetTickableWhenPaused(true);
	WidgetComponent->SetHiddenInGame(true);

	start = true;

	bQuick = false;

	bInMenu = false;

	bLost = false;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	if (start)
		BuildComponent->SetBuildingClass(StartBuilding);

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	GetWorld()->bIsCameraMoveableWhenPaused = true;

	UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pcontroller->GetLocalPlayer());

	if (InputSystem && InputMapping)
		InputSystem->AddMappingContext(InputMapping, 0);

	BuildUIInstance = CreateWidget<UUserWidget>(pcontroller, BuildUI);

	PauseUIInstance = CreateWidget<UUserWidget>(pcontroller, PauseUI);

	MenuUIInstance = CreateWidget<UUserWidget>(pcontroller, MenuUI);

	LostUIInstance = CreateWidget<UUserWidget>(pcontroller, LostUI);

	SettingsUIInstance = CreateWidget<UUserWidget>(pcontroller, SettingsUI);
}

void ACamera::TickWhenPaused(bool bTickWhenPaused)
{
	MovementComponent->SetTickableWhenPaused(bTickWhenPaused);
	BuildComponent->SetTickableWhenPaused(bTickWhenPaused);
	SpringArmComponent->SetTickableWhenPaused(bTickWhenPaused);
	CameraComponent->SetTickableWhenPaused(bTickWhenPaused);
	ResourceManagerComponent->SetTickableWhenPaused(bTickWhenPaused);

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetPause(!bTickWhenPaused);
}

void ACamera::DisplayInteract(AActor* Actor)
{
	SetInteractableText(Actor);

	float height;

	if (Actor->IsA<AAI>()) {
		height = Cast<AAI>(Actor)->GetMesh()->GetLocalBounds().GetBox().GetSize().Z;

		AttachToActor(Actor, FAttachmentTransformRules::KeepRelativeTransform);
		SetActorLocation(Actor->GetActorLocation() + FVector(0.0f, 0.0f, 5.0f));
	}
	else {
		UStaticMeshComponent* mesh = Actor->GetComponentByClass<UStaticMeshComponent>();

		height = mesh->GetStaticMesh()->GetBounds().GetBox().GetSize().Z;
	}

	WidgetComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	WidgetComponent->SetWorldLocation(Actor->GetActorLocation() + FVector(0.0f, 0.0f, height));

	WidgetComponent->SetHiddenInGame(false);
}

void ACamera::Lose()
{
	bLost = true;
	bInMenu = true;

	TickWhenPaused(false);

	LostUIInstance->AddToViewport();
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

	Input->BindAction(InputRotate, ETriggerEvent::Triggered, this, &ACamera::Rotate);
	Input->BindAction(InputRotate, ETriggerEvent::Completed, this, &ACamera::Rotate);

	// Other
	Input->BindAction(InputPause, ETriggerEvent::Started, this, &ACamera::Pause);

	Input->BindAction(InputMenu, ETriggerEvent::Started, this, &ACamera::Menu);

	Input->BindAction(InputDebug, ETriggerEvent::Started, this, &ACamera::Debug);
}

void ACamera::Action()
{
	if (bInMenu)
		return;

	if (BuildComponent->IsComponentTickEnabled()) {
		if (bQuick) {
			BuildComponent->QuickPlace();
		}
		else {
			BuildComponent->Place();
		}
	}
	else {
		FVector mouseLoc, mouseDirection;
		APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

		FHitResult hit(ForceInit);

		FVector endTrace = mouseLoc + (mouseDirection * 10000);

		if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_Visibility)) {
			AActor* actor = hit.GetActor();

			if (!actor->IsA<AAI>() && !actor->IsA<ABuilding>())
				return;

			DisplayInteract(actor);
		}
	}
}

void ACamera::Cancel()
{
	if (!BuildComponent->IsComponentTickEnabled() || start || bInMenu)
		return;

	BuildComponent->SetBuildingClass(BuildComponent->Building->GetClass());
}

void ACamera::NewMap()
{
	if (!start || bInMenu)
		return;

	Grid->Clear();
	Grid->Load();
}

void ACamera::Pause()
{
	if (GetWorld()->GetMapName() != "Map" || bInMenu)
		return;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetPause(!pcontroller->IsPaused());

	if (PauseUIInstance->IsInViewport()) {
		PauseUIInstance->RemoveFromParent();
	}
	else {
		PauseUIInstance->AddToViewport();
	}
}

void ACamera::Menu()
{
	if (bLost)
		return;

	if (!WidgetComponent->bHiddenInGame) {
		WidgetComponent->SetHiddenInGame(true);

		return;
	}

	if (GetWorld()->GetMapName() != "Map")
		return;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (bInMenu) {
		MenuUIInstance->RemoveFromParent();
		SettingsUIInstance->RemoveFromParent();

		bInMenu = false;

		if (!PauseUIInstance->IsInViewport())
			TickWhenPaused(true);
	}
	else {
		MenuUIInstance->AddToViewport();

		bInMenu = true;

		if (!PauseUIInstance->IsInViewport())
			TickWhenPaused(false);
	}
}

void ACamera::Debug()
{
	if (bInMenu)
		return;

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->SpawnEnemiesAsync();
}

void ACamera::Rotate(const struct FInputActionInstance& Instance)
{
	if (bInMenu)
		return;

	BuildComponent->RotateBuilding(Instance.GetValue().Get<bool>());
}

void ACamera::Look(const struct FInputActionInstance& Instance)
{
	if (bInMenu)
		return;

	MovementComponent->Look(Instance);
}

void ACamera::Move(const struct FInputActionInstance& Instance)
{
	if (bInMenu)
		return;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	FVector location = GetActorLocation();
	location.Z = 800.0f;
	SetActorLocation(location);

	MovementComponent->Move(Instance);
}

void ACamera::Speed(const struct FInputActionInstance& Instance)
{
	if (bInMenu)
		return;

	MovementComponent->Speed(Instance);

	bQuick = !bQuick;
}

void ACamera::Scroll(const struct FInputActionInstance& Instance)
{
	if (bInMenu)
		return;

	MovementComponent->Scroll(Instance);
}