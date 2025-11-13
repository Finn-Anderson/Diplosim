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
#include "Misc/ScopeTryLock.h"

#include "Map/Grid.h"
#include "Map/AIVisualiser.h"
#include "Map/Resources/Mineral.h"
#include "Map/Resources/Vegetation.h"
#include "Components/BuildComponent.h"
#include "Components/CameraMovementComponent.h"
#include "Components/SaveGameComponent.h"
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

	SaveGameComponent = CreateDefaultSubobject<USaveGameComponent>(TEXT("SaveGameComponent"));

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
	TypedColonyName = "Eggerton";
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
	Settings = nullptr;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	CitizenManager->Camera = this;
	SaveGameComponent->Camera = this;

	Settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	Settings->Camera = this;
	Settings->SetVignette(Settings->GetVignette());
	Settings->SetSSAO(Settings->GetSSAO());

	ResourceManager->GameMode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PController->bEnableClickEvents = true;

	ConquestManager->CreatePlayerFaction();

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
	SaveLoadGameUIInstance = CreateWidget<UUserWidget>(PController, SaveLoadGameUI);
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

	ArmyEditorUIInstance = CreateWidget<UUserWidget>(PController, ArmyEditorUI);

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
		
		TArray<AAI*> ais;

		for (FFactionStruct faction : ConquestManager->Factions) {
			ais.Append(faction.Citizens);
			ais.Append(faction.Rebels);
			ais.Append(faction.Clones);
		}

		ais.Append(CitizenManager->Enemies);

		FVector chosenLocation = FVector(1000000000000000.0f);

		for (AAI* ai : ais) {
			if (!IsValid(ai) || ai->HealthComponent->GetHealth() == 0)
				continue;

			FVector aiLoc = ai->MovementComponent->Transform.GetLocation() + FVector(0.0f, 0.0f, 9.0f);
			float distance = FMath::PointDistToSegment(aiLoc, mouseLoc, hit.Location);

			float size = 1.0f;
			if (ai->IsA<ACitizen>())
				size = Cast<ACitizen>(ai)->ReachMultiplier;

			if (distance > 12.0f * size || (chosenLocation != FVector(1000000000000000.0f) && FVector::Dist(mouseLoc, chosenLocation) < FVector::Dist(mouseLoc, aiLoc)))
				continue;

			HoveredActor.Actor = ai;

			TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> hismValue = Grid->AIVisualiser->GetAIHISM(ai);
			HoveredActor.Component = hismValue.Key;
			HoveredActor.Instance = hismValue.Value;

			chosenLocation = aiLoc;
		}

		if ((hit.GetActor()->IsA<ABuilding>() || hit.GetActor()->IsA<AEggBasket>()) && FVector::Dist(mouseLoc, chosenLocation) > FVector::Dist(mouseLoc, hit.Location)) {
			HoveredActor.Actor = actor;
			HoveredActor.Component = hit.GetComponent();
			HoveredActor.Instance = hit.Item;
		}

		if (hit.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>() && HoveredActor.Actor == nullptr)
			return;

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
	BuildComponent->SpawnBuilding(StartBuilding, ColonyName);

	bStartMenu = false;
	bInMenu = false;

	Grid->MapUIInstance->AddToViewport();
}

void ACamera::OnEggTimerPlace(ABuilding* EggTimer)
{
	if (PauseUIInstance->IsInViewport())
		Pause();

	ColonyName = TypedColonyName;

	Grid->BuildSpecialBuildings();

	Grid->MapUIInstance->RemoveFromParent();

	ResourceManager->RandomiseMarket();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->Camera = this;
	gamemode->Grid = Grid;

	gamemode->SetWaveTimer();

	CitizenManager->CreateTimer("TradeValue", this, 300, "SetTradeValues", {}, true);

	CitizenManager->CreateTimer("EggBasket", Grid, 300, "SpawnEggBasket", {}, true, true);

	CitizenManager->StartDiseaseTimer();

	ConquestManager->FinaliseFactions(Cast<ABroch>(EggTimer));

	Start = false;

	SaveGameComponent->StartNewSave();

	IntroUI();
}

void ACamera::IntroUI()
{
	bBlockPause = true;

	DisplayEvent("Welcome to", ColonyName);

	FTimerHandle displayBuildUITimer;
	GetWorld()->GetTimerManager().SetTimer(displayBuildUITimer, this, &ACamera::DisplayBuildUI, 2.7f, false);
}

void ACamera::Quit(bool bMenu)
{
	MainMenuUIInstance->RemoveFromParent();

	if (bMenu)
		UGameplayStatics::OpenLevel(GetWorld(), "Map");
	else
		UKismetSystemLibrary::QuitGame(GetWorld(), PController, EQuitPreference::Quit, false);
}

