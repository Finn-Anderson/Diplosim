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
#include "Components/BuildComponent.h"
#include "Components/CameraMovementComponent.h"
#include "Managers/ResourceManager.h"
#include "Managers/ConstructionManager.h"
#include "Managers/CitizenManager.h"
#include "Managers/ResearchManager.h"
#include "Buildings/Building.h"
#include "Buildings/House.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Misc/Festival.h"
#include "Buildings/Work/Service/Religion.h"
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
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->bDoCollisionTest = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
	CameraComponent->PrimaryComponentTick.bCanEverTick = false;
	CameraComponent->PostProcessSettings.AutoExposureMinBrightness = 0.0f;
	CameraComponent->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;

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

	ResearchManager = CreateDefaultSubobject<UResearchManager>(TEXT("ResearchManager"));

	WidgetSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("WidgetSpringArmComponent"));
	WidgetSpringArmComponent->SetupAttachment(RootComponent);
	WidgetSpringArmComponent->SetTickableWhenPaused(true);
	WidgetSpringArmComponent->TargetArmLength = 0.0f;
	WidgetSpringArmComponent->bUsePawnControlRotation = true;
	WidgetSpringArmComponent->bEnableCameraLag = true;
	WidgetSpringArmComponent->CameraLagSpeed = 15.0f;
	WidgetSpringArmComponent->bDoCollisionTest = false;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(WidgetSpringArmComponent);
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

	ActorAttachedTo = nullptr;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();
	settings->Camera = this;
	settings->SetVignette(settings->GetVignette());

	ResourceManager->GameMode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	GetWorldTimerManager().SetTimer(ResourceManager->ValueTimer, ResourceManager, &UResourceManager::SetTradeValues, 300.0f, true);

	APlayerController* pcontroller = UGameplayStatics::GetPlayerController(GetWorld(), 0);

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

	FVector endTrace = mouseLoc + (mouseDirection * 10000);

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
}

void ACamera::OnBrochPlace(ABuilding* Broch)
{
	if (PauseUIInstance->IsInViewport())
		Pause();

	bBlockPause = true;

	Grid->MapUIInstance->RemoveFromParent();

	ResourceManager->RandomiseMarket();

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->Camera = this;
	gamemode->Broch = Broch;
	gamemode->Grid = Grid;

	gamemode->SetWaveTimer();

	FTimerStruct timer;

	timer.CreateTimer("Interest", this, 300, FTimerDelegate::CreateUObject(ResourceManager, &UResourceManager::Interest), true, true);
	CitizenManager->Timers.Add(timer);

	CitizenManager->StartDiseaseTimer();
	CitizenManager->BrochLocation = Broch->GetActorLocation();

	Cast<ABroch>(Broch)->SpawnCitizens();

	Start = false;

	DisplayEvent("Welcome to", ColonyName);

	FTimerHandle displayBuildUITimer;
	GetWorld()->GetTimerManager().SetTimer(displayBuildUITimer, this, &ACamera::DisplayBuildUI, 2.7f, false);
}

void ACamera::PlayAmbientSound(UAudioComponent* AudioComponent, USoundBase* Sound)
{
	if (MainMenuUIInstance->IsInViewport() || Grid->Storage.IsEmpty())
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
	bBlockPause = false;

	BuildUIInstance->AddToViewport();
}

void ACamera::ShowWarning(FString Warning)
{
	DisplayWarning(Warning);
}

void ACamera::SetPause(bool bPause, bool bTickWhenPaused)
{
	float timeDilation = 0.0001f;

	if (!bPause) {
		timeDilation = 1.0f;

		bTickWhenPaused = bPause;
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), timeDilation);

	if (bTickWhenPaused)
		CustomTimeDilation = 1.0f / timeDilation;
	else
		CustomTimeDilation = 1.0f;
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

	if (bStatus)
		WidgetSpringArmComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, *SocketName);
	else
		WidgetSpringArmComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
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

	FTimerStruct timer;
	timer.CreateTimer("Smite", GetOwner(), timeToCompleteDay, FTimerDelegate::CreateUObject(this, &ACamera::IncrementSmites, -1), false);

	CitizenManager->Timers.Add(timer);
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
}

