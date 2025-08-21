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
#include "NiagaraComponent.h"
#include "Engine/ScopedMovementUpdate.h"

#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Resources/Mineral.h"
#include "Map/Resources/Vegetation.h"
#include "Components/BuildComponent.h"
#include "Components/CameraMovementComponent.h"
#include "Managers/ResourceManager.h"
#include "Managers/ConstructionManager.h"
#include "Managers/CitizenManager.h"
#include "Managers/ResearchManager.h"
#include "Managers/ConquestManager.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/Misc/Special/Special.h"
#include "AI/AI.h"
#include "AI/Citizen.h"
#include "AI/AIMovementComponent.h"
#include "Universal/DiplosimGameModeBase.h"
#include "Universal/EggBasket.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/HealthComponent.h"
#include "Map/Atmosphere/AtmosphereComponent.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UCameraMovementComponent>(TEXT("CameraMovementComponent"));

	BuildComponent = CreateDefaultSubobject<UBuildComponent>(TEXT("BuildComponent"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = MovementComponent->TargetLength;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->bDoCollisionTest = true;

	RootComponent = SpringArmComponent;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CameraComponent->PrimaryComponentTick.bCanEverTick = false;
	CameraComponent->PostProcessSettings.AutoExposureMinBrightness = 0.0f;
	CameraComponent->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;

	InteractAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("InteractAudioComponent"));
	InteractAudioComponent->SetupAttachment(CameraComponent);
	InteractAudioComponent->SetUISound(true);
	InteractAudioComponent->SetAutoActivate(false);

	MusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicAudioComponent"));
	MusicAudioComponent->SetupAttachment(CameraComponent);
	MusicAudioComponent->SetVolumeMultiplier(0.0f);
	MusicAudioComponent->SetUISound(true);
	MusicAudioComponent->SetAutoActivate(true);

	ResourceManager = CreateDefaultSubobject<UResourceManager>(TEXT("ResourceManager"));

	ConstructionManager = CreateDefaultSubobject<UConstructionManager>(TEXT("ConstructionManager"));

	CitizenManager = CreateDefaultSubobject<UCitizenManager>(TEXT("CitizenManager"));

	ResearchManager = CreateDefaultSubobject<UResearchManager>(TEXT("ResearchManager"));

	ConquestManager = CreateDefaultSubobject<UConquestManager>(TEXT("ConquestManager"));

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetTickableWhenPaused(true);
	WidgetComponent->SetHiddenInGame(true);
	WidgetComponent->SetDrawSize(FVector2D(0.0f, 0.0f));
	WidgetComponent->SetPivot(FVector2D(0.5f, 1.1f));

	SmiteComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SmiteComponent"));
	SmiteComponent->SetupAttachment(RootComponent);
	SmiteComponent->SetAutoActivate(false);

	Smites = 0;

	bStartMenu = true;

	Start = true;

	bQuick = false;

	bInMenu = true;

	bLost = false;

	ColonyName = "Eggerton";
	CitizenNum = 10;

	FocusedCitizen = nullptr;

	bMouseCapture = true;

	bBlockPause = false;

	bInstantBuildCheat = false;
	bInstantEnemies = false;

	bWasClosingWindow = false;

	GameSpeed = 1.0f;
	ResetGameSpeedCounter();

	WikiURL = FPaths::ProjectDir() + "/Content/Custom/Wiki/index.html";
	HoveredWidget = nullptr;

	PController = nullptr;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	CitizenManager->Camera = this;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Camera = this;
	settings->SetVignette(settings->GetVignette());
	settings->SetSSAO(settings->GetSSAO());

	ResourceManager->GameMode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->bEnableClickEvents = true;

	SetMouseCapture(false);

	GetWorld()->bIsCameraMoveableWhenPaused = false;

	UEnhancedInputLocalPlayerSubsystem* inputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PController->GetLocalPlayer());
	inputSystem->AddMappingContext(MouseInputMapping, 0);
	inputSystem->AddMappingContext(MovementInputMapping, 1);

	MainMenuUIInstance = CreateWidget<UUserWidget>(PController, MainMenuUI);
	MainMenuUIInstance->AddToViewport();

	BuildUIInstance = CreateWidget<UUserWidget>(PController, BuildUI);

	PauseUIInstance = CreateWidget<UUserWidget>(PController, PauseUI);

	MenuUIInstance = CreateWidget<UUserWidget>(PController, MenuUI);

	LostUIInstance = CreateWidget<UUserWidget>(PController, LostUI);

	SettingsUIInstance = CreateWidget<UUserWidget>(PController, SettingsUI);
	WikiUIInstance = CreateWidget<UUserWidget>(PController, WikiUI);

	EventUIInstance = CreateWidget<UUserWidget>(PController, EventUI);
	EventUIInstance->AddToViewport();

	WarningUIInstance = CreateWidget<UUserWidget>(PController, WarningUI);
	WarningUIInstance->AddToViewport(1000);

	ParliamentUIInstance = CreateWidget<UUserWidget>(PController, ParliamentUI);

	LawPassedUIInstance = CreateWidget<UUserWidget>(PController, LawPassedUI);

	BribeUIInstance = CreateWidget<UUserWidget>(PController, BribeUI);

	BuildingColourUIInstance = CreateWidget<UUserWidget>(PController, BuildingColourUI);

	ResearchUIInstance = CreateWidget<UUserWidget>(PController, ResearchUI);

	ResearchHoverUIInstance = CreateWidget<UUserWidget>(PController, ResearchHoverUI);

	DiplomacyUIInstance = CreateWidget<UUserWidget>(PController, DiplomacyUI);

	HoursUIInstance = CreateWidget<UUserWidget>(PController, HoursUI);

	GiftUIInstance = CreateWidget<UUserWidget>(PController, GiftUI);

	DiplomacyNotifyUIInstance = CreateWidget<UUserWidget>(PController, DiplomacyNotifyUI);

	LogUIInstance = CreateWidget<UUserWidget>(PController, LogUI);

	if (GetWorld()->GetMapName() == "Map")
		Grid->Load();
}