void ACamera::PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound, float Pitch)
{
	if (Sound == nullptr || MainMenuUIInstance->IsInViewport() || Grid->Storage.IsEmpty())
		return;

	Async(EAsyncExecution::TaskGraphMainTick, [this, AudioComponent, Sound, Pitch]() {
		float pitch = Pitch;

		if (pitch == -1.0f)
			pitch = Grid->Stream.FRandRange(0.8f, 1.2f);

		AudioComponent->SetSound(Sound);
		AudioComponent->SetPitchMultiplier(pitch);
		AudioComponent->SetVolumeMultiplier(Settings->GetAmbientVolume() * Settings->GetMasterVolume());
		AudioComponent->Play();
	});
}

void ACamera::PlayInteractSound(USoundBase* Sound, float Pitch)
{
	InteractAudioComponent->SetSound(Sound);
	InteractAudioComponent->SetVolumeMultiplier(Settings->GetMasterVolume() * Settings->GetSFXVolume());
	InteractAudioComponent->SetPitchMultiplier(Pitch * Grid->Stream.RandRange(0.95f, 1.05f));

	InteractAudioComponent->Play();
}

void ACamera::DisplayBuildUI()
{
	bBlockPause = false;

	if (Settings->GetShowLog())
		LogUIInstance->AddToViewport();

	BuildUIInstance->AddToViewport();
}

void ACamera::ShowWarning(FString Warning)
{
	DisplayWarning(Warning);
}

void ACamera::NotifyLog(FString Type, FString Message, FString IslandName)
{
	Async(EAsyncExecution::TaskGraphMainTick, [this, Type, Message, IslandName]() { DisplayNotifyLog(Type, Message, IslandName); });
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

void ACamera::DisplayInteractOnAI(AAI* AI)
{
	TTuple<class UHierarchicalInstancedStaticMeshComponent*, int32> hism = Grid->AIVisualiser->GetAIHISM(AI);

	DisplayInteract(AI, hism.Key, hism.Value);
}

void ACamera::DisplayInteract(AActor* Actor, USceneComponent* Component, int32 Instance)
{
	if (!IsValid(Actor))
		return;

	if (!IsValid(Component))
		Component = Actor->GetRootComponent();

	PlayInteractSound(InteractSound);
	
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
	else {
		Detach();
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
	FFactionStruct* faction = ConquestManager->GetFaction("", AI);

	bool bPass = ResourceManager->TakeUniversalResource(faction, Crystal, GetSmiteCost(), 0);

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

	TArray<FTimerParameterStruct> params;
	CitizenManager->SetParameter(-1, params);
	CitizenManager->CreateTimer("Smite", this, timeToCompleteDay, "IncrementSmites", params, false);
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

	Input->BindAction(InputGameSpeedNumbers, ETriggerEvent::Triggered, this, &ACamera::DirectSetGameSpeed);
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

		ConquestManager->SetSelectedArmy(INDEX_NONE);

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

	PlayInteractSound(BuildComponent->PlaceSound);

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
	else if (ConquestManager->PlayerSelectedArmyIndex > -1)
		ConquestManager->PlayerMoveArmy(MouseHitLocation);

	if (bBulldoze || BuildComponent->IsComponentTickEnabled() || ConquestManager->PlayerSelectedArmyIndex > -1)
		PlayInteractSound(InteractSound);
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

	PlayInteractSound(InteractSound);

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
	else if (DiplomacyUIInstance->IsInViewport()) {
		DiplomacyUIInstance->RemoveFromParent();

		return;
	}

	if (bInMenu) {
		if (SettingsUIInstance->IsInViewport())
			Settings->SaveIniSettings();

		SettingsUIInstance->RemoveFromParent();
		WikiUIInstance->RemoveFromParent();
		SaveLoadGameUIInstance->RemoveFromParent();

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

void ACamera::DirectSetGameSpeed(const FInputActionInstance& Instance)
{
	float value = Instance.GetValue().Get<float>();

	if (GameSpeed == value)
		return;

	SetGameSpeed(value);

	UpdateSpeedUI(GameSpeed);
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
	gamemode->StartRaid();
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

	ResearchManager->Research(100000000000.0f, ColonyName);
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

	FFactionStruct* faction = ConquestManager->GetFaction(ColonyName);

	for (int32 i = 0; i < Amount; i++) {
		ACitizen* citizen = GetWorld()->SpawnActor<ACitizen>(Cast<ABroch>(faction->EggTimer)->CitizenClass, MouseHitLocation, FRotator(0.0f));
		citizen->CitizenSetup(faction);

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

	CitizenManager->CreateEvent(ColonyName, type, building, nullptr, Period, Day, hours, bRecurring, {}, bFireFestival);
}

void ACamera::DamageActor(int32 Amount)
{
	AActor* actor = WidgetComponent->GetAttachParentActor();

	if (IsValid(FocusedCitizen))
		actor = FocusedCitizen;

	if (!IsValid(actor) || (!actor->IsA<AAI>() && !actor->IsA<ABuilding>()))
		return;

	UHealthComponent* healthComp = actor->GetComponentByClass<UHealthComponent>();
	healthComp->TakeHealth(Amount, this);
}