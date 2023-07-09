#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;

	TargetLength = 1000.0f;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = TargetLength;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);

	CameraSpeed = 4.0f;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();
	
	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->SetControlRotation(FRotator(-45.0f, 0.0f, 0.0f));
	PController->SetInputMode(FInputModeGameAndUI());
	PController->bShowMouseCursor = true;
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
	bool rmbHeld = PController->IsInputKeyDown(EKeys::RightMouseButton);
	bool keyQ = PController->IsInputKeyDown(EKeys::Q);
	bool keyE = PController->IsInputKeyDown(EKeys::E);

	if (rmbHeld || keyQ || keyE)
	{
		AddControllerYawInput(Value / 6);
	}
}

void ACamera::LookUp(float Value) 
{
	bool keyHeld = PController->IsInputKeyDown(EKeys::RightMouseButton);

	if (keyHeld)
	{
		AddControllerPitchInput(Value / 6);
	}
}

void ACamera::MoveForward(float Value) 
{
	const FRotator rotation = Controller->GetControlRotation();
	const FRotator yawRotation(0, rotation.Yaw, 0);

	FVector direction = FRotationMatrix(yawRotation).GetScaledAxis(EAxis::X);

	FVector loc = GetActorLocation();

	loc += direction * Value * CameraSpeed;

	SetActorLocation(loc);
}

void ACamera::MoveRight(float Value)
{
	FVector direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);

	FVector loc = GetActorLocation();

	loc += direction * Value * CameraSpeed;

	SetActorLocation(loc);
}

void ACamera::SpeedUp() 
{
	CameraSpeed *= 2;
}

void ACamera::SlowDown() 
{
	CameraSpeed /= 2;
}

void ACamera::Scroll(float Value) 
{
	float target = 250.0f * Value;

	if (target != 0.0f) {
		TargetLength = FMath::Clamp(TargetLength + target, 0.0f, 2000.0f);
	}

	SpringArmComponent->TargetArmLength = FMath::FInterpTo(SpringArmComponent->TargetArmLength, TargetLength, GetWorld()->DeltaTimeSeconds, 10.0f);
}