void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime > 1.0f)
		return;

	if (CustomTimeDilation <= 1.0f && !Start)
		Grid->AIVisualiser->MainLoop(this);

	if (bMouseCapture)
		PController->SetMouseLocation(MousePosition.X, MousePosition.Y);

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	FHitResult hit(ForceInit);

	if (MainMenuUIInstance->IsInViewport() || BuildComponent->IsComponentTickEnabled() || ParliamentUIInstance->IsInViewport())
		return;

	HoveredActor.Reset();

	if (bBulldoze)
		PController->CurrentMouseCursor = EMouseCursor::CardinalCross;
	else
		PController->CurrentMouseCursor = EMouseCursor::Default;

	FVector mouseLoc, mouseDirection;
	PController->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FVector endTrace = mouseLoc + (mouseDirection * 30000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_Visibility)) {
		AActor* actor = hit.GetActor();

		MouseHitLocation = hit.Location;

		if (hit.GetComponent() == Grid->HISMSea || (hit.GetActor() != Grid && !hit.GetActor()->IsA<ABuilding>() && !hit.GetActor()->IsA<AEggBasket>()))
			return;

		if (hit.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>()) {
			AAI* ai = Grid->AIVisualiser->GetHISMAI(this, Cast<UHierarchicalInstancedStaticMeshComponent>(hit.Component), hit.Item);

			if (IsValid(ai))
				HoveredActor.Actor = ai;
			else
				return;
		}
		else
			HoveredActor.Actor = actor;

		HoveredActor.Component = hit.GetComponent();
		HoveredActor.Instance = hit.Item;

		if (!bBulldoze)
			PController->CurrentMouseCursor = EMouseCursor::Hand;
	}
}

