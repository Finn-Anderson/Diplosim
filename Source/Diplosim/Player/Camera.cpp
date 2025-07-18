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

#include "Map/Grid.h"
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
#include "Buildings/Work/Service/Religion.h"
#include "Buildings/Misc/Special/Special.h"
#include "AI/AI.h"
#include "AI/Citizen.h"
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

	ActorAttachedTo = nullptr;

	bWasClosingWindow = false;

	GameSpeed = 1.0f;
	ResetGameSpeedCounter();

	WikiURL = FPaths::ProjectDir() + "/Content/Custom/Wiki/index.html";
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Camera = this;
	settings->SetVignette(settings->GetVignette());
	settings->SetSSAO(settings->GetSSAO());

	ResourceManager->GameMode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->bEnableClickEvents = true;

	SetMouseCapture(false);

	GetWorld()->bIsCameraMoveableWhenPaused = false;

	UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pcontroller->GetLocalPlayer());

	if (InputSystem && InputMapping)
		InputSystem->AddMappingContext(InputMapping, 0);

	MainMenuUIInstance = CreateWidget<UUserWidget>(pcontroller, MainMenuUI);
	MainMenuUIInstance->AddToViewport();

	BuildUIInstance = CreateWidget<UUserWidget>(pcontroller, BuildUI);

	PauseUIInstance = CreateWidget<UUserWidget>(pcontroller, PauseUI);

	MenuUIInstance = CreateWidget<UUserWidget>(pcontroller, MenuUI);

	LostUIInstance = CreateWidget<UUserWidget>(pcontroller, LostUI);

	SettingsUIInstance = CreateWidget<UUserWidget>(pcontroller, SettingsUI);
	WikiUIInstance = CreateWidget<UUserWidget>(pcontroller, WikiUI);

	EventUIInstance = CreateWidget<UUserWidget>(pcontroller, EventUI);
	EventUIInstance->AddToViewport();

	WarningUIInstance = CreateWidget<UUserWidget>(pcontroller, WarningUI);
	WarningUIInstance->AddToViewport(1000);

	ParliamentUIInstance = CreateWidget<UUserWidget>(pcontroller, ParliamentUI);

	LawPassedUIInstance = CreateWidget<UUserWidget>(pcontroller, LawPassedUI);

	BribeUIInstance = CreateWidget<UUserWidget>(pcontroller, BribeUI);

	BuildingColourUIInstance = CreateWidget<UUserWidget>(pcontroller, BuildingColourUI);

	ResearchUIInstance = CreateWidget<UUserWidget>(pcontroller, ResearchUI);

	ResearchHoverUIInstance = CreateWidget<UUserWidget>(pcontroller, ResearchHoverUI);

	WorldUIInstance = CreateWidget<UUserWidget>(pcontroller, WorldUI);

	FactionColourUIInstance = CreateWidget<UUserWidget>(pcontroller, FactionColourUI);

	HoursUIInstance = CreateWidget<UUserWidget>(pcontroller, HoursUI);

	GiftUIInstance = CreateWidget<UUserWidget>(pcontroller, GiftUI);

	DiplomacyNotifyUIInstance = CreateWidget<UUserWidget>(pcontroller, DiplomacyNotifyUI);

	LogUIInstance = CreateWidget<UUserWidget>(pcontroller, LogUI);

	if (GetWorld()->GetMapName() == "Map")
		Grid->Load();
}

