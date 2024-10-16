#include "Camera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/AudioComponent.h"

#include "Map/Grid.h"
#include "Map/Resources/Mineral.h"
#include "Components/BuildComponent.h"
#include "Components/CameraMovementComponent.h"
#include "Managers/ResourceManager.h"
#include "Managers/ConstructionManager.h"
#include "Managers/CitizenManager.h"
#include "Buildings/Building.h"
#include "Buildings/Misc/Broch.h"
#include "AI/AI.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/EggBasket.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/HealthComponent.h"

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
	CameraComponent->PrimaryComponentTick.bCanEverTick = false;

	InteractAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("InteractAudioComponent"));
	InteractAudioComponent->SetupAttachment(CameraComponent);
	InteractAudioComponent->SetUISound(true);
	InteractAudioComponent->SetAutoActivate(false);
	InteractAudioComponent->SetTickableWhenPaused(true);

	MusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicAudioComponent"));
	MusicAudioComponent->SetupAttachment(CameraComponent);
	MusicAudioComponent->SetVolumeMultiplier(0.0f);
	MusicAudioComponent->SetUISound(true);
	MusicAudioComponent->SetAutoActivate(true);
	MusicAudioComponent->SetTickableWhenPaused(true);

	ResourceManager = CreateDefaultSubobject<UResourceManager>(TEXT("ResourceManager"));
	ResourceManager->SetTickableWhenPaused(true);

	ConstructionManager = CreateDefaultSubobject<UConstructionManager>(TEXT("ConstructionManager"));
	ConstructionManager->SetTickableWhenPaused(true);

	CitizenManager = CreateDefaultSubobject<UCitizenManager>(TEXT("CitizenManager"));

	WidgetSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("WidgetSpringArmComponent"));
	WidgetSpringArmComponent->SetupAttachment(RootComponent);
	WidgetSpringArmComponent->SetTickableWhenPaused(true);
	WidgetSpringArmComponent->TargetArmLength = 0.0f;
	WidgetSpringArmComponent->bUsePawnControlRotation = true;
	WidgetSpringArmComponent->bEnableCameraLag = true;
	WidgetSpringArmComponent->bDoCollisionTest = false;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(WidgetSpringArmComponent);
	WidgetComponent->SetTickableWhenPaused(true);
	WidgetComponent->SetHiddenInGame(true);

	Start = true;

	bQuick = false;

	bInMenu = false;

	bLost = false;

	ColonyName = "Eggerton";
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	ResourceManager->GameMode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	GetWorldTimerManager().SetTimer(ResourceManager->ValueTimer, ResourceManager, &UResourceManager::SetTradeValues, 300.0f, true);

	if (Start) {
		ResourceManager->RandomiseMarket();

		BuildComponent->SpawnBuilding(StartBuilding);
	}

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

	EventUIInstance = CreateWidget<UUserWidget>(pcontroller, EventUI);
	EventUIInstance->AddToViewport();

	TLDRUIInstance = CreateWidget<UUserWidget>(pcontroller, TLDRUI);
}

void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (BuildComponent->IsComponentTickEnabled())
		return;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	HoveredActor.Reset();

	pcontroller->CurrentMouseCursor = EMouseCursor::Default;

	FVector mouseLoc, mouseDirection;
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FHitResult hit(ForceInit);

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_Visibility)) {
		AActor* actor = hit.GetActor();

		if (!actor->IsA<AAI>() && !actor->IsA<ABuilding>() && !actor->IsA<AEggBasket>() && !actor->IsA<AMineral>())
			return;

		HoveredActor.Actor = actor;

		if (actor->IsA<AMineral>())
			HoveredActor.Instance = hit.Item;

		pcontroller->CurrentMouseCursor = EMouseCursor::Hand;
	}
}

void ACamera::StartGame(ABuilding* Broch)
{
	Start = false;
	Grid->MapUIInstance->RemoveFromParent();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->Camera = this;
	gamemode->Broch = Broch;
	gamemode->Grid = Grid;

	gamemode->SetWaveTimer();

	GetWorld()->GetTimerManager().SetTimer(ResourceManager->InterestTimer, ResourceManager, &UResourceManager::Interest, 300.0f, true);

	CitizenManager->StartTimers();

	CitizenManager->BrochLocation = Broch->GetActorLocation();

	Cast<ABroch>(Broch)->SpawnCitizens();

	DisplayEvent("Welcome to", ColonyName);

	FTimerHandle displayBuildUITimer;
	GetWorld()->GetTimerManager().SetTimer(displayBuildUITimer, this, &ACamera::DisplayBuildUI, 2.7f, false);
}