void ACamera::SetMouseCapture(bool bCapture, bool bUI, bool bOverwrite)
{
	UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport(); 
	UEnhancedInputLocalPlayerSubsystem* inputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PController->GetLocalPlayer());

	if (bUI) {
		FInputModeGameAndUI inputMode;
		inputMode.SetHideCursorDuringCapture(false);
		PController->SetInputMode(inputMode);

		GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
		inputSystem->RemoveMappingContext(MouseInputMapping);

		PController->FlushPressedKeys();

		bCapture = false;
	}
	else if (!inputSystem->HasMappingContext(MouseInputMapping))
		inputSystem->AddMappingContext(MouseInputMapping, 0);

	if (bMouseCapture == bCapture && !bOverwrite)
		return;

	bMouseCapture = bCapture;
	PController->SetShowMouseCursor(!bCapture);

	if (bCapture) {
		FInputModeGameOnly inputMode;
		inputMode.SetConsumeCaptureMouseDown(bCapture);
		PController->SetInputMode(inputMode);

		GetWorld()->GetGameViewport()->GetMousePosition(MousePosition);
	}
	else {
		FInputModeGameAndUI inputMode;
		inputMode.SetHideCursorDuringCapture(bCapture);
		PController->SetInputMode(inputMode);
	}
}

void ACamera::StartGame()
{
	BuildComponent->SpawnBuilding(StartBuilding);

	bStartMenu = false;
	bInMenu = false;

	Grid->MapUIInstance->AddToViewport();
}

void ACamera::OnBrochPlace(ABuilding* Broch)
{
	if (PauseUIInstance->IsInViewport())
		Pause();

	bBlockPause = true;

	Grid->BuildSpecialBuildings();

	Grid->MapUIInstance->RemoveFromParent();

	ResourceManager->RandomiseMarket();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->Camera = this;
	gamemode->Broch = Broch;
	gamemode->Grid = Grid;

	gamemode->SetWaveTimer();

	CitizenManager->CreateTimer("Interest", this, 300, FTimerDelegate::CreateUObject(ResourceManager, &UResourceManager::Interest), true, true);

	CitizenManager->CreateTimer("TradeValue", this, 300, FTimerDelegate::CreateUObject(ResourceManager, &UResourceManager::SetTradeValues), true);

	CitizenManager->CreateTimer("EggBasket", this, 300, FTimerDelegate::CreateUObject(Grid, &AGrid::SpawnEggBasket), true, true);

	CitizenManager->StartDiseaseTimer();
	CitizenManager->BrochLocation = Broch->GetActorLocation();

	Cast<ABroch>(Broch)->SpawnCitizens();

	ConquestManager->CreateFactions();

	Start = false;

	DisplayEvent("Welcome to", ColonyName);

	FTimerHandle displayBuildUITimer;
	GetWorld()->GetTimerManager().SetTimer(displayBuildUITimer, this, &ACamera::DisplayBuildUI, 2.7f, false);
}

void ACamera::PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound, float Pitch)
{
	if (MainMenuUIInstance->IsInViewport() || Grid->Storage.IsEmpty())
		return;

	AsyncTask(ENamedThreads::GameThread, [this, AudioComponent, Sound, Pitch]() {
		UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

		float pitch = Pitch;

		if (pitch == -1.0f)
			pitch = Grid->Stream.FRandRange(0.8f, 1.2f);

		AudioComponent->SetSound(Sound);
		AudioComponent->SetPitchMultiplier(pitch);
		AudioComponent->SetVolumeMultiplier(settings->GetAmbientVolume() * settings->GetMasterVolume());
		AudioComponent->Play();
	});
}

void ACamera::SetInteractAudioSound(USoundBase* Sound, float Volume, float Pitch)
{
	InteractAudioComponent->SetSound(Sound);
	InteractAudioComponent->SetVolumeMultiplier(Volume);
	InteractAudioComponent->SetPitchMultiplier(Pitch);
}

void ACamera::PlayInteractSound()
{
	InteractAudioComponent->Play();
}

void ACamera::DisplayBuildUI()
{
	bBlockPause = false;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	if (settings->GetShowLog())
		LogUIInstance->AddToViewport();

	BuildUIInstance->AddToViewport();
}

void ACamera::ShowWarning(FString Warning)
{
	DisplayWarning(Warning);
}

void ACamera::NotifyLog(FString Type, FString Message, FString IslandName)
{
	AsyncTask(ENamedThreads::GameThread, [this, Type, Message, IslandName]() { DisplayNotifyLog(Type, Message, IslandName); });
}