void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime > 1.0f)
		return;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (bMouseCapture)
		pcontroller->SetMouseLocation(MousePosition.X, MousePosition.Y);

	FSlateApplication& slate = FSlateApplication::Get();
	TSet<FKey> keys = slate.GetPressedMouseButtons();

	if (keys.Contains("LeftMouseButton"))
		ClearPopUI();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	FHitResult hit(ForceInit);

	if (MainMenuUIInstance->IsInViewport() || BuildComponent->IsComponentTickEnabled() || ParliamentUIInstance->IsInViewport())
		return;

	HoveredActor.Reset();

	if (bBulldoze)
		pcontroller->CurrentMouseCursor = EMouseCursor::CardinalCross;
	else
		pcontroller->CurrentMouseCursor = EMouseCursor::Default;

	FVector mouseLoc, mouseDirection;
	pcontroller->DeprojectMousePositionToWorld(mouseLoc, mouseDirection);

	FVector endTrace = mouseLoc + (mouseDirection * 30000);

	if (GetWorld()->LineTraceSingleByChannel(hit, mouseLoc, endTrace, ECollisionChannel::ECC_Visibility)) {
		AActor* actor = hit.GetActor();

		MouseHitLocation = hit.Location;

		if (!actor->IsA<AAI>() && !actor->IsA<ABuilding>() && !actor->IsA<AEggBasket>() && !actor->IsA<AMineral>())
			return;

		HoveredActor.Actor = actor;

		if (actor->IsA<AMineral>())
			HoveredActor.Instance = hit.Item;

		if (!bBulldoze)
			pcontroller->CurrentMouseCursor = EMouseCursor::Hand;
	}
}

void ACamera::SetMouseCapture(bool bCapture)
{
	if (bMouseCapture == bCapture)
		return;

	bMouseCapture = bCapture;

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	pcontroller->SetShowMouseCursor(!bCapture);

	if (bCapture) {
		FInputModeGameOnly inputMode;
		inputMode.SetConsumeCaptureMouseDown(bCapture);
		pcontroller->SetInputMode(inputMode);

		GetWorld()->GetGameViewport()->GetMousePosition(MousePosition);
	}
	else {
		FInputModeGameAndUI inputMode;
		inputMode.SetHideCursorDuringCapture(bCapture);
		pcontroller->SetInputMode(inputMode);
	}
}

void ACamera::StartGame()
{
	BuildComponent->SpawnBuilding(StartBuilding);

	bStartMenu = false;
	bInMenu = false;

	Grid->MapUIInstance->AddToViewport();
	WorldUIInstance->AddToViewport();
}

void ACamera::OnBrochPlace(ABuilding* Broch)
{
	if (PauseUIInstance->IsInViewport())
		Pause();

	for (ASpecial* building :  Grid->SpecialBuildings) {
		if (building->IsHidden()) {
			building->Destroy();
		}
		else {
			TArray<FHitResult> hits = BuildComponent->GetBuildingOverlaps(building);

			for (FHitResult hit : hits) {
				if (!hit.GetActor()->IsA<AVegetation>())
					continue;

				AVegetation* vegetation = Cast<AVegetation>(hit.GetActor());
				Grid->RemoveTree(vegetation, hit.Item);
			}
		}
	}

	bBlockPause = true;

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

	ConquestManager->StartConquest();
	UpdateInteractUI(false);

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

void ACamera::ClearPopUI()
{
	if (BribeUIInstance->IsInViewport()) {
		BribeUIInstance->RemoveFromParent();

		bWasClosingWindow = true;

		return;
	}
	else if (ResearchHoverUIInstance->IsInViewport()) {
		ResearchHoverUIInstance->RemoveFromParent();

		bWasClosingWindow = true;

		return;
	}
	else if (BuildingColourUIInstance->IsInViewport()) {
		BuildingColourUIInstance->RemoveFromParent();

		bWasClosingWindow = true;

		return;
	}
	else if (FactionColourUIInstance->IsInViewport()) {
		FactionColourUIInstance->RemoveFromParent();

		bWasClosingWindow = true;

		return;
	}
	else if (HoursUIInstance->IsInViewport()) {
		HoursUIInstance->RemoveFromParent();

		bWasClosingWindow = true;

		return;
	}
}

void ACamera::SetPause(bool bPause, bool bTickWhenPaused)
{
	float timeDilation = 0.0001f;

	if (!bPause) {
		timeDilation = GameSpeed;

		bTickWhenPaused = bPause;
	}

	SetTimeDilation(timeDilation);
}

void ACamera::SetGameSpeed(float Speed)
{
	GameSpeed = FMath::Clamp(GameSpeed + Speed, 1.0f, 5.0f);

	if (CustomTimeDilation <= 1.0f)
		SetTimeDilation(GameSpeed);

	UpdateSpeedUI(GameSpeed);
}

void ACamera::SetTimeDilation(float Dilation)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), Dilation);

	CustomTimeDilation = 1.0f / Dilation;
}