void ACamera::PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound)
{
	if (GetWorld()->GetMapName() != "Map" || Grid->Storage.IsEmpty())
		return;

	AsyncTask(ENamedThreads::GameThread, [this, AudioComponent, Sound]() {
		UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

		AudioComponent->SetSound(Sound);
		AudioComponent->SetVolumeMultiplier(settings->GetAmbientVolume() * settings->GetMasterVolume());
		AudioComponent->Play();
	});
}

void ACamera::SetInteractAudioSound(USoundBase* Sound, float Volume)
{
	InteractAudioComponent->SetSound(Sound);
	InteractAudioComponent->SetVolumeMultiplier(Volume);
}

void ACamera::PlayInteractSound()
{
	InteractAudioComponent->Play();
}

void ACamera::DisplayBuildUI()
{
	BuildUIInstance->AddToViewport();

	Pause();

	TLDRUIInstance->AddToViewport();
}

void ACamera::TickWhenPaused(bool bTickWhenPaused)
{
	MovementComponent->SetTickableWhenPaused(bTickWhenPaused);
	BuildComponent->SetTickableWhenPaused(bTickWhenPaused);
	SpringArmComponent->SetTickableWhenPaused(bTickWhenPaused);
	CameraComponent->SetTickableWhenPaused(bTickWhenPaused);
	ResourceManager->SetTickableWhenPaused(bTickWhenPaused);
	ConstructionManager->SetTickableWhenPaused(bTickWhenPaused);

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetPause(!bTickWhenPaused);
}

void ACamera::DisplayInteract(AActor* Actor, int32 Instance)
{
	SetInteractableText(Actor, Instance);

	if (Actor->IsA<AAI>()) {
		AttachToActor(Actor, FAttachmentTransformRules::KeepRelativeTransform);
		SetActorLocation(Actor->GetActorLocation() + FVector(0.0f, 0.0f, 5.0f));

		SpringArmComponent->ProbeSize = 6.0f;
	}

	UDecalComponent* decal = Actor->FindComponentByClass<UDecalComponent>();

	if (IsValid(decal) && decal->GetDecalMaterial() != nullptr && !ConstructionManager->IsBeingConstructed(Cast<ABuilding>(Actor), nullptr))
		decal->SetVisibility(true);

	FString socketName = "InfoSocket";

	if (Instance > -1)
		socketName.AppendInt(Instance);

	WidgetSpringArmComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(*socketName));

	WidgetComponent->SetHiddenInGame(false);
}

void ACamera::Lose()
{
	bLost = true;

	Cancel();

	BuildUIInstance->RemoveFromParent();
	EventUIInstance->RemoveFromParent();
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
		if (bQuick)
			BuildComponent->QuickPlace();
		else
			BuildComponent->Place();
	}
	else {
		if (WidgetSpringArmComponent->GetAttachParent() != RootComponent && !IsValid(HoveredActor.Actor)) {
			if (!WidgetComponent->bHiddenInGame)
				WidgetComponent->SetHiddenInGame(true);

			UDecalComponent* decalComponent = WidgetSpringArmComponent->GetAttachmentRootActor()->GetComponentByClass<UDecalComponent>();

			if (IsValid(decalComponent))
				decalComponent->SetVisibility(false);

			WidgetSpringArmComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			return;
		}

		if (!IsValid(HoveredActor.Actor))
			return;

		if (HoveredActor.Actor->IsA<AEggBasket>())
			Cast<AEggBasket>(HoveredActor.Actor)->RedeemReward();
		else
			DisplayInteract(HoveredActor.Actor, HoveredActor.Instance);
	}
}

void ACamera::Cancel()
{
	if (!BuildComponent->IsComponentTickEnabled() || Start || bInMenu)
		return;

	BuildComponent->RemoveBuilding();
}

void ACamera::NewMap()
{
	if (!Start || bInMenu)
		return;

	Grid->Clear();
	Grid->Load();
}

void ACamera::Pause()
{
	if (GetWorld()->GetMapName() != "Map" || bInMenu || TLDRUIInstance->IsInViewport())
		return;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetPause(!pcontroller->IsPaused());

	if (PauseUIInstance->IsInViewport()) {
		PauseUIInstance->RemoveFromParent();

		GetWorldTimerManager().UnPauseTimer(ResourceManager->ValueTimer);
	}
	else {
		PauseUIInstance->AddToViewport();

		GetWorldTimerManager().PauseTimer(ResourceManager->ValueTimer);
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

	// Spawn Enemies
	//ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	//gamemode->SpawnEnemiesAsync();

	CitizenManager->Buildings.Last()->HealthComponent->TakeHealth(1000, CitizenManager->Buildings.Last());
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

	if (GetAttachParentActor() != nullptr)
		SpringArmComponent->ProbeSize = 12.0f;

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