void ACamera::ClearPopupUI()
{
	TArray<UUserWidget*> widgets = { ParliamentUIInstance, BribeUIInstance, ResearchUIInstance, ResearchHoverUIInstance, BuildingColourUIInstance, HoursUIInstance };

	if (widgets.Contains(HoveredWidget))
		widgets.Remove(HoveredWidget);

	for (UUserWidget* widget : widgets) {
		if (widget->IsInViewport()) {
			widget->RemoveFromParent();

			bWasClosingWindow = true;
		}
	}
}

void ACamera::SetPause(bool bPause, bool bAlterViewport)
{
	if (bInMenu && PauseUIInstance->IsInViewport())
		return;
	
	float timeDilation = 0.0001f;

	if (bPause) {
		if (bAlterViewport)
			PauseUIInstance->AddToViewport();

		UpdateSpeedUI(0.0f);
	}
	else {
		if (bAlterViewport)
			PauseUIInstance->RemoveFromParent();

		if (GameSpeed == 0.0f)
			GameSpeed = 1.0f;

		UpdateSpeedUI(GameSpeed);

		timeDilation = GameSpeed;
	}

	SetTimeDilation(timeDilation);
}

void ACamera::SetGameSpeed(float Speed)
{
	GameSpeed = FMath::Clamp(Speed, 0.0f, 5.0f);

	if (GameSpeed == 0.0f) {
		SetPause(true);
	}
	else {
		if (PauseUIInstance->IsInViewport())
			SetPause(false);
		else if (CustomTimeDilation <= 1.0f)
			SetTimeDilation(GameSpeed);
	}
}

void ACamera::SetTimeDilation(float Dilation)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), Dilation);

	CustomTimeDilation = 1.0f / Dilation;
}

void ACamera::DisplayInteract(AActor* Actor, USceneComponent* Component, int32 Instance)
{
	if (!IsValid(Actor))
		return;

	if (!IsValid(Component))
		Component = Actor->GetRootComponent();
	
	SetInteractableText(Actor, Instance);

	if (Actor->IsA<AAI>()) {
		bool bAttach = true;

		if (Actor->IsA<ACitizen>()) {
			ABuilding* building = Cast<ACitizen>(Actor)->Building.BuildingAt;

			if (building != nullptr && building->bHideCitizen)
				bAttach = false;

			FocusedCitizen = Cast<ACitizen>(Actor);
		}

		if (bAttach)
			Attach(Actor, Component, Instance);
	}

	SetInteractStatus(Actor, true, Component, Instance);
}

