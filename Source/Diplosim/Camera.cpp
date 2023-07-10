#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "CameraMovementComponent.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UCameraMovementComponent>(TEXT("CameraMovementComponent"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = MovementComponent->TargetLength;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();
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