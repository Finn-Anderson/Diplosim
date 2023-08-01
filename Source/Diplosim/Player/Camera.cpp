#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

#include "Map/Grid.h"
#include "BuildComponent.h"
#include "CameraMovementComponent.h"
#include "ResourceManager.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UCameraMovementComponent>(TEXT("CameraMovementComponent"));

	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = MovementComponent->TargetLength;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);

	ResourceManager = CreateDefaultSubobject<UResourceManager>(TEXT("ResourceManager"));

	start = true;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	if (start) {
		BuildComponent->Build();
	}

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	BuildUIInstance = CreateWidget<UUserWidget>(pcontroller, BuildUI);
}

void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Camera movement
	PlayerInputComponent->BindAxis("Turn", this, &ACamera::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ACamera::LookUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACamera::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACamera::MoveRight);

	PlayerInputComponent->BindAction("Speed", IE_Pressed, this, &ACamera::SpeedUp);
	PlayerInputComponent->BindAction("Speed", IE_Released, this, &ACamera::SlowDown);

	PlayerInputComponent->BindAxis("Scroll", this, &ACamera::Scroll);

	PlayerInputComponent->BindAction("Action", IE_Pressed, this, &ACamera::Action);

	PlayerInputComponent->BindAction("Render", IE_Pressed, this, &ACamera::NewMap);

	PlayerInputComponent->BindAction("Grid", IE_Pressed, this, &ACamera::GridStatus);

	PlayerInputComponent->BindAction("Build", IE_Pressed, this, &ACamera::BuildStatus);
	PlayerInputComponent->BindAction("Rotate", IE_Pressed, this, &ACamera::Rotate);
}

void ACamera::Action()
{
	if (BuildComponent->IsComponentTickEnabled()) {
		BuildComponent->Place();
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

void ACamera::BuildStatus()
{
	if (!start) {
		BuildComponent->Build();

		if (BuildComponent->IsComponentTickEnabled()) {
			BuildUIInstance->AddToViewport();
		}
		else {
			BuildUIInstance->RemoveFromViewport();
		}
	}
}

void ACamera::Rotate()
{
	BuildComponent->RotateBuilding();
}

void ACamera::Turn(float Value)
{
	MovementComponent->Turn(Value);
}

void ACamera::LookUp(float Value) 
{
	MovementComponent->LookUp(Value);
}

void ACamera::MoveForward(float Value) 
{
	MovementComponent->MoveForward(Value);
}

void ACamera::MoveRight(float Value)
{
	MovementComponent->MoveRight(Value);
}

void ACamera::SpeedUp() 
{
	MovementComponent->SpeedUp();
}

void ACamera::SlowDown() 
{
	MovementComponent->SlowDown();
}

void ACamera::Scroll(float Value) 
{
	MovementComponent->Scroll(Value);
}