void ACamera::SetInteractStatus(AActor* Actor, bool bStatus, USceneComponent* Component, int32 Instance)
{
	if (IsValid(WidgetComponent->GetAttachParentActor()) && WidgetComponent->GetAttachParentActor() != Actor && WidgetComponent->GetAttachParentActor()->IsA<ABuilding>())
		Cast<ABuilding>(WidgetComponent->GetAttachParentActor())->ToggleDecalComponentVisibility(false);

	if (IsValid(Actor) && Actor->IsA<ABuilding>())
		Cast<ABuilding>(Actor)->ToggleDecalComponentVisibility(bStatus);

	WidgetComponent->SetHiddenInGame(!bStatus);

	if (bStatus) {
		WidgetComponent->AttachToComponent(Component, FAttachmentTransformRules::KeepWorldTransform);

		MovementComponent->SetAttachedMovementLocation(Actor, Component, Instance);
	}
	else {
		WidgetComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void ACamera::Attach(AActor* Actor, USceneComponent* Component, int32 Instance)
{
	AttachedTo.Actor = Actor;

	if (IsValid(Component)) {
		AttachedTo.Component = Component;
		AttachedTo.Instance = Instance;
	}

	SpringArmComponent->ProbeSize = 6.0f;
}

void ACamera::Detach()
{
	if (!IsValid(AttachedTo.Actor))
		return;
	
	SpringArmComponent->ProbeSize = 12.0f;

	AttachedTo.Reset();

	FVector location = GetActorLocation();
	location.Z = 800.0f;
	MovementComponent->MovementLocation = location;

	FocusedCitizen = nullptr;
}

FVector ACamera::GetTargetActorLocation(AActor* Actor)
{
	FVector location = Actor->GetActorLocation();

	if (Actor->IsA<AAI>())
		location = Cast<AAI>(Actor)->MovementComponent->Transform.GetLocation();

	return location;
}

void ACamera::Lose()
{
	bLost = true;

	Cancel();

	LogUIInstance->RemoveFromParent();
	BuildUIInstance->RemoveFromParent();
	EventUIInstance->RemoveFromParent();
	LostUIInstance->AddToViewport();
}

void ACamera::Smite(class AAI* AI)
{
	bool bPass = ResourceManager->TakeUniversalResource(Crystal, GetSmiteCost(), 0);

	if (!bPass) {
		ShowWarning("Cannot afford");

		return;
	}

	SmiteComponent->SetVariablePosition("StartLocation", GetTargetActorLocation(AI) + FVector(0.0f, 0.0f, 2000.0f));
	SmiteComponent->SetVariablePosition("EndLocation", GetTargetActorLocation(AI));

	SmiteComponent->Activate();

	AI->HealthComponent->TakeHealth(200, this);

	IncrementSmites(1);

	int32 timeToCompleteDay = 360 / (24 * Grid->AtmosphereComponent->Speed);

	CitizenManager->CreateTimer("Smite", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &ACamera::IncrementSmites, -1), false);
}

void ACamera::IncrementSmites(int32 Increment)
{
	Smites = FMath::Max(Smites + Increment, 0);
}

int32 ACamera::GetSmiteCost()
{
	int32 cost = 20;

	for (int32 i = 0; i < Smites; i++)
		cost *= 1.15;

	return cost;
}

void ACamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	// Camera movement
	Input->BindAction(InputLookChord, ETriggerEvent::Started, this, &ACamera::ActivateLook);
	Input->BindAction(InputLookChord, ETriggerEvent::Completed, this, &ACamera::ActivateLook);

	Input->BindAction(InputLook, ETriggerEvent::Triggered, this, &ACamera::Look);
	Input->BindAction(InputLook, ETriggerEvent::Completed, this, &ACamera::Look);

	Input->BindAction(InputMove, ETriggerEvent::Triggered, this, &ACamera::Move);

	Input->BindAction(InputSpeed, ETriggerEvent::Started, this, &ACamera::Speed);
	Input->BindAction(InputSpeed, ETriggerEvent::Completed, this, &ACamera::Speed);

	Input->BindAction(InputScroll, ETriggerEvent::Triggered, this, &ACamera::Scroll);

	// Build
	Input->BindAction(InputAction, ETriggerEvent::Started, this, &ACamera::Action);
	Input->BindAction(InputAction, ETriggerEvent::Completed, this, &ACamera::Action);

	Input->BindAction(InputAction, ETriggerEvent::Triggered, this, &ACamera::Bulldoze);

	Input->BindAction(InputCancel, ETriggerEvent::Started, this, &ACamera::Cancel);

	Input->BindAction(InputRender, ETriggerEvent::Started, this, &ACamera::NewMap);

	Input->BindAction(InputRotate, ETriggerEvent::Triggered, this, &ACamera::Rotate);
	Input->BindAction(InputRotate, ETriggerEvent::Completed, this, &ACamera::Rotate);

	// Other
	Input->BindAction(InputPause, ETriggerEvent::Started, this, &ACamera::Pause);

	Input->BindAction(InputMenu, ETriggerEvent::Started, this, &ACamera::Menu);

	Input->BindAction(InputGameSpeed, ETriggerEvent::Triggered, this, &ACamera::IncrementGameSpeed);
	Input->BindAction(InputGameSpeed, ETriggerEvent::Completed, this, &ACamera::ResetGameSpeedCounter);
}