void ACamera::Action(const struct FInputActionInstance& Instance)
{
	if (BribeUIInstance->IsInViewport())
		BribeUIInstance->RemoveFromParent();

	if (ResearchHoverUIInstance->IsInViewport())
		ResearchHoverUIInstance->RemoveFromParent();

	if (BuildingColourUIInstance->IsInViewport()) {
		BuildingColourUIInstance->RemoveFromParent();

		return;
	}
	
	if (bInMenu || ParliamentUIInstance->IsInViewport() || ResearchUIInstance->IsInViewport() || bBulldoze)
		return;

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

		if (WidgetSpringArmComponent->GetAttachParent() != RootComponent && !IsValid(HoveredActor.Actor)) {
			SetInteractStatus(WidgetSpringArmComponent->GetAttachmentRootActor(), false);

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
	if (!bBulldoze || !IsValid(HoveredActor.Actor) || !HoveredActor.Actor->IsA<ABuilding>() || !Cast<ABuilding>(HoveredActor.Actor)->bCanDestroy)
		return;

	UDiplosimUserSettings* settings = UDiplosimUserSettings::GetDiplosimUserSettings();

	SetInteractAudioSound(BuildComponent->PlaceSound, settings->GetMasterVolume() * settings->GetSFXVolume());
	PlayInteractSound();

	Cast<ABuilding>(HoveredActor.Actor)->DestroyBuilding();
}

void ACamera::Cancel()
{
	if (Start || bInMenu)
		return;

	if (bBulldoze)
		bBulldoze = false;
	else if (BuildComponent->IsComponentTickEnabled())
		BuildComponent->RemoveBuilding();
}

void ACamera::NewMap()
{
	if (!Start || bInMenu)
		return;

	Grid->Clear();
	Grid->Load();

	BuildComponent->Buildings[0]->SetActorLocation(FVector(0.0f, 0.0f, -1000.0f));
}

void ACamera::Pause()
{
	if (bInMenu || bBlockPause)
		return;

	if (PauseUIInstance->IsInViewport()) {
		PauseUIInstance->RemoveFromParent();

		SetPause(false, false);

		GetWorldTimerManager().UnPauseTimer(ResourceManager->ValueTimer);
	}
	else {
		PauseUIInstance->AddToViewport();

		SetPause(true, true);

		GetWorldTimerManager().PauseTimer(ResourceManager->ValueTimer);
	}
}

void ACamera::Menu()
{
	if (bLost || MainMenuUIInstance->IsInViewport())
		return;

	if (!WidgetComponent->bHiddenInGame && !BuildComponent->IsComponentTickEnabled()) {
		SetInteractStatus(WidgetSpringArmComponent->GetAttachmentRootActor(), false);

		return;
	}

	if (ParliamentUIInstance->IsInViewport()) {
		ParliamentUIInstance->RemoveFromParent();

		BribeUIInstance->RemoveFromParent();

		return;
	}

	if (bInMenu) {
		if (SettingsUIInstance->IsInViewport())
			UDiplosimUserSettings::GetDiplosimUserSettings()->SaveIniSettings();

		SettingsUIInstance->RemoveFromParent();

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
	if (bInMenu)
		return;

	BuildComponent->RotateBuilding(Instance.GetValue().Get<bool>());
}

void ACamera::ActivateLook(const struct FInputActionInstance& Instance)
{
	if (bInMenu && !bMouseCapture)
		return;

	if (((bool)(Instance.GetTriggerEvent() & ETriggerEvent::Completed)))
		SetMouseCapture(false);
	else
		SetMouseCapture(true);
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

	Detach();

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

//
// Debugging
//
void ACamera::SpawnEnemies()
{
	if (bInMenu)
		return;

	ADiplosimGameModeBase* gamemode = Cast<ADiplosimGameModeBase>(GetWorld()->GetAuthGameMode());
	gamemode->SpawnEnemiesAsync();
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