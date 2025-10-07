#include "Building.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NavigationSystem.h"
#include "Components/Widget.h"
#include "Components/AudioComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Components/BoxComponent.h"

#include "AI/Citizen.h"
#include "AI/DiplosimAIController.h"
#include "AI/AIMovementComponent.h"
#include "Universal/HealthComponent.h"
#include "Player/Camera.h"
#include "Player/Managers/ResourceManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Components/BuildComponent.h"
#include "Map/Resources/Vegetation.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Grid.h"
#include "Buildings/Work/Defence/Wall.h"
#include "Buildings/Work/Defence/Trap.h"
#include "Buildings/Work/Defence/Fort.h"
#include "Buildings/Work/Service/Builder.h"
#include "Buildings/Work/Service/Trader.h"
#include "Buildings/Work/Service/Stockpile.h"
#include "Buildings/Work/Booster.h"
#include "Buildings/House.h"
#include "Buildings/Work/Production/Farm.h"
#include "Buildings/Work/Production/InternalProduction.h"
#include "Buildings/Misc/Road.h"
#include "Buildings/Misc/Festival.h"
#include "Universal/EggBasket.h"

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Overlap);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Block);
	BuildingMesh->SetCanEverAffectNavigation(false);
	BuildingMesh->SetGenerateOverlapEvents(false);
	BuildingMesh->bFillCollisionUnderneathForNavmesh = true;
	BuildingMesh->PrimaryComponentTick.bCanEverTick = false;

	RootComponent = BuildingMesh;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->MaxHealth = 100;
	HealthComponent->Health = HealthComponent->MaxHealth;

	ParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ParticleComponent"));
	ParticleComponent->SetupAttachment(RootComponent, "ParticleSocket");
	ParticleComponent->SetUseAutoManageAttachment(true);
	ParticleComponent->SetCastShadow(true);
	ParticleComponent->AutoAttachSocketName = "ParticleSocket";
	ParticleComponent->bAutoActivate = false;

	DestructionComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DestructionComponent"));
	DestructionComponent->SetupAttachment(RootComponent);
	DestructionComponent->SetUseAutoManageAttachment(true);
	DestructionComponent->SetCastShadow(true);
	DestructionComponent->bAutoActivate = false;

	AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudioComponent"));
	AmbientAudioComponent->SetupAttachment(RootComponent);
	AmbientAudioComponent->SetVolumeMultiplier(0.0f);
	AmbientAudioComponent->bCanPlayMultipleInstances = true;
	AmbientAudioComponent->PitchModulationMin = 0.9f;
	AmbientAudioComponent->PitchModulationMax = 1.1f;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(1500.0f, 1500.0f, 1500.0f);
	DecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	DecalComponent->SetVisibility(false);

	GroundDecalComponent = CreateDefaultSubobject<UDecalComponent>("GroundDecalComponent");
	GroundDecalComponent->SetupAttachment(RootComponent);
	GroundDecalComponent->DecalSize = FVector(1.0f, 100.0f, 100.0f);
	GroundDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -0.75f));
	GroundDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	GroundDecalComponent->SetHiddenInGame(true);
	GroundDecalComponent->PrimaryComponentTick.bCanEverTick = false;

	Emissiveness = 0.0f;
	bBlink = false;

	Capacity = 2;
	MaxCapacity = 2;

	bHideCitizen = true;

	bInstantConstruction = false;

	ActualMesh = nullptr;

	bConstant = false;

	bAffectBuildingMesh = false;

	BuildingName = "";

	bUnique = false;

	bCoastal = false;

	SeedNum = 0;

	Tier = 1;

	bOperate = true;

	FactionName = "";

	DeathTime = 0.0f;
}