void ACamera::Action(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport())
		return;

	ClearPopupUI();
	
	if (bWasClosingWindow || bInMenu || ParliamentUIInstance->IsInViewport() || ResearchUIInstance->IsInViewport() || bBulldoze) {
		bWasClosingWindow = false;

		return;
	}

	if (BuildComponent->IsComponentTickEnabled()) {
		if ((bool)(Instance.GetTriggerEvent() & ETriggerEvent::Completed)) {
			if (bQuick)
				BuildComponent->QuickPlace();
			else
				BuildComponent->Place();
		}
		else
			BuildComponent->StartPathPlace();
	}
	else if (!(bool)(Instance.GetTriggerEvent() & ETriggerEvent::Completed)) {
		FocusedCitizen = nullptr;

		if (WidgetComponent->GetAttachParent() != RootComponent && !IsValid(HoveredActor.Actor)) {
			SetInteractStatus(WidgetComponent->GetAttachmentRootActor(), false);

			return;
		}

		if (!IsValid(HoveredActor.Actor))
			return;

		if (HoveredActor.Actor->IsA<AEggBasket>())
			Cast<AEggBasket>(HoveredActor.Actor)->RedeemReward();
		else
			DisplayInteract(HoveredActor.Actor, HoveredActor.Component, HoveredActor.Instance);
	}
}

void ACamera::Bulldoze()
{
	if (Grid->LoadUIInstance->IsInViewport() || !bBulldoze || !IsValid(HoveredActor.Actor) || !HoveredActor.Actor->IsA<ABuilding>() || !Cast<ABuilding>(HoveredActor.Actor)->bCanDestroy)
		return;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	SetInteractAudioSound(BuildComponent->PlaceSound, settings->GetMasterVolume() * settings->GetSFXVolume());
	PlayInteractSound();

	Cast<ABuilding>(HoveredActor.Actor)->DestroyBuilding();
}

void ACamera::Cancel()
{
	if (Grid->LoadUIInstance->IsInViewport() || Start || bInMenu)
		return;

	if (bBulldoze)
		bBulldoze = false;
	else if (BuildComponent->IsComponentTickEnabled())
		BuildComponent->RemoveBuilding();
}

void ACamera::NewMap()
{
	if (Grid->LoadUIInstance->IsInViewport() || !Start || bInMenu)
		return;

	SetMouseCapture(false);

	Grid->Clear();
	Grid->Load();

	BuildComponent->Buildings[0]->SetActorLocation(FVector(0.0f, 0.0f, -1000.0f));
}

void ACamera::Pause()
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu || bBlockPause)
		return;

	if (PauseUIInstance->IsInViewport())
		SetPause(false);
	else
		SetPause(true);
}

void ACamera::Menu()
{
	if (Grid->LoadUIInstance->IsInViewport() || bLost || MainMenuUIInstance->IsInViewport())
		return;

	if (!WidgetComponent->bHiddenInGame && !BuildComponent->IsComponentTickEnabled()) {
		SetInteractStatus(WidgetComponent->GetAttachmentRootActor(), false);

		BuildingColourUIInstance->RemoveFromParent();
		HoursUIInstance->RemoveFromParent();

		if (HoveredWidget == BuildingColourUIInstance || HoveredWidget == HoursUIInstance)
			SetMouseCapture(bMouseCapture, false, true);

		return;
	}

	if (ParliamentUIInstance->IsInViewport()) {
		ParliamentUIInstance->RemoveFromParent();

		BribeUIInstance->RemoveFromParent();

		return;
	}
	else if (ResearchUIInstance->IsInViewport()) {
		ResearchUIInstance->RemoveFromParent();

		ResearchHoverUIInstance->RemoveFromParent();

		return;
	}

	if (bInMenu) {
		if (SettingsUIInstance->IsInViewport())
			UDiplosimUserSettings::GetDiplosimUserSettings()->SaveIniSettings();

		SettingsUIInstance->RemoveFromParent();
		WikiUIInstance->RemoveFromParent();

		if (bStartMenu) {
			MainMenuUIInstance->AddToViewport();

			return;
		}
		if (!MenuUIInstance->IsInViewport()) {
			MenuUIInstance->AddToViewport();

			return;
		}

		MenuUIInstance->RemoveFromParent();

		SetPause(false, false);

		bInMenu = false;
	}
	else {
		MenuUIInstance->AddToViewport();

		SetMouseCapture(false);

		bInMenu = true;

		SetPause(true, false);
	}
}

void ACamera::Rotate(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu)
		return;

	BuildComponent->RotateBuilding(Instance.GetValue().Get<bool>());
}

void ACamera::ActivateLook(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || (bInMenu && !bMouseCapture))
		return;

	if (((bool)(Instance.GetTriggerEvent() & ETriggerEvent::Completed)))
		SetMouseCapture(false);
	else
		SetMouseCapture(true);
}