void ACamera::DisplayInteract(AActor* Actor, int32 Instance)
{
	if (!IsValid(Actor))
		return;
	
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
			Attach(Actor);
	}

	FString socketName = "InfoSocket";

	if (Instance > -1)
		socketName.AppendInt(Instance);

	SetInteractStatus(Actor, true, socketName);
}

void ACamera::SetInteractStatus(AActor* Actor, bool bStatus, FString SocketName)
{
	if (IsValid(Actor)) {
		TArray<UDecalComponent*> decalComponents;
		Actor->GetComponents<UDecalComponent>(decalComponents);

		if (decalComponents.Num() >= 2 && decalComponents[1]->GetDecalMaterial() != nullptr && !ConstructionManager->IsBeingConstructed(Cast<ABuilding>(Actor), nullptr)) {
			decalComponents[1]->SetVisibility(bStatus);

			if (Actor->IsA<ABroadcast>()) {
				ABroadcast* broadcaster = Cast<ABroadcast>(Actor);

				if (bStatus)
					for (AHouse* house : broadcaster->Houses)
						house->BuildingMesh->SetOverlayMaterial(broadcaster->InfluencedMaterial);
				else
					for (AHouse* house : broadcaster->Houses)
						broadcaster->RemoveInfluencedMaterial(house);
			}
		}
	}

	WidgetComponent->SetHiddenInGame(!bStatus);

	if (bStatus) {
		WidgetComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform, *SocketName);

		WidgetComponent->SetWorldLocation(MovementComponent->SetAttachedMovementLocation(Actor, true));
	}
	else {
		WidgetComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void ACamera::Attach(AActor* Actor)
{
	if (ActorAttachedTo == Actor)
		return;
	
	ActorAttachedTo = Actor;

	SpringArmComponent->ProbeSize = 6.0f;
}

void ACamera::Detach()
{
	if (ActorAttachedTo == nullptr)
		return;
	
	SpringArmComponent->ProbeSize = 12.0f;

	ActorAttachedTo = nullptr;

	FVector location = GetActorLocation();
	location.Z = 800.0f;
	MovementComponent->MovementLocation = location;

	FocusedCitizen = nullptr;
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

	SmiteComponent->SetVariablePosition("StartLocation", AI->GetActorLocation() + FVector(0.0f, 0.0f, 2000.0f));
	SmiteComponent->SetVariablePosition("EndLocation", AI->GetActorLocation());

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
			DisplayInteract(HoveredActor.Actor, HoveredActor.Instance);
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

	if (PauseUIInstance->IsInViewport()) {
		PauseUIInstance->RemoveFromParent();

		SetPause(false, false);
	}
	else {
		PauseUIInstance->AddToViewport();

		SetPause(true, true);
	}
}

void ACamera::Menu()
{
	if (Grid->LoadUIInstance->IsInViewport() || bLost || MainMenuUIInstance->IsInViewport())
		return;

	if (!WidgetComponent->bHiddenInGame && !BuildComponent->IsComponentTickEnabled()) {
		SetInteractStatus(WidgetComponent->GetAttachmentRootActor(), false);

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

		bInMenu = false;

		SetPause(false, false);

		if (PauseUIInstance->IsInViewport())
			SetPause(true, true);
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
	if (Grid->LoadUIInstance->IsInViewport() || bInMenu)
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

	SetGameSpeed(Instance.GetValue().Get<float>());
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

	CitizenManager->CreateEvent(type, building, Period, Day, StartHour, EndHour, bRecurring, bFireFestival);
}

void ACamera::DamageActor(int32 Amount)
{
	if (!IsValid(HoveredActor.Actor) || !(HoveredActor.Actor->IsA<AAI>() && HoveredActor.Actor->IsA<ABuilding>()))
		return;

	UHealthComponent* healthComp = HoveredActor.Actor->GetComponentByClass<UHealthComponent>();
	healthComp->TakeHealth(Amount, HoveredActor.Actor);
}