void ABuilding::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	Camera = PController->GetPawn<ACamera>();

	if (!Colours.IsEmpty()) {
		int32 index = Camera->Grid->Stream.RandRange(0, Colours.Num() - 1);

		ChosenColour = Colours[index];
	}
	else {
		float r = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);
		float g = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);
		float b = Camera->Grid->Stream.FRandRange(0.0f, 1.0f);

		ChosenColour = FLinearColor(r, g, b);
	}

	BuildingMesh->SetCustomPrimitiveDataFloat(1, ChosenColour.R);
	BuildingMesh->SetCustomPrimitiveDataFloat(2, ChosenColour.G);
	BuildingMesh->SetCustomPrimitiveDataFloat(3, ChosenColour.B);

	BuildingMesh->SetCustomPrimitiveDataFloat(5, Emissiveness);

	float blink = 0.0f;
	if (bBlink)
		blink = 1.0f;

	BuildingMesh->SetCustomPrimitiveDataFloat(6, blink);

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

	for (TSubclassOf<AResource> resource : resources) {
		FItemStruct itemStruct;
		itemStruct.Resource = resource;

		if (Storage.Contains(itemStruct))
			continue;

		Storage.Add(itemStruct);
	}

	SetSeed(SeedNum);

	SetLights(Camera->Grid->AtmosphereComponent->Calendar.Hour);
}

void ABuilding::SetLights(int32 Hour)
{
	float oldLights = 0.0f;
	if (BuildingMesh->GetCustomPrimitiveData().Data.Num() > 7)
		oldLights = BuildingMesh->GetCustomPrimitiveData().Data[7];

	float newLights = 0.0f;

	if (Hour >= 18 || Hour < 6)
		newLights = 1.0f;

	if (oldLights != newLights)
		BuildingMesh->SetCustomPrimitiveDataFloat(7, newLights);
}

void ABuilding::ToggleDecalComponentVisibility(bool bVisible)
{
	if (!IsValid(DecalComponent->GetDecalMaterial()) || Camera->ConstructionManager->IsBeingConstructed(this, nullptr))
		return;

	DecalComponent->SetVisibility(bVisible);

	Camera->BuildComponent->DisplayInfluencedBuildings(this, bVisible);
}

void ABuilding::SetSeed(int32 Seed)
{
	if (Seeds.IsEmpty())
		return;

	if (FactionName == Camera->ColonyName)
		Camera->SetInteractStatus(this, false);

	if (bAffectBuildingMesh) {
		if (!Seeds[Seed].Meshes.IsEmpty()) {
			BuildingMesh->SetStaticMesh(Seeds[Seed].Meshes[0]);

			FVector size = Seeds[Seed].Meshes[0]->GetBounds().GetBox().GetSize();

			GroundDecalComponent->DecalSize = FVector(size.X / 2.0f, size.Y / 2.0f, 1.0f);

			if (IsA<AFestival>())
				Cast<AFestival>(this)->BoxAreaAffect->SetBoxExtent(FVector(size.X / 2.0f, size.Y / 2.0f, 20.0f));
		}

		if (Seeds[Seed].Health != -1) {
			HealthComponent->MaxHealth = Seeds[Seed].Health;
			HealthComponent->Health = HealthComponent->MaxHealth;
		}

		if (Seeds[Seed].Capacity != -1) {
			MaxCapacity = Seeds[Seed].Capacity;
			Capacity = MaxCapacity;

			for (int32 i = GetOccupied().Num() - 1; i > Capacity; i--)
				RemoveCitizen(GetOccupied()[i]);
		}

		if (!Seeds[Seed].Cost.IsEmpty())
			CostList = Seeds[Seed].Cost;

		if (IsA<ATrap>())
			Cast<ATrap>(this)->bExplode = Seeds[Seed].bExplosive;

		if (IsA<ARoad>() || IsA<AFestival>())
			SetTier(Seeds[Seed].Tier);
	}
	else {
		TArray<USceneComponent*> children;
		BuildingMesh->GetChildrenComponents(true, children);

		for (USceneComponent* component : children)
			component->DestroyComponent();

		TArray<FName> sockets = BuildingMesh->GetAllSocketNames();

		if (IsA<AFarm>())
			Cast<AFarm>(this)->CropMeshes.Empty();

		for (FName socket : sockets) {
			if (!socket.ToString().Contains("Random"))
				continue;

			int32 num = 0;
			int32 angle = Camera->Grid->Stream.RandRange(0, 3);

			if (IsA<AInternalProduction>())
				angle = 0;

			if (Seeds[Seed].Meshes.Num() > 1) {
				FString result = socket.ToString().Replace(TEXT("Random"), TEXT(""), ESearchCase::IgnoreCase);
				num = FCString::Atoi(*result) - 1;
			}

			UStaticMeshComponent* meshComp = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
			meshComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			meshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			meshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
			meshComp->SetStaticMesh(Seeds[Seed].Meshes[num]);
			meshComp->SetupAttachment(BuildingMesh, socket);
			meshComp->SetRelativeRotation(FRotator(0.0f, 90.0f * angle, 0.0f));
			meshComp->RegisterComponent();

			meshComp->SetCustomPrimitiveDataFloat(1, ChosenColour.R);
			meshComp->SetCustomPrimitiveDataFloat(2, ChosenColour.G);
			meshComp->SetCustomPrimitiveDataFloat(3, ChosenColour.B);

			if (IsA<AFarm>()) {
				meshComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.0f));

				Cast<AFarm>(this)->CropMeshes.Add(meshComp);
			}

			const UStaticMeshSocket* pSocket = meshComp->GetSocketByName("ParticleSocket");

			if (pSocket != nullptr) {
				UStaticMeshSocket* p = const_cast<UStaticMeshSocket*>(meshComp->GetSocketByName("ParticleSocket"));

				UStaticMeshSocket* s = const_cast<UStaticMeshSocket*>(BuildingMesh->GetSocketByName("ParticleSocket"));
				s->RelativeLocation = p->RelativeLocation;
			}
		}

		if (IsA<AFarm>()) {
			AFarm* farm = Cast<AFarm>(this);

			farm->Crop = Seeds[Seed].Resource;
			farm->Yield = Seeds[Seed].Yield;
			farm->TimeLength = Seeds[Seed].TimeLength;

			BuildingName = Seeds[Seed].Name;
		}
		else if (Seeds[Seed].Resource != nullptr) {
			AInternalProduction* prod = Cast<AInternalProduction>(this);

			prod->ResourceToOverlap = Seeds[Seed].Resource;
			prod->MaxYield = Seeds[Seed].Yield;
			prod->TimeLength = Seeds[Seed].TimeLength;

			prod->WorkHat = Seeds[Seed].WorkHat;
		}
	}

	ParticleComponent->SetAsset(Seeds[Seed].NiagaraSystem);

	if (Seeds[Seed].Name != "") {
		BuildingName = Seeds[Seed].Name;

		Camera->UpdateDisplayName();
	}

	SeedNum = Seed;

	if (FactionName == Camera->ColonyName)
		Camera->DisplayInteract(this);
}

int32 ABuilding::GetTier()
{
	return Tier;
}

void ABuilding::SetTier(int32 Value)
{
	Tier = Value;
	
	float r = 0;
	float g = 0;
	float b = 0;

	if (Value == 1) {
		r = 0.473531;
		g = 0.361307;
		b = 0.187821;
	}
	else if (Value == 2) {
		r = 0.571125;
		g = 0.590619;
		b = 0.64448;
	}

	SetBuildingColour(r, g, b);
}

void ABuilding::SetBuildingColour(float R, float G, float B)
{
	BuildingMesh->SetCustomPrimitiveDataFloat(1, R);
	BuildingMesh->SetCustomPrimitiveDataFloat(2, G);
	BuildingMesh->SetCustomPrimitiveDataFloat(3, B);

	ChosenColour = FLinearColor(R, G, B);
}

TArray<FItemStruct> ABuilding::GetRebuildCost()
{
	TArray<FItemStruct> items;

	for (FItemStruct item : CostList) {
		item.Amount = FMath::CeilToInt(item.Amount / 3.0f);

		items.Add(item);
	}

	return items;
}

void ABuilding::Rebuild(FString NewFactionName)
{
	if (!Camera->bInstantBuildCheat) {
		for (FItemStruct item : GetRebuildCost()) {
			int32 amount = Camera->ResourceManager->GetResourceAmount(FactionName, item.Resource);

			if (amount < item.Amount) {
				if (FactionName == Camera->ColonyName)
					Camera->ShowWarning("Cannot afford building");

				return;
			}
		}
	}

	FactionName = NewFactionName;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (faction->RuinedBuildings.Contains(this))
		faction->RuinedBuildings.Remove(this);

	Build(true);

	BuildingMesh->SetCustomPrimitiveDataFloat(8, 0.0f);

	GroundDecalComponent->SetHiddenInGame(true);
}

void ABuilding::Build(bool bRebuild, bool bUpgrade, int32 Grade)
{
	BuildingMesh->SetOverlayMaterial(nullptr);

	BuildingMesh->SetCanEverAffectNavigation(true);

	BuildingMesh->bReceivesDecals = true;

	UResourceManager* rm = Camera->ResourceManager;
	UConstructionManager* cm = Camera->ConstructionManager;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	TargetList = CostList;

	if (bRebuild)
		TargetList = GetRebuildCost();
	else if (bUpgrade) {
		TargetList = GetGradeCost(Grade);

		for (int32 i = TargetList.Num() - 1; i > -1; i--) {
			if (TargetList[i].Amount >= 0)
				continue;

			rm->AddUniversalResource(faction, TargetList[i].Resource, FMath::Abs(TargetList[i].Amount) / 2.0f);

			TargetList.RemoveAt(i);
		}

		for (int32 i = GetOccupied().Num() - 1; i > -1; i--)
			RemoveCitizen(GetOccupied()[i]);

		SetSeed(Grade);
	}

	if (CheckInstant()) {
		OnBuilt();

		if (Camera->bInstantBuildCheat)
			return;

		for (FItemStruct item : TargetList)
			rm->TakeUniversalResource(faction, item.Resource, item.Amount, 0);
	} else {
		for (FItemStruct item : TargetList)
			rm->AddCommittedResource(faction, item.Resource, item.Amount);

		cm->AddBuilding(this, EBuildStatus::Construction);

		ActualMesh = BuildingMesh->GetStaticMesh();
		FVector bSize = ActualMesh->GetBounds().GetBox().GetSize();
		FVector cSize = ConstructionMesh->GetBounds().GetBox().GetSize();

		FVector size = bSize / cSize;

		if (size.Z < 0.75f)
			size.Z = 0.75f;

		BuildingMesh->SetRelativeScale3D(size);
		BuildingMesh->SetStaticMesh(ConstructionMesh);
	}
}

void ABuilding::DestroyBuilding(bool bCheckAbove)
{
	UResourceManager* rm = Camera->ResourceManager;
	UConstructionManager* cm = Camera->ConstructionManager;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

	if (cm->IsBeingConstructed(this, nullptr)) {
		for (FItemStruct items : CostList) {
			rm->AddUniversalResource(faction, items.Resource, items.Stored);

			rm->TakeCommittedResource(faction, items.Resource, items.Amount - items.Stored);
		}
	}
	else {
		for (FItemStruct items : CostList)
			rm->AddUniversalResource(faction, items.Resource, items.Stored / 2.0f);
	}

	if (IsA(Camera->BuildComponent->FoundationClass)) {
		FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorLocation());

		if (tile->bRiver)
			Camera->Grid->HISMRiver->PerInstanceSMCustomData[tile->Instance * 4] = 1.0f;
	}

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);

	for (TSubclassOf<AResource> resource : resources)
		rm->TakeLocalResource(resource, this, Capacity);

	cm->RemoveBuilding(this);

	for (ACitizen* citizen : GetOccupied()) {
		if (!IsValid(citizen))
			continue;

		RemoveCitizen(citizen);

		citizen->AIController->DefaultAction();
	}

	if (!Camera->BuildComponent->Buildings.Contains(this) && (IsA(Camera->BuildComponent->FoundationClass) || IsA(Camera->BuildComponent->RampClass) || IsA<ARoad>())) {
		if (IsA(Camera->BuildComponent->FoundationClass)) {
			FCollisionQueryParams params;
			params.AddIgnoredActor(this);
			params.AddIgnoredActor(Camera->Grid);

			FHitResult hit;

			while (bCheckAbove) {
				if (GetWorld()->LineTraceSingleByChannel(hit, GetActorLocation() + FVector(0.0f, 0.0f, 20000.0f), GetActorLocation(), ECollisionChannel::ECC_Visibility, params) && hit.GetActor()->GetActorLocation().Z > GetActorLocation().Z)
					Cast<ABuilding>(hit.GetActor())->DestroyBuilding(false);
				else
					bCheckAbove = false;
			}
		}

		Camera->BuildComponent->RemoveWalls(this);

		FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorLocation());

		if (IsA(Camera->BuildComponent->FoundationClass))
			tile->Level--;
		else if (IsA(Camera->BuildComponent->RampClass))
			tile->bRamp = false;

		for (auto &element : tile->AdjacentTiles)
			Camera->Grid->CreateEdgeWalls(element.Value);

		Camera->Grid->CreateEdgeWalls(tile);

		Camera->Grid->HISMWall->BuildTreeIfOutdated(true, false);
	}

	if (faction->Buildings.Contains(this))
		faction->Buildings.Remove(this);

	if (IsA(Camera->CitizenManager->PoliceStationClass)) {
		for (ACitizen* citizen : faction->Citizens) {
			if (citizen->Building.BuildingAt != this || GetOccupied().Contains(citizen) || !faction->Police.Arrested.Contains(citizen))
				continue;

			Camera->CitizenManager->SetInNearestJail(faction, nullptr, citizen);
		}
	}

	rm->UpdateResourceCapacityUI(this);

	Destroy();
}

TArray<FItemStruct> ABuilding::GetGradeCost(int32 Grade)
{
	TArray<FItemStruct> items;

	items = Seeds[Grade].Cost;

	for (FItemStruct item : Seeds[SeedNum].Cost) {
		item.Amount = -item.Amount;

		int32 index = items.Find(item);

		if (index != INDEX_NONE)
			items[index].Amount += item.Amount;
		else
			items.Add(item);
	}

	return items;
}

void ABuilding::OnBuilt()
{
	if (ActualMesh != nullptr) {
		BuildingMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		BuildingMesh->SetStaticMesh(ActualMesh);
	}

	if (IsA(Camera->BuildComponent->FoundationClass))
		BuildingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	Camera->ConstructionManager->RemoveBuilding(this);

	HealthComponent->Health = HealthComponent->MaxHealth;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);
	faction->Buildings.Add(this);

	if (bConstant && ParticleComponent->GetAsset() != nullptr)
		ParticleComponent->Activate();

	Camera->ResourceManager->UpdateResourceCapacityUI(this);
}

void ABuilding::AlterOperate()
{
	bOperate = !bOperate;

	if (bOperate)
		return;

	for (ACitizen* citizen : GetOccupied())
		RemoveCitizen(citizen);
}

bool ABuilding::AddCitizen(ACitizen* Citizen)
{
	if (GetCapacity() <= Occupied.Num() || !bOperate)
		return false;

	FOccupantStruct occupant;
	occupant.Occupant = Citizen;

	Occupied.Add(occupant);

	return true;
}

bool ABuilding::RemoveCitizen(ACitizen* Citizen)
{
	if (!GetOccupied().Contains(Citizen))
		return false;

	if (Citizen->Building.BuildingAt == this ||  Citizen->HealthComponent->GetHealth() != 0)
		Leave(Citizen);

	for (ACitizen* citizen : GetVisitors(Citizen))
		RemoveVisitor(Citizen, citizen);

	FOccupantStruct occupant;
	occupant.Occupant = Citizen;

	Occupied.Remove(occupant);

	return true;
}

ACitizen* ABuilding::GetOccupant(class ACitizen* Citizen)
{
	if (GetOccupied().Contains(Citizen))
		return Citizen;

	for (FOccupantStruct occupant : Occupied) {
		if (!occupant.Visitors.Contains(Citizen))
			continue;

		return occupant.Occupant;
	}

	return nullptr;
}

TArray<ACitizen*> ABuilding::GetVisitors(ACitizen* Occupant)
{
	FOccupantStruct occupant;
	occupant.Occupant = Occupant;

	int32 index = Occupied.Find(occupant);

	return Occupied[index].Visitors;
}

void ABuilding::AddVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	FOccupantStruct occupant;
	occupant.Occupant = Occupant;

	int32 index = Occupied.Find(occupant);

	if (index == INDEX_NONE)
		return;

	Occupied[index].Visitors.Add(Visitor);

	if (!IsA<AFestival>())
		Visitor->AIController->DefaultAction();
}

void ABuilding::RemoveVisitor(ACitizen* Occupant, ACitizen* Visitor)
{
	FOccupantStruct occupant;
	occupant.Occupant = Occupant;

	int32 index = Occupied.Find(occupant);

	if (index != INDEX_NONE)
		Occupied[index].Visitors.Remove(Visitor);

	if (Visitor->Building.BuildingAt == this)
		Leave(Visitor);

	Visitor->AIController->DefaultAction();
}

TArray<ACitizen*> ABuilding::GetCitizensAtBuilding()
{
	TArray<ACitizen*> citizens;

	for (ACitizen* citizen : GetOccupied()) {
		if (!IsValid(citizen) || citizen->Building.BuildingAt != this)
			continue;

		citizens.Add(citizen);
	}

	return citizens;
}

void ABuilding::StoreSocketLocations()
{
	SocketList.Empty();
	
	TArray<FName> sockets = BuildingMesh->GetAllSocketNames();

	FSocketStruct socketStruct;

	for (FName socket : sockets) {
		if (!socket.ToString().Contains("Position"))
			continue;

		socketStruct.Name = socket;
		socketStruct.SocketLocation = BuildingMesh->GetSocketLocation(socket);
		socketStruct.SocketRotation = BuildingMesh->GetSocketRotation(socket);

		SocketList.Add(socketStruct);
	}
}

void ABuilding::SetSocketLocation(class ACitizen* Citizen)
{
	if (!GetOccupied().Contains(Citizen) && (Occupied.IsEmpty() || !Occupied[0].Visitors.Contains(Citizen)))
		return;

	bool bAnim = false;

	FSocketStruct socketStruct;
	socketStruct.Citizen = Citizen;

	int32 index = SocketList.Find(socketStruct);

	if (index != INDEX_NONE) {
		Citizen->MovementComponent->Transform.SetLocation(SocketList[index].SocketLocation);
		Citizen->MovementComponent->Transform.SetRotation(SocketList[index].SocketRotation.Quaternion());
	}
	else if (!SocketList.IsEmpty()) {
		for (int32 i = 0; i < SocketList.Num(); i++) {
			if (SocketList[i].Citizen != nullptr)
				continue;

			SocketList[i].Citizen = Citizen;

			Citizen->MovementComponent->Transform.SetLocation(SocketList[i].SocketLocation);
			Citizen->MovementComponent->Transform.SetRotation(SocketList[i].SocketRotation.Quaternion());

			index = i;

			break;
		}
	}
	else if (bHideCitizen || Camera->ConstructionManager->IsBeingConstructed(this, nullptr)) {
		Citizen->MovementComponent->Transform.SetLocation(GetActorLocation());
	}

	if (AnimSockets.IsEmpty())
		return;

	EAnim anim = EAnim::Still;
	bool bRepeat = false;

	TArray<FName> keys;
	AnimSockets.GenerateKeyArray(keys);

	if (keys[0] == FName("All"))
		anim = *AnimSockets.Find(keys[0]);
	else if (index != INDEX_NONE)
		anim = *AnimSockets.Find(SocketList[index].Name);

	if (anim != EAnim::Still)
		bRepeat = true;

	float speed = 2.0f;

	if (!IsA<AFestival>())
		speed = Citizen->GetProductivity();

	Citizen->MovementComponent->SetAnimation(anim, bRepeat, speed);
}

void ABuilding::Enter(ACitizen* Citizen)
{
	if (Citizen->Building.BuildingAt == this)
		return;
	
	Citizen->Building.BuildingAt = this;
	Citizen->Building.EnterLocation = Citizen->MovementComponent->Transform.GetLocation();

	SetSocketLocation(Citizen);

	if (!Inside.Contains(Citizen))
		Inside.Add(Citizen);

	if (GetCitizensAtBuilding().Num() == 1 && !bConstant && ParticleComponent->GetAsset() != nullptr)
		ParticleComponent->Activate();
	
	UConstructionManager* cm = Camera->ConstructionManager;

	if (bHideCitizen || cm->IsBeingConstructed(this, nullptr))
		if (Camera->FocusedCitizen == Citizen)
			Camera->Attach(this);

	if (Citizen->Carrying.Amount > 0)
		StoreResource(Citizen);

	if (IsValid(Citizen->Building.Employment) && Citizen->Building.Employment->IsA<ABuilder>() && cm->IsBeingConstructed(this, nullptr)) {
		ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

		if (cm->IsRepairJob(this, builder))
			builder->StartRepairTimer(Citizen, this);
		else
			builder->CheckCosts(Citizen, this);
	}
	else if (!IsA<AHouse>() && !IsA<ABooster>() && !GetOccupied().Contains(Citizen) && IsValid(Citizen->Building.Employment)) {
		TArray<FItemStruct> items;
		ABuilding* deliverTo = nullptr;

		if (Citizen->Building.Employment->IsA<ABuilder>()) {
			ABuilder* builder = Cast<ABuilder>(Citizen->Building.Employment);

			deliverTo = cm->GetBuilding(builder);
			items = deliverTo->CostList;
		}
		else if (Citizen->Building.Employment->IsA<ATrader>()) {
			ATrader* trader = Cast<ATrader>(Citizen->Building.Employment);

			deliverTo = trader;

			if (!trader->Orders[0].bCancelled)
				items = trader->Orders[0].SellingItems;
		}
		else if (Citizen->Building.Employment->IsA<AStockpile>()) {
			AStockpile* stockpile = Cast <AStockpile>(Citizen->Building.Employment);

			deliverTo = stockpile;

			items.Add(stockpile->GetItemToGather(Citizen));
		}
		
		if (deliverTo == nullptr)
			return;
		else if (items.IsEmpty())
			Citizen->AIController->AIMoveTo(deliverTo);
		else
			CarryResources(Citizen, deliverTo, items);
	}
}

void ABuilding::Leave(ACitizen* Citizen)
{
	Citizen->Building.BuildingAt = nullptr;

	Inside.Remove(Citizen);

	if (GetCitizensAtBuilding().IsEmpty() && !bConstant && ParticleComponent->GetAsset() != nullptr)
		ParticleComponent->Deactivate();

	FSocketStruct socketStruct;
	socketStruct.Citizen = Citizen;

	int32 index = SocketList.Find(socketStruct);

	if (index != INDEX_NONE)
		SocketList[index].Citizen = nullptr;

	if (!Citizen->IsHidden())
		return;

	if (Camera->FocusedCitizen == Citizen)
		Camera->Attach(Citizen);

	FHitResult hit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FVector location = Citizen->Building.EnterLocation;

	if (BuildingMesh->DoesSocketExist("Entrance")) {
		FVector pos = BuildingMesh->GetSocketLocation("Entrance");

		if (GetWorld()->LineTraceSingleByChannel(hit, pos, pos - FVector(0.0f, 0.0f, 200.0f), ECollisionChannel::ECC_WorldStatic, QueryParams))
			if (hit.GetActor()->IsA<AGrid>() && hit.GetComponent() == Cast<AGrid>(hit.GetActor())->HISMGround)
				location = pos;
	}

	Citizen->MovementComponent->Transform.SetLocation(location);
}

bool ABuilding::CheckInstant()
{
	if (Camera->bInstantBuildCheat)
		bInstantConstruction = true;

	return bInstantConstruction;
}

void ABuilding::AddCapacity()
{
	if (Capacity == MaxCapacity)
		return;

	Capacity++;
}

void ABuilding::RemoveCapacity()
{
	if (Capacity == 0)
		return;

	Capacity--;

	if (GetOccupied().Num() <= GetCapacity())
		return;

	RemoveCitizen(GetOccupied().Last());
}

int32 ABuilding::GetCapacity()
{
	return Capacity;
}

TArray<ACitizen*> ABuilding::GetOccupied()
{
	TArray<ACitizen*> citizens;

	for (FOccupantStruct &occupant : Occupied)
		citizens.Add(occupant.Occupant);

	return citizens;
}

bool ABuilding::CheckStored(ACitizen* Citizen, TArray<FItemStruct> Items)
{
	for (FItemStruct item : Items) {
		if (item.Stored == item.Amount)
			continue;

		Citizen->AIController->GetGatherSite(item.Resource);

		return false;
	}

	return true;
}

void ABuilding::CarryResources(ACitizen* Citizen, ABuilding* DeliverTo, TArray<FItemStruct> Items)
{
	int32 amount = 0;
	int32 capacity = 10 * Citizen->GetProductivity();

	TSubclassOf<AResource> resource = nullptr;
	int32 stored = 0;

	for (FItemStruct item : Storage) {
		if (Items.Contains(item)) {
			resource = item.Resource;

			stored = item.Amount;

			break;
		}
	}

	if (capacity > stored)
		capacity = stored;

	for (FItemStruct item : Items) {
		if (resource == item.Resource) {
			amount = FMath::Clamp(item.Amount - item.Stored, 0, capacity);

			break;
		}
	}

	Camera->ResourceManager->TakeLocalResource(resource, this, amount);

	if (!DeliverTo->IsA<AStockpile>()) {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(FactionName);

		Camera->ResourceManager->TakeCommittedResource(faction, resource, amount);
	}

	Citizen->Carry(Cast<AResource>(resource->GetDefaultObject()), amount, DeliverTo);
}

void ABuilding::StoreResource(ACitizen* Citizen)
{
	if (Citizen->Carrying.Type == nullptr)
		return;

	TArray<TSubclassOf<AResource>> resources = Camera->ResourceManager->GetResources(this);
	TSubclassOf<AResource> resource = Citizen->Carrying.Type->GetClass();

	if (!resources.IsEmpty() && resources.Contains(resource)) {

		if (IsA<AWork>() && Cast<AWork>(this)->Boosters != 0)
			Citizen->Carrying.Amount *= (1.50f * Cast<AWork>(this)->Boosters);

		int32 extra = Camera->ResourceManager->AddLocalResource(resource, this, Citizen->Carrying.Amount);

		if (extra > 0)
			AddToBasket(resource, extra);

		Citizen->Carry(nullptr, 0, nullptr);
	}
	else {
		UConstructionManager* cm = Camera->ConstructionManager;

		TArray<FItemStruct> items;

		if (cm->IsBeingConstructed(this, nullptr))
			items = CostList;
		else if (IsA<ATrader>())
			items = Cast<ATrader>(this)->Orders[0].SellingItems;
		else
			items = Cast<AInternalProduction>(this)->Intake;

		for (int32 i = 0; i < items.Num(); i++) {
			if (resource != items[i].Resource)
				continue;

			if (cm->IsBeingConstructed(this, nullptr))
				CostList[i].Stored += Citizen->Carrying.Amount;
			else if (IsA<ATrader>())
				Cast<ATrader>(this)->Orders[0].SellingItems[i].Stored += Citizen->Carrying.Amount;
			else
				Cast<AInternalProduction>(this)->Intake[i].Stored += Citizen->Carrying.Amount;

			Citizen->Carry(nullptr, 0, nullptr);

			break;
		}
	}
}

void ABuilding::AddToBasket(TSubclassOf<AResource> Resource, int32 Amount)
{
	FGuid id = FGuid().NewGuid();

	Camera->CitizenManager->CreateTimer("Basket", this, 300.0f, FTimerDelegate::CreateUObject(this, &ABuilding::RemoveFromBasket, id), false);

	FBasketStruct basketStruct;
	basketStruct.ID = id;
	basketStruct.Item.Resource = Resource;
	basketStruct.Item.Amount = Amount;

	Basket.Add(basketStruct);
}

void ABuilding::RemoveFromBasket(FGuid ID)
{
	FBasketStruct basketStruct;
	basketStruct.ID = ID;

	int32 index = Basket.Find(basketStruct);

	if (index == INDEX_NONE)
		return;

	Basket.RemoveAt(index);
}