void ACamera::Look(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu || !bMouseCapture)
		return;

	MovementComponent->Look(Instance);
}

void ACamera::Move(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu)
		return;

	Detach();

	MovementComponent->Move(Instance);
}

void ACamera::Speed(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu)
		return;

	MovementComponent->Speed(Instance);

	bQuick = !bQuick;
}

void ACamera::Scroll(const struct FInputActionInstance& Instance)
{
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu)
		return;

	MovementComponent->Scroll(Instance);
}

void ACamera::IncrementGameSpeed(const struct FInputActionInstance& Instance)
{	
	GameSpeedCounter++;
	
	if (Start || bInMenu || GameSpeedCounter < 50)
		return;

	GameSpeedCounter = 0;

	SetGameSpeed(GameSpeed + Instance.GetValue().Get<float>());

	UpdateSpeedUI(GameSpeed);
}

void ACamera::ResetGameSpeedCounter()
{
	GameSpeedCounter = 100;
}

//
// Debugging
//
void ACamera::SpawnEnemies()
{
	if (bInMenu)
		return;

	bInstantEnemies = true;

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	Async(EAsyncExecution::Thread, [this, gamemode]() { gamemode->StartRaid(); });
}

void ACamera::AddEnemies(FString Category, int32 Amount)
{
	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	TSubclassOf<AResource> resource;

	for (FResourceStruct resourceStruct : ResourceManager->ResourceList) {
		if (resourceStruct.Category != Category)
			continue;

		resource = resourceStruct.Type;

		break;
	}

	gamemode->TallyEnemyData(resource, Amount * 200);
}

void ACamera::DamageLastBuilding()
{
	if (bInMenu)
		return;

	CitizenManager->Buildings.Last()->HealthComponent->TakeHealth(1000, CitizenManager->Buildings.Last());
}

void ACamera::ChangeSeasonAffect(FString Season)
{
	if (bInMenu)
		return;

	Grid->SetSeasonAffect(Season, 0.02f);
}

void ACamera::CompleteResearch()
{
	if (bInMenu)
		return;

	ResearchManager->Research(100000000000.0f);
}

void ACamera::TurnOnInstantBuild(bool Value)
{
	if (bInMenu)
		return;

	bInstantBuildCheat = Value;
}

void ACamera::SpawnCitizen(int32 Amount, bool bAdult)
{
	if (bInMenu)
		return;

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());

	for (int32 i = 0; i < Amount; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(Cast<ABroch>(gamemode->Broch)->CitizenClass, MouseHitLocation, FRotator(0.0f));

		if (!bAdult || !IsValid(citizen))
			continue;

		for (int32 j = 0; j < 2; j++)
			citizen->GivePersonalityTrait();

		citizen->BioStruct.Age = 17;
		citizen->Birthday();

		citizen->HealthComponent->AddHealth(100);
	}
}

void ACamera::SetEvent(FString Type, FString Period, int32 Day, int32 StartHour, int32 EndHour, bool bRecurring, bool bFireFestival)
{
	if (bInMenu)
		return;
	
	EEventType type;
	TSubclassOf<ABuilding> building = nullptr;

	if (Type == "Holliday")
		type = EEventType::Holliday;
	else if (Type == "Festival") {
		type = EEventType::Festival;
		building = AFestival::StaticClass();
	}
	else if (Type == "Mass")
		type = EEventType::Mass;
	else
		type = EEventType::Protest;

	TArray<int32> hours;
	for (int32 i = StartHour; i < EndHour; i++)
		hours.Add(i);

	CitizenManager->CreateEvent(type, building, nullptr, Period, Day, hours, bRecurring, {}, bFireFestival);
}

void ACamera::DamageActor(int32 Amount)
{
	if (!IsValid(HoveredActor.Actor) || !(HoveredActor.Actor->IsA<AAI>() && HoveredActor.Actor->IsA<ABuilding>()))
		return;

	UHealthComponent* healthComp = HoveredActor.Actor->GetComponentByClass<UHealthComponent>();
	healthComp->TakeHealth(Amount, HoveredActor.Actor);
}