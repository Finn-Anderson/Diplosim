#include "Player/Components/DiplosimSaveGame.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/WidgetComponent.h"

#include "Kismet/KismetMathLibrary.h"

#include "AI/Clone.h"
#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "AI/AISpawner.h"
#include "AI/Citizen/Citizen.h"
#include "AI/Citizen/Components/BuildingComponent.h"
#include "AI/Citizen/Components/HappinessComponent.h"
#include "AI/Citizen/Components/BioComponent.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/Builder.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/AIVisualiser.h"
#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Managers/ConquestManager.h"
#include "Player/Managers/ArmyManager.h"
#include "Player/Managers/DiplosimTimerManager.h"
#include "Player/Managers/DiseaseManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Player/Components/BuildComponent.h"
#include "Player/Components/SaveGameComponent.h"
#include "Player/Components/DiplomacyComponent.h"
#include "Universal/EggBasket.h"
#include "Universal/HealthComponent.h"
#include "Universal/AttackComponent.h"
#include "Universal/Projectile.h"

//
// Saving
//
void UDiplosimSaveGame::SaveGame(ACamera* Camera, int32 Index, FString ID)
{
	TArray<AActor*> foundActors;

	TArray<AActor*> actors = { Camera, Camera->Grid };
	TArray<AActor*> potentialWetActors;

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AResource::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AAISpawner::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AProjectile::StaticClass(), foundActors);
	actors.Append(foundActors);

	Saves[Index].EmptyData();
	int32 citizenNum = 0;

	for (AActor* actor : actors)
	{
		if (!IsValid(actor) || actor->IsPendingKillPending() || (Camera->BuildComponent->Buildings.Contains(actor) && !actor->IsA<ABroch>()))
			continue;

		FActorSaveData actorData;
		actorData.Name = actor->GetName();
		actorData.Class = actor->GetClass();
		actorData.Transform = actor->GetActorTransform();

		if (actor->IsA<AGrid>())
			SaveWorld(actorData, actor, Index, potentialWetActors);
		else if (actor->IsA<AResource>())
			SaveResource(Camera, actorData, actor, Index);
		else if (actor->IsA<ACamera>()) {
			SaveCamera(actorData, actor, Index);

			SaveCitizenManager(actorData, actor, Index);

			SaveFactions(actorData, actor, Index);

			SaveGamemode(actorData, actor, Index);
		}
		else if (actor->IsA<AAI>()) {
			FAIData aiData;
			SaveAI(Camera, actorData, aiData, actor, Index);

			if (actor->IsA<ACitizen>()) {
				SaveCitizen(Camera, actorData, aiData, actor, Index);

				citizenNum++;
			}

			actorData.dataIndex = Saves[Index].AIData.Add(aiData);
		}
		else if (actor->IsA<ABuilding>())
			SaveBuilding(Camera, actorData, actor, Index);
		else if (actor->IsA<AProjectile>())
			SaveProjectile(actorData, actor, Index);
		else if (actor->IsA<AAISpawner>())
			SaveAISpawner(actorData, actor, Index);

		SaveTimers(Camera, actorData, actor);

		SaveComponents(actorData, actor, Index);

		Saves[Index].SavedActors.Add(actorData);
	}

	Saves[Index].CitizenNum = citizenNum;
	LastTimeUpdated = FDateTime::Now();

	UGameplayStatics::AsyncSaveGameToSlot(this, ID, 0);
}

void UDiplosimSaveGame::SaveWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TArray<AActor*> PotentialWetActors)
{
	AGrid* grid = Cast<AGrid>(Actor);
	FWorldSaveData worldSaveData;
	
	worldSaveData.Size = grid->Size;
	worldSaveData.Chunks = grid->Chunks;

	worldSaveData.Stream = grid->Camera->Stream;

	worldSaveData.LavaSpawnLocations = grid->LavaSpawnLocations;

	worldSaveData.Tiles.Empty();

	auto bound = grid->GetMapBounds();
	for (TArray<FTileStruct>& row : grid->Storage) {
		for (FTileStruct& tile : row) {
			FTileData t;
			t.Level = tile.Level;
			t.Fertility = tile.Fertility;
			t.X = tile.X;
			t.Y = tile.Y;
			t.Rotation = tile.Rotation;
			t.bRamp = tile.bRamp;
			t.bRiver = tile.bRiver;
			t.bEdge = tile.bEdge;
			t.bMineral = tile.bMineral;
			t.bUnique = tile.bUnique;

			worldSaveData.Tiles.Add(t);
		}
	}

	TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea };

	UCloudComponent* clouds = grid->AtmosphereComponent->Clouds;

	for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
		FHISMData data;
		data.Name = hism->GetName();

		for (int32 i = 0; i < hism->GetInstanceCount(); i++) {
			FTransform transform;
			hism->GetInstanceTransform(i, transform);

			data.Transforms.Add(transform);

			if (hism == grid->HISMLava || hism == grid->HISMRiver)
				continue;

			FWetnessStruct wetnessStruct;
			wetnessStruct.HISM = hism;
			wetnessStruct.Instance = i;

			int32 index = clouds->WetnessStruct.Find(wetnessStruct);

			if (index == INDEX_NONE)
				continue;

			FWetnessData wetnessData;
			wetnessData.Location = transform.GetLocation();
			wetnessData.Value = clouds->WetnessStruct[index].Value;
			wetnessData.Increment = clouds->WetnessStruct[index].Increment;

			worldSaveData.CloudsData.WetnessData.Add(wetnessData);
		}

		data.CustomDataValues = hism->PerInstanceSMCustomData;

		worldSaveData.HISMData.Add(data);
	}

	worldSaveData.AtmosphereData.Calendar = grid->AtmosphereComponent->Calendar;
	worldSaveData.AtmosphereData.bRedSun = grid->AtmosphereComponent->bRedSun;
	worldSaveData.AtmosphereData.WindRotation = grid->AtmosphereComponent->WindRotation;
	worldSaveData.AtmosphereData.SunRotation = grid->AtmosphereComponent->Sun->GetRelativeRotation();
	worldSaveData.AtmosphereData.MoonRotation = grid->AtmosphereComponent->Moon->GetRelativeRotation();

	worldSaveData.NaturalDisasterData.DisasterChance = grid->AtmosphereComponent->NaturalDisasterComponent->DisasterChance;
	worldSaveData.NaturalDisasterData.Frequency = grid->AtmosphereComponent->NaturalDisasterComponent->Frequency;
	worldSaveData.NaturalDisasterData.Intensity = grid->AtmosphereComponent->NaturalDisasterComponent->Intensity;

	for (AActor* a : PotentialWetActors) {
		FWetnessStruct wetnessStruct;
		wetnessStruct.Actor = a;

		int32 index = clouds->WetnessStruct.Find(wetnessStruct);

		if (index == INDEX_NONE)
			continue;

		FWetnessData wetnessData;
		wetnessData.Location = a->GetActorLocation();
		wetnessData.Value = clouds->WetnessStruct[index].Value;
		wetnessData.Increment = clouds->WetnessStruct[index].Increment;

		worldSaveData.CloudsData.WetnessData.Add(wetnessData);
	}

	for (FCloudStruct cloud : grid->AtmosphereComponent->Clouds->Clouds) {
		FCloudData data;
		data.Transform = cloud.HISMCloud->GetRelativeTransform();
		data.Distance = cloud.Distance;
		data.bPrecipitation = cloud.Precipitation != nullptr;
		data.bHide = cloud.bHide;
		data.lightningFrequency = cloud.lightningFrequency;
		data.lightningTimer = cloud.lightningTimer;
		data.Opacity = cloud.HISMCloud->GetCustomPrimitiveData().Data[0];

		data.HISMData.Name = cloud.HISMCloud->GetName();
		for (int32 i = 0; i < cloud.HISMCloud->GetInstanceCount(); i++) {
			FTransform transform;
			cloud.HISMCloud->GetInstanceTransform(i, transform);

			data.HISMData.Transforms.Add(transform);
		}

		worldSaveData.CloudsData.CloudData.Add(data);
	}

	Saves[Index].WorldData = worldSaveData;
}

void UDiplosimSaveGame::SaveResource(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	AResource* resource = Cast<AResource>(Actor);
	FResourceData resourceData;

	for (int32 i = 0; i < resource->ResourceHISM->GetInstanceCount(); i++) {
		FTransform transform;
		resource->ResourceHISM->GetInstanceTransform(i, transform);

		resourceData.HISMData.Transforms.Add(transform);

		if (Camera->Grid->AtmosphereComponent->GetFireComponent(Actor, i) != nullptr)
			resourceData.FireInstances.Add(i);
	}

	resourceData.HISMData.CustomDataValues = resource->ResourceHISM->PerInstanceSMCustomData;

	int32 workerCount = 0;
	for (FWorkerStruct worker : resource->WorkerStruct) {
		FWorkerData workerData;
		workerData.Instance = worker.Instance;

		for (ACitizen* citizen : worker.Citizens)
			workerData.CitizenNames.Add(citizen->GetName());

		resourceData.WorkersData.Add(workerData);
	}

	ActorData.dataIndex = Saves[Index].ResourceData.Add(resourceData);
}

void UDiplosimSaveGame::SaveCamera(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);
	Saves[Index].CameraData.ColonyName = camera->ColonyName;

	for (FConstructionStruct constructionStruct : camera->ConstructionManager->Construction) {
		FConstructionData data;
		data.BuildingName = constructionStruct.Building->GetName();
		data.BuildPercentage = constructionStruct.BuildPercentage;
		data.Status = constructionStruct.Status;

		if (IsValid(constructionStruct.Builder))
			data.BuilderName = constructionStruct.Builder->GetName();

		Saves[Index].CameraData.ConstructionData.Add(data);
	}
}

void UDiplosimSaveGame::SaveCitizenManager(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);

	FCitizenManagerData cmData;

	for (FPersonality personality : camera->CitizenManager->Personalities) {
		FPersonalityData data;
		data.Trait = personality.Trait;

		for (ACitizen* citizen : personality.Citizens)
			data.CitizenNames.Add(citizen->GetName());

		cmData.PersonalitiesData.Add(data);
	}

	for (ACitizen* citizen : camera->DiseaseManager->Infectible)
		cmData.InfectibleNames.Add(citizen->GetName());

	for (ACitizen* citizen : camera->DiseaseManager->Infected)
		cmData.InfectedNames.Add(citizen->GetName());

	for (ACitizen* citizen : camera->DiseaseManager->Injured)
		cmData.InjuredNames.Add(citizen->GetName());

	cmData.IssuePensionHour = camera->CitizenManager->IssuePensionHour;

	Saves[Index].CameraData.CitizenManagerData = cmData;
}

void UDiplosimSaveGame::SaveFactions(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);

	for (FFactionStruct faction : camera->ConquestManager->Factions) {
		FFactionData data;
		data.Name = faction.Name;
		data.FlagColour = faction.FlagColour;
		data.AtWar = faction.AtWar;
		data.Allies = faction.Allies;
		data.Happiness = faction.Happiness;
		data.WarFatigue = faction.WarFatigue;
		data.PartyInPower = faction.PartyInPower;
		data.LargestReligion = faction.LargestReligion;
		data.RebelCooldownTimer = faction.RebelCooldownTimer;
		data.ResearchStruct = faction.ResearchStruct;
		data.ResearchIndices = faction.ResearchIndices;
		data.PrayStruct = faction.PrayStruct;
		data.Resources = faction.Resources;
		data.AccessibleBuildLocations = faction.AccessibleBuildLocations;
		data.InaccessibleBuildLocations = faction.InaccessibleBuildLocations;
		data.RoadBuildLocations = faction.RoadBuildLocations;
		data.FailedBuild = faction.FailedBuild;

		data.BuildingClassAmount.Empty();
		for (auto& element : faction.BuildingClassAmount)
			data.BuildingClassAmount.Add(element.Value);

		// Politics
		FPoliticsData politicsData;
		for (FPartyStruct party : faction.Politics.Parties) {
			FPartyData partyData;
			partyData.Party = party.Party;
			partyData.Agreeable = party.Agreeable;

			for (auto element : party.Members)
				partyData.MembersName.Add(element.Key->GetName(), element.Value);

			if (IsValid(party.Leader))
				partyData.LeaderName = party.Leader->GetName();

			politicsData.PartiesData.Add(partyData);
		}

		for (ACitizen* citizen : faction.Politics.Representatives)
			if (IsValid(citizen))
				politicsData.RepresentativesNames.Add(citizen->GetName());

		for (ACitizen* citizen : faction.Politics.Votes.For)
			politicsData.VotesData.ForNames.Add(citizen->GetName());

		for (ACitizen* citizen : faction.Politics.Votes.Against)
			politicsData.VotesData.AgainstNames.Add(citizen->GetName());

		for (ACitizen* citizen : faction.Politics.Predictions.For)
			politicsData.PredictionsData.ForNames.Add(citizen->GetName());

		for (ACitizen* citizen : faction.Politics.Predictions.Against)
			politicsData.PredictionsData.AgainstNames.Add(citizen->GetName());

		politicsData.BribeValue = faction.Politics.BribeValue;
		politicsData.Laws = faction.Politics.Laws;
		politicsData.ProposedBills = faction.Politics.ProposedBills;

		data.PoliticsData = politicsData;

		// Police
		FPoliceData policeData;
		for (auto element : faction.Police.Arrested)
			policeData.ArrestedNames.Add(element.Key->GetName(), element.Value);

		for (FPoliceReport report : faction.Police.PoliceReports) {
			FPoliceReportData reportData;
			reportData.Type = report.Type;
			reportData.Location = report.Location;

			if (IsValid(report.Team1.Instigator))
				reportData.Team1.InstigatorName = report.Team1.Instigator->GetName();

			for (ACitizen* citizen : report.Team1.Assistors)
				reportData.Team1.AssistorsNames.Add(citizen->GetName());

			if (IsValid(report.Team2.Instigator))
				reportData.Team2.InstigatorName = report.Team2.Instigator->GetName();

			for (ACitizen* citizen : report.Team2.Assistors)
				reportData.Team2.AssistorsNames.Add(citizen->GetName());

			for (auto element : report.Witnesses)
				reportData.WitnessesNames.Add(element.Key->GetName(), element.Value);

			if (IsValid(report.RespondingOfficer))
				reportData.RespondingOfficerName = report.RespondingOfficer->GetName();

			for (ACitizen* citizen : report.AcussesTeam1)
				reportData.AcussesTeam1Names.Add(citizen->GetName());

			for (ACitizen* citizen : report.Impartial)
				reportData.ImpartialNames.Add(citizen->GetName());

			for (ACitizen* citizen : report.AcussesTeam2)
				reportData.AcussesTeam2Names.Add(citizen->GetName());

			policeData.PoliceReportsData.Add(reportData);
		}

		data.PoliceData = policeData;

		// Events
		data.EventsData.Empty();
		for (FEventStruct event : faction.Events) {
			FEventData eventData;
			eventData.Type = event.Type;
			eventData.Period = event.Period;
			eventData.Day = event.Day;
			eventData.Hours = event.Hours;
			eventData.bRecurring = event.bRecurring;
			eventData.bFireFestival = event.bFireFestival;
			eventData.bStarted = event.bStarted;
			eventData.Building = event.Building;
			eventData.Location = event.Location;

			if (IsValid(event.Venue))
				eventData.VenueName = event.Venue->GetName();

			for (ACitizen* citizen : event.Whitelist)
				eventData.WhitelistNames.Add(citizen->GetName());

			for (ACitizen* citizen : event.Attendees)
				eventData.AttendeesNames.Add(citizen->GetName());

			data.EventsData.Add(eventData);
		}

		// Army
		data.ArmiesData.Empty();
		for (FArmyStruct army : faction.Armies) {
			FArmyData armyData;
			armyData.bGroup = army.bGroup;

			for (ACitizen* citizen : army.Citizens)
				armyData.CitizensNames.Add(citizen->GetName());

			data.ArmiesData.Add(armyData);
		}

		Saves[Index].CameraData.FactionData.Add(data);
	}
}

void UDiplosimSaveGame::SaveGamemode(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);

	ADiplosimGameModeBase* gamemode = camera->GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
	FGamemodeData* gamemodeData = &Saves[Index].CameraData.GamemodeData;

	gamemodeData->WaveData.Empty();
	for (FWaveStruct wave : gamemode->WavesData) {
		FWaveData waveData;
		waveData.SpawnLocations = wave.SpawnLocations;

		for (FDiedToStruct diedTo : wave.DiedTo)
			waveData.DiedTo.Add(diedTo.Actor->GetName(), diedTo.Kills);

		for (TWeakObjectPtr<AActor> threat : wave.Threats)
			waveData.Threats.Add(threat->GetName());

		waveData.NumKilled = wave.NumKilled;
		waveData.EnemiesData = wave.EnemiesData;

		gamemodeData->WaveData.Add(waveData);
	}

	for (AAI* enemy : gamemode->Enemies)
		gamemodeData->EnemyNames.Add(enemy->GetName());

	for (AAI* snake : gamemode->Snakes)
		gamemodeData->SnakeNames.Add(snake->GetName());

	gamemodeData->bOngoingRaid = gamemode->bOngoingRaid;
	gamemodeData->CrystalOpacity = camera->Grid->CrystalMesh->GetCustomPrimitiveData().Data[0];
	gamemodeData->TargetOpacity = gamemode->TargetOpacity;
}

void UDiplosimSaveGame::SaveAI(ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, AActor* Actor, int32 Index)
{
	AAI* ai = Cast<AAI>(Actor);

	ADiplosimGameModeBase* gamemode = Camera->GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", ai);

	AIData.Colour = ai->Colour;

	if (ai->MovementComponent->bSetPoints)
		AIData.MovementData.Points = ai->MovementComponent->TempPoints;
	else
		AIData.MovementData.Points = ai->MovementComponent->Points;

	AIData.MovementData.CurrentAnim = ai->MovementComponent->CurrentAnim;
	AIData.MovementData.LastUpdatedTime = ai->MovementComponent->LastUpdatedTime;
	AIData.MovementData.Transform = ai->MovementComponent->Transform;

	if (IsValid(ai->MovementComponent->ActorToLookAt))
		AIData.MovementData.ActorToLookAtName = ai->MovementComponent->ActorToLookAt->GetName();

	if (IsValid(ai->AIController->ChosenBuilding))
		AIData.MovementData.ChosenBuildingName = ai->AIController->ChosenBuilding->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetGoalActor()))
		AIData.MovementData.ActorName = ai->AIController->MoveRequest.GetGoalActor()->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetLinkedPortal()))
		AIData.MovementData.LinkedPortalName = ai->AIController->MoveRequest.GetLinkedPortal()->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetUltimateGoalActor()))
		AIData.MovementData.UltimateGoalName = ai->AIController->MoveRequest.GetUltimateGoalActor()->GetName();

	AIData.MovementData.Instance = ai->AIController->MoveRequest.GetGoalInstance();
	AIData.MovementData.Location = ai->AIController->MoveRequest.GetLocation();

	if (gamemode->Snakes.Contains(ai))
		AIData.bSnake = true;

	if (faction != nullptr)
		AIData.FactionName = faction->Name;
}

void UDiplosimSaveGame::SaveCitizen(ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, AActor* Actor, int32 Index)
{
	ACitizen* citizen = Cast<ACitizen>(Actor);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

	if (faction != nullptr) {
		if (faction->Rebels.Contains(citizen))
			AIData.CitizenData.bRebel = true;

		AIData.CitizenData.EnterLocation = citizen->BuildingComponent->EnterLocation;
	}

	if (IsValid(citizen->Carrying.Type)) {
		AIData.CitizenData.ResourceCarryClass = citizen->Carrying.Type->GetClass();
		AIData.CitizenData.CarryAmount = citizen->Carrying.Amount;
	}

	if (citizen->BioComponent->Mother != nullptr)
		AIData.CitizenData.MothersName = citizen->BioComponent->Mother->GetName();

	if (citizen->BioComponent->Father != nullptr)
		AIData.CitizenData.FathersName = citizen->BioComponent->Father->GetName();

	if (citizen->BioComponent->Partner != nullptr)
		AIData.CitizenData.PartnersName = citizen->BioComponent->Partner->GetName();

	for (ACitizen* child : citizen->BioComponent->Children)
		AIData.CitizenData.ChildrensNames.Add(child->GetName());

	for (ACitizen* sibling : citizen->BioComponent->Siblings)
		AIData.CitizenData.SiblingsNames.Add(sibling->GetName());

	AIData.CitizenData.HoursTogetherWithPartner = citizen->BioComponent->HoursTogetherWithPartner;
	AIData.CitizenData.bMarried = citizen->BioComponent->bMarried;
	AIData.CitizenData.Sex = citizen->BioComponent->Sex;
	AIData.CitizenData.Sexuality = citizen->BioComponent->Sexuality;
	AIData.CitizenData.Age = citizen->BioComponent->Age;
	AIData.CitizenData.Name = citizen->BioComponent->Name;
	AIData.CitizenData.EducationLevel = citizen->BioComponent->EducationLevel;
	AIData.CitizenData.EducationProgress = citizen->BioComponent->EducationProgress;
	AIData.CitizenData.PaidForEducationLevel = citizen->BioComponent->PaidForEducationLevel;
	AIData.CitizenData.bAdopted = citizen->BioComponent->bAdopted;
	AIData.CitizenData.SpeedBeforeOld = citizen->BioComponent->SpeedBeforeOld;
	AIData.CitizenData.MaxHealthBeforeOld = citizen->BioComponent->MaxHealthBeforeOld;

	AIData.CitizenData.Spirituality = citizen->Spirituality;
	AIData.CitizenData.TimeOfAcquirement = citizen->BuildingComponent->TimeOfAcquirement;
	AIData.CitizenData.VoicePitch = citizen->VoicePitch;
	AIData.CitizenData.Balance = citizen->Balance;
	AIData.CitizenData.HoursWorked = citizen->BuildingComponent->HoursWorked;
	AIData.CitizenData.Hunger = citizen->Hunger;
	AIData.CitizenData.Energy = citizen->Energy;
	AIData.CitizenData.bGain = citizen->bGain;
	AIData.CitizenData.bHasBeenLeader = citizen->bHasBeenLeader;
	AIData.CitizenData.HealthIssues = citizen->HealthIssues;
	AIData.CitizenData.Modifiers = citizen->HappinessComponent->Modifiers;
	AIData.CitizenData.SadTimer = citizen->HappinessComponent->SadTimer;
	AIData.CitizenData.MassStatus = citizen->HappinessComponent->MassStatus;
	AIData.CitizenData.FestivalStatus = citizen->HappinessComponent->FestivalStatus;
	AIData.CitizenData.bConversing = citizen->bConversing;
	AIData.CitizenData.DecayingHappiness = citizen->HappinessComponent->DecayingHappiness;
	AIData.CitizenData.Genetics = citizen->Genetics;
	AIData.CitizenData.bSleep = citizen->bSleep;
	AIData.CitizenData.HoursSleptToday = citizen->HoursSleptToday;

	AIData.MovementData.Transform.SetScale3D(FVector(1.0f));

	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
		AIData.CitizenData.PersonalityTraits.Add(personality->Trait);
}

void UDiplosimSaveGame::SaveBuilding(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ABuilding* building = Cast<ABuilding>(Actor);
	FBuildingData buildingData;

	if (Camera->Grid->AtmosphereComponent->GetFireComponent(Actor) != nullptr)
		buildingData.bOnFire = true;

	buildingData.FactionName = building->FactionName;
	buildingData.Capacity = building->Capacity;

	buildingData.Seed = building->SeedNum;
	buildingData.ChosenColour = building->ChosenColour;
	buildingData.Tier = building->Tier;

	buildingData.TargetList = building->TargetList;

	buildingData.Storage = building->Storage;
	buildingData.Basket = building->Basket;

	double* time = building->Camera->Grid->AIVisualiser->DestructingActors.Find(building);
	if (time != nullptr)
		buildingData.DeathTime = *time;

	buildingData.bOperate = building->bOperate;

	TArray<FCapacityData> occData;

	for (FCapacityStruct capacityStruct : building->Occupied) {
		FCapacityData capData;

		if (IsValid(capacityStruct.Citizen)) {
			capData.CitizenName = capacityStruct.Citizen->GetName();

			for (ACitizen* visitor : capacityStruct.Visitors)
				capData.VisitorNames.Add(visitor->GetName());
		}

		capData.Amount = capacityStruct.Amount;
		capData.WorkHours = capacityStruct.WorkHours;
		capData.bBlocked = capacityStruct.bBlocked;

		occData.Add(capData);
	}

	buildingData.OccupiedData = occData;

	ActorData.dataIndex = Saves[Index].BuildingData.Add(buildingData);
}

void UDiplosimSaveGame::SaveProjectile(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);
	FProjectileData projectileData;

	projectileData.OwnerName = projectile->GetOwner()->GetName();
	projectileData.Velocity = projectile->ProjectileMovementComponent->Velocity;

	ActorData.dataIndex = Saves[Index].ProjectileData.Add(projectileData);
}

void UDiplosimSaveGame::SaveAISpawner(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);
	FSpawnerData spawnerData;

	spawnerData.Colour = spawner->Colour;
	spawnerData.IncrementSpawned = spawner->IncrementSpawned;

	ActorData.dataIndex = Saves[Index].SpawnerData.Add(spawnerData);
}

void UDiplosimSaveGame::SaveTimers(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	for (FTimerStruct timer : Camera->TimerManager->Timers) {
		if (timer.Actor != Actor)
			continue;

		for (FTimerParameterStruct& param : timer.Parameters) {
			if (!IsValid(param.Object))
				continue;

			param.ObjectName = param.Object->GetName();
			param.Object = nullptr;
		}

		timer.Actor = nullptr;
		ActorData.SavedTimers.Add(timer);
	}
}

void UDiplosimSaveGame::SaveComponents(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	FHealthData healthData;
	FAttackData attackData;

	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp)
		healthData.Health = healthComp->GetHealth();

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		if (*attackComp->ProjectileClass)
			attackData.ProjectileClass = attackComp->ProjectileClass;

		attackData.AttackTimer = attackComp->AttackTimer;
		attackData.bShowMercy = attackComp->bShowMercy;

		for (AActor* enemy : attackComp->OverlappingEnemies)
			attackData.ActorNames.Add(enemy->GetName());
	}

	if (Actor->IsA<ABuilding>()) {
		FBuildingData& buildingData = Saves[Index].BuildingData[ActorData.dataIndex];
		buildingData.HealthData = healthData;
		buildingData.AttackData = attackData;
	}
	else if (Actor->IsA<AAI>()) {
		FAIData& aiData = Saves[Index].AIData[ActorData.dataIndex];
		aiData.HealthData = healthData;
		aiData.AttackData = attackData;
	}
}

//
// Loading
//
void UDiplosimSaveGame::LoadGame(ACamera* Camera, int32 Index)
{
	ADiplosimGameModeBase* gamemode = Camera->GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
	gamemode->Enemies.Empty();
	gamemode->SnakeSpawners.Empty();
	gamemode->Snakes.Empty();

	TArray<AActor*> foundActors;
	TArray<UClass*> classes = { AAI::StaticClass(), AEggBasket::StaticClass(), ABuilding::StaticClass(), AAISpawner::StaticClass(), AProjectile::StaticClass() };

	for (UClass* clss : classes) {
		UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), clss, foundActors);

		for (AActor* a : foundActors)
			a->Destroy();
	}

	TMap<FString, FActorSaveData*> aiToName; 
	TArray<FWetnessData> wetnessData;

	for (FActorSaveData& actorData : Saves[Index].SavedActors) {
		AActor* actor = nullptr;

		UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), actorData.Class, foundActors);

		if (foundActors.Num() == 1 && (foundActors[0]->IsA<ACamera>() || foundActors[0]->IsA<AGrid>() || foundActors[0]->IsA<AResource>())) {
			actor = foundActors[0];

			if (actorData.Transform.GetLocation() != actor->GetActorLocation())
				actor->SetActorTransform(actorData.Transform);
		}
		else {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = Camera->GetWorld()->SpawnActor<AActor>(actorData.Class, actorData.Transform, params);
		}

		if (actor->IsA<AGrid>())
			LoadWorld(Saves[Index].WorldData, actor, wetnessData);
		else if (actor->IsA<AResource>())
			LoadResource(Camera, Saves[Index].ResourceData[actorData.dataIndex], actor);
		else if (actor->IsA<AEggBasket>())
			LoadEggBasket(Camera, actorData, actor);
		else if (actor->IsA<ACamera>()) {
			LoadCamera(actorData, Saves[Index].CameraData, actor);

			LoadFactions(actorData, Saves[Index].CameraData, actor);

			LoadGamemode(gamemode, actorData, Saves[Index].CameraData.GamemodeData, actor);
		}
		else if (actor->IsA<AAI>()) {
			LoadAI(Camera, gamemode, actorData, Saves[Index].AIData[actorData.dataIndex], Saves[Index].CameraData.GamemodeData, actor, aiToName);

			if (actor->IsA<ACitizen>())
				LoadCitizen(Camera, actorData, Saves[Index].AIData[actorData.dataIndex], Saves[Index].CameraData, actor);
		}
		else if (actor->IsA<ABuilding>())
			LoadBuilding(Camera, actorData, Saves[Index].BuildingData[actorData.dataIndex], Saves[Index].CameraData.ConstructionData, actor, aiToName, Index);
		else if (actor->IsA<AProjectile>())
			LoadProjectile(Saves[Index].ProjectileData[actorData.dataIndex], actor);
		else if (actor->IsA<AAISpawner>())
			LoadAISpawner(Saves[Index].SpawnerData[actorData.dataIndex], actor);

		LoadComponents(actorData, actor, Index);

		actorData.Actor = actor;
	}

	for (FActorSaveData actorData : Saves[Index].SavedActors) {
		LoadTimers(Camera, Index, actorData, Saves[Index].SavedActors);

		for (FActorSaveData savedData : Saves[Index].SavedActors) {
			LoadOverlappingEnemies(Camera, actorData, actorData.Actor, savedData, Index);

			InitialiseObjects(Camera, gamemode, actorData, savedData, Index);
		}
	}

	for (FWetnessData wetData : wetnessData)
		Camera->Grid->AtmosphereComponent->Clouds->RainCollisionHandler(wetData.Location, wetData.Value, wetData.Increment);
}

void UDiplosimSaveGame::LoadWorld(FWorldSaveData WorldData, AActor* Actor, TArray<FWetnessData>& WetnessData)
{
	AGrid* grid = Cast<AGrid>(Actor);

	grid->Size = WorldData.Size;
	grid->Chunks = WorldData.Chunks;

	grid->Camera->Stream = WorldData.Stream;

	grid->LavaSpawnLocations = WorldData.LavaSpawnLocations;

	grid->Clear();

	grid->SetMapBounds();
	grid->InitialiseStorage();
	auto bound = grid->GetMapBounds();

	for (FTileData t : WorldData.Tiles) {
		FTileStruct* tile = &grid->Storage[t.X + (bound / 2)][t.Y + (bound / 2)];
		tile->Level = t.Level;
		tile->Fertility = t.Fertility;
		tile->X = t.X;
		tile->Y = t.Y;
		tile->Rotation = t.Rotation;
		tile->bRamp = t.bRamp;
		tile->bRiver = t.bRiver;
		tile->bEdge = t.bEdge;
		tile->bMineral = t.bMineral;
		tile->bUnique = t.bUnique;
	}

	TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea };

	for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
		FHISMData data;
		data.Name = hism->GetName();

		int32 index = WorldData.HISMData.Find(data);
		TArray<FTransform> transforms = WorldData.HISMData[index].Transforms;

		hism->AddInstances(transforms, false, true, true);
		hism->PerInstanceSMCustomData = WorldData.HISMData[index].CustomDataValues;
		hism->BuildTreeIfOutdated(true, true);

		hism->PartialNavigationUpdates(transforms);
	}

	grid->AtmosphereComponent->Calendar = WorldData.AtmosphereData.Calendar;
	grid->AtmosphereComponent->bRedSun = WorldData.AtmosphereData.bRedSun;
	grid->AtmosphereComponent->WindRotation = WorldData.AtmosphereData.WindRotation;
	grid->AtmosphereComponent->Sun->SetRelativeRotation(WorldData.AtmosphereData.SunRotation);
	grid->AtmosphereComponent->Moon->SetRelativeRotation(WorldData.AtmosphereData.MoonRotation);

	grid->AtmosphereComponent->NaturalDisasterComponent->DisasterChance = WorldData.NaturalDisasterData.DisasterChance;
	grid->AtmosphereComponent->NaturalDisasterComponent->Frequency = WorldData.NaturalDisasterData.Frequency;
	grid->AtmosphereComponent->NaturalDisasterComponent->Intensity = WorldData.NaturalDisasterData.Intensity;

	if (grid->AtmosphereComponent->bRedSun)
		grid->AtmosphereComponent->NaturalDisasterComponent->AlterSunGradually(0.15f, -1.00f);

	grid->AtmosphereComponent->Clouds->ProcessRainEffect.Empty();
	grid->AtmosphereComponent->Clouds->WetnessStruct.Empty();
	WetnessData = WorldData.CloudsData.WetnessData;

	for (FCloudData data : WorldData.CloudsData.CloudData) {
		FCloudStruct cloudStruct = grid->AtmosphereComponent->Clouds->CreateCloud(data.Transform, data.bPrecipitation ? 100 : 0, true, data.HISMData.Transforms);
		cloudStruct.Distance = data.Distance;
		cloudStruct.bHide = data.bHide;
		cloudStruct.lightningFrequency = data.lightningFrequency;
		cloudStruct.lightningTimer = data.lightningTimer;

		cloudStruct.HISMCloud->SetCustomPrimitiveDataFloat(0, data.Opacity);
		
		grid->AtmosphereComponent->Clouds->Clouds.Add(cloudStruct);
	}

	grid->SetupEnvironment(true);
	grid->AIVisualiser->ResetToDefaultValues();
}

void UDiplosimSaveGame::LoadResource(ACamera* Camera, FResourceData& ResourceData, AActor* Actor)
{
	AResource* resource = Cast<AResource>(Actor);

	resource->ResourceHISM->AddInstances(ResourceData.HISMData.Transforms, false, true, true);
	resource->ResourceHISM->PerInstanceSMCustomData = ResourceData.HISMData.CustomDataValues;
	resource->ResourceHISM->SetCustomDataValue(0, 0, ResourceData.HISMData.CustomDataValues[0]);

	for (FWorkerData workerData : ResourceData.WorkersData) {
		FWorkerStruct workerStruct;
		workerStruct.Instance = workerData.Instance;

		resource->WorkerStruct.Add(workerStruct);
	}

	for (int32 instance : ResourceData.FireInstances)
		Camera->Grid->AtmosphereComponent->SetOnFire(Actor, instance, true);
}

void UDiplosimSaveGame::LoadEggBasket(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	AEggBasket* eggBasket = Cast<AEggBasket>(Actor);

	FTileStruct* tile = Camera->Grid->GetTileFromLocation(ActorData.Transform.GetLocation());

	eggBasket->Grid = Camera->Grid;
	eggBasket->Tile = tile;

	Camera->Grid->ResourceTiles.RemoveSingle(tile);
}

void UDiplosimSaveGame::LoadCamera(FActorSaveData& ActorData, FCameraData& CameraData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);
	camera->ColonyName = CameraData.ColonyName;

	camera->Start = false;
	camera->Cancel();

	camera->ClearPopupUI();
	camera->SetInteractStatus(camera->WidgetComponent->GetAttachmentRootActor(), false);

	camera->Detach();
	camera->MovementComponent->TargetLength = 3000.0f;
	camera->MovementComponent->MovementLocation = ActorData.Transform.GetLocation();

	camera->ConstructionManager->Construction.Empty();

	camera->CitizenManager->IssuePensionHour = CameraData.CitizenManagerData.IssuePensionHour;
}

void UDiplosimSaveGame::LoadFactions(FActorSaveData& ActorData, FCameraData& CameraData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	camera->ConquestManager->Factions.Empty();
	for (FFactionData data : CameraData.FactionData) {
		FFactionStruct faction;
		faction.Name = data.Name;
		faction.FlagColour = data.FlagColour;
		faction.AtWar = data.AtWar;
		faction.Allies = data.Allies;
		faction.Happiness = data.Happiness;
		faction.WarFatigue = data.WarFatigue;
		faction.PartyInPower = data.PartyInPower;
		faction.LargestReligion = data.LargestReligion;
		faction.RebelCooldownTimer = data.RebelCooldownTimer;
		faction.ResearchStruct = data.ResearchStruct;
		faction.ResearchIndices = data.ResearchIndices;
		faction.PrayStruct = data.PrayStruct;
		faction.Resources = data.Resources;
		faction.AccessibleBuildLocations = data.AccessibleBuildLocations;
		faction.InaccessibleBuildLocations = data.InaccessibleBuildLocations;
		faction.RoadBuildLocations = data.RoadBuildLocations;
		faction.FailedBuild = data.FailedBuild;

		for (int32 i = 0; i < camera->ConquestManager->BuildingClassDefaultAmount.Num(); i++)
			faction.BuildingClassAmount.Add(camera->ConquestManager->BuildingClassDefaultAmount[i], data.BuildingClassAmount[i]);
		
		// Politics
		for (FPartyData partyData : data.PoliticsData.PartiesData) {
			FPartyStruct party;

			party.Party = partyData.Party;
			party.Agreeable = partyData.Agreeable;

			faction.Politics.Parties.Add(party);
		}

		faction.Politics.BribeValue = data.PoliticsData.BribeValue;
		faction.Politics.Laws = data.PoliticsData.Laws;
		faction.Politics.ProposedBills = data.PoliticsData.ProposedBills;

		// Police
		for (FPoliceReportData reportData : data.PoliceData.PoliceReportsData) {
			FPoliceReport report;
			report.Type = reportData.Type;
			report.Location = reportData.Location;

			faction.Police.PoliceReports.Add(report);
		}

		// Events
		for (FEventData eventData : data.EventsData) {
			FEventStruct event;
			event.Type = eventData.Type;
			event.Period = eventData.Period;
			event.Day = eventData.Day;
			event.Hours = eventData.Hours;
			event.bRecurring = eventData.bRecurring;
			event.bFireFestival = eventData.bFireFestival;
			event.bStarted = eventData.bStarted;
			event.Building = eventData.Building;
			event.Location = eventData.Location;

			faction.Events.Add(event);
		}

		// Army
		for (FArmyData armyData : data.ArmiesData)
			camera->ArmyManager->CreateArmy(data.Name, {}, armyData.bGroup, true);

		camera->ConquestManager->Factions.Add(faction);
	}
}

void UDiplosimSaveGame::LoadGamemode(ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FGamemodeData& GamemodeData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	for (FWaveData waveData : GamemodeData.WaveData) {
		FWaveStruct wave;
		wave.SpawnLocations = waveData.SpawnLocations;

		for (auto element : waveData.DiedTo) {
			FDiedToStruct diedTo;
			diedTo.Kills = element.Value;

			wave.DiedTo.Add(diedTo);
		}

		wave.NumKilled = waveData.NumKilled;
		wave.EnemiesData = waveData.EnemiesData;

		Gamemode->WavesData.Add(wave);
	}

	Gamemode->bOngoingRaid = GamemodeData.bOngoingRaid;
	camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, GamemodeData.CrystalOpacity);
	Gamemode->TargetOpacity = GamemodeData.TargetOpacity;

	if (GamemodeData.CrystalOpacity != Gamemode->TargetOpacity)
		Gamemode->SetActorTickEnabled(true);
}

void UDiplosimSaveGame::LoadAI(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FAIData& AIData, FGamemodeData& GamemodeData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName)
{
	AAI* ai = Cast<AAI>(Actor);
	AIToName.Add(ActorData.Name, &ActorData);

	UInstancedStaticMeshComponent* ism;

	if (AIData.FactionName == "") {
		if (AIData.bSnake)
			ism = Camera->Grid->AIVisualiser->HISMSnake;
		else
			ism = Camera->Grid->AIVisualiser->HISMEnemy;
	}
	else {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(AIData.FactionName);

		if (ai->IsA<AClone>()) {
			faction->Clones.Add(ai);
			ism = Camera->Grid->AIVisualiser->HISMClone;
		}
		else if (AIData.CitizenData.bRebel) {
			faction->Rebels.Add(Cast<ACitizen>(ai));
			ism = Camera->Grid->AIVisualiser->HISMRebel;
		}
		else {
			faction->Citizens.Add(Cast<ACitizen>(ai));
			ism = Camera->Grid->AIVisualiser->HISMCitizen;
		}
	}

	ai->Colour = AIData.Colour;

	ai->MovementComponent->Points = AIData.MovementData.Points;
	ai->MovementComponent->CurrentAnim = AIData.MovementData.CurrentAnim;
	ai->MovementComponent->LastUpdatedTime = AIData.MovementData.LastUpdatedTime;

	Camera->Grid->AIVisualiser->AddInstance(ai, ism, AIData.MovementData.Transform);

	if (!ai->IsA<AEnemy>())
		return; 

	if (GamemodeData.EnemyNames.Contains(ActorData.Name))
		Gamemode->Enemies.Add(ai);

	if (GamemodeData.SnakeNames.Contains(ActorData.Name))
		Gamemode->Snakes.Add(ai);
}

void UDiplosimSaveGame::LoadCitizen(ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FCameraData& CameraData, AActor* Actor)
{
	ACitizen* citizen = Cast<ACitizen>(Actor);

	if (IsValid(AIData.CitizenData.ResourceCarryClass)) {
		citizen->Carrying.Type = Cast<AResource>(AIData.CitizenData.ResourceCarryClass->GetDefaultObject());
		citizen->Carrying.Amount = AIData.CitizenData.CarryAmount;
	}

	citizen->BioComponent->HoursTogetherWithPartner = AIData.CitizenData.HoursTogetherWithPartner;
	citizen->BioComponent->bMarried = AIData.CitizenData.bMarried;
	citizen->BioComponent->Sex = AIData.CitizenData.Sex;
	citizen->BioComponent->Sexuality = AIData.CitizenData.Sexuality;
	citizen->BioComponent->Age = AIData.CitizenData.Age;
	citizen->BioComponent->Name = AIData.CitizenData.Name;
	citizen->BioComponent->EducationLevel = AIData.CitizenData.EducationLevel;
	citizen->BioComponent->EducationProgress = AIData.CitizenData.EducationProgress;
	citizen->BioComponent->PaidForEducationLevel = AIData.CitizenData.PaidForEducationLevel;
	citizen->BioComponent->bAdopted = AIData.CitizenData.bAdopted;
	citizen->BioComponent->SpeedBeforeOld = AIData.CitizenData.SpeedBeforeOld;
	citizen->BioComponent->MaxHealthBeforeOld = AIData.CitizenData.MaxHealthBeforeOld;

	citizen->Spirituality = AIData.CitizenData.Spirituality;
	citizen->BuildingComponent->TimeOfAcquirement = AIData.CitizenData.TimeOfAcquirement;
	citizen->VoicePitch = AIData.CitizenData.VoicePitch;
	citizen->Balance = AIData.CitizenData.Balance;
	citizen->BuildingComponent->HoursWorked = AIData.CitizenData.HoursWorked;
	citizen->Hunger = AIData.CitizenData.Hunger;
	citizen->Energy = AIData.CitizenData.Energy;
	citizen->bGain = AIData.CitizenData.bGain;
	citizen->bHasBeenLeader = AIData.CitizenData.bHasBeenLeader;
	citizen->HealthIssues = AIData.CitizenData.HealthIssues;
	citizen->HappinessComponent->Modifiers = AIData.CitizenData.Modifiers;
	citizen->HappinessComponent->SadTimer = AIData.CitizenData.SadTimer;
	citizen->HappinessComponent->MassStatus = AIData.CitizenData.MassStatus;
	citizen->HappinessComponent->FestivalStatus = AIData.CitizenData.FestivalStatus;
	citizen->HappinessComponent->DecayingHappiness = AIData.CitizenData.DecayingHappiness;
	citizen->Genetics = AIData.CitizenData.Genetics;
	citizen->bSleep = AIData.CitizenData.bSleep;
	citizen->HoursSleptToday = AIData.CitizenData.HoursSleptToday;

	if (AIData.CitizenData.bConversing)
		Camera->PlayAmbientSound(citizen->AmbientAudioComponent, Camera->CitizenManager->GetConversationSound(citizen), citizen->VoicePitch);

	for (FGeneticsStruct& genetic : citizen->Genetics)
		citizen->ApplyGeneticAffect(genetic);

	for (FString trait : AIData.CitizenData.PersonalityTraits) {
		FPersonality personality;
		personality.Trait = trait;

		int32 index = Camera->CitizenManager->Personalities.Find(personality);
		Camera->CitizenManager->Personalities[index].Citizens.Add(citizen);

		citizen->ApplyTraitAffect(Camera->CitizenManager->Personalities[index].Affects);
	}

	FCitizenManagerData cmData = CameraData.CitizenManagerData;

	if (cmData.InfectibleNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Infectible.Add(citizen);

	if (cmData.InfectedNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Infected.Add(citizen);

	if (cmData.InjuredNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Injured.Add(citizen);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(AIData.FactionName);
	for (FFactionData factionData : CameraData.FactionData) {
		if (factionData.Name != AIData.FactionName || !factionData.PoliticsData.RepresentativesNames.Contains(ActorData.Name))
			continue;

		faction->Politics.Representatives.Add(citizen);

		if (factionData.PoliticsData.VotesData.ForNames.Contains(ActorData.Name))
			faction->Politics.Votes.For.Add(citizen);
		else if (factionData.PoliticsData.VotesData.AgainstNames.Contains(ActorData.Name))
			faction->Politics.Votes.Against.Add(citizen);
		else if (factionData.PoliticsData.PredictionsData.ForNames.Contains(ActorData.Name))
			faction->Politics.Predictions.For.Add(citizen);
		else if (factionData.PoliticsData.PredictionsData.AgainstNames.Contains(ActorData.Name))
			faction->Politics.Predictions.Against.Add(citizen);

		break;
	}
}

void UDiplosimSaveGame::LoadBuilding(ACamera* Camera, FActorSaveData& ActorData, FBuildingData& BuildingData, TArray<FConstructionData>& ConstructionData, AActor* Actor, TMap<FString, FActorSaveData*>& AIToName, int32 Index)
{
	ABuilding* building = Cast<ABuilding>(Actor);

	building->ToggleDecalComponentVisibility(false);
	building->BuildingMesh->SetCanEverAffectNavigation(true);
	building->BuildingMesh->bReceivesDecals = true;

	building->FactionName = BuildingData.FactionName;

	if (BuildingData.bOnFire)
		Camera->Grid->AtmosphereComponent->SetOnFire(Actor, INDEX_NONE, true);

	bool bBuilt = true;
	for (FConstructionData constructionData : ConstructionData) {
		if (constructionData.BuildingName != ActorData.Name)
			continue;

		FConstructionStruct constructionStruct;
		constructionStruct.Status = constructionData.Status;
		constructionStruct.BuildPercentage = constructionData.BuildPercentage;
		constructionStruct.Building = building;

		if (constructionStruct.Status == EBuildStatus::Construction)
			constructionStruct.Building->SetConstructionMesh();

		Camera->ConstructionManager->Construction.Add(constructionStruct);
		bBuilt = false;

		break;
	}

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(building->FactionName);
	if (faction != nullptr) {
		if (bBuilt)
			faction->Buildings.Add(building);

		if (building->IsA<ABroch>())
			faction->EggTimer = Cast<ABroch>(building);
	}

	building->SetSeed(BuildingData.Seed);
	building->SetTier(BuildingData.Tier);

	building->Capacity = BuildingData.Capacity;
	building->StoreSocketLocations();

	FLinearColor colour = BuildingData.ChosenColour;
	building->SetBuildingColour(colour.R, colour.G, colour.B);

	building->TargetList = BuildingData.TargetList;

	building->Storage = BuildingData.Storage;
	building->Basket = BuildingData.Basket;

	if (BuildingData.DeathTime != 0.0f)
		building->Camera->Grid->AIVisualiser->DestructingActors.Add(building, BuildingData.DeathTime);

	building->bOperate = BuildingData.bOperate;

	building->Occupied.Empty();
	for (FCapacityData data : BuildingData.OccupiedData) {
		FCapacityStruct capacityStruct;

		if (data.CitizenName != "") {
			FActorSaveData* actorData = *AIToName.Find(data.CitizenName);

			Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, actorData, Saves[Index].AIData[actorData->dataIndex], false);

			capacityStruct.Citizen = Cast<ACitizen>(actorData->Actor);

			for (FString name : data.VisitorNames) {
				FActorSaveData* visitorData = *AIToName.Find(name);

				capacityStruct.Visitors.Add(Cast<ACitizen>(visitorData->Actor));

				Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, visitorData, Saves[Index].AIData[visitorData->dataIndex], true);
			}
		}

		capacityStruct.Amount = data.Amount;
		capacityStruct.WorkHours = data.WorkHours;
		capacityStruct.bBlocked = data.bBlocked;

		building->Occupied.Add(capacityStruct);
	}
}

void UDiplosimSaveGame::LoadProjectile(FProjectileData& ProjectileData, AActor* Actor)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);
	projectile->ProjectileMovementComponent->Velocity = ProjectileData.Velocity;
}

void UDiplosimSaveGame::LoadAISpawner(FSpawnerData& SpawnerData, AActor* Actor)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);

	spawner->Colour = SpawnerData.Colour;
	spawner->IncrementSpawned = SpawnerData.IncrementSpawned;
}

void UDiplosimSaveGame::LoadComponents(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	FHealthData healthData;
	FAttackData attackData;

	if (Actor->IsA<ABuilding>()) {
		healthData = Saves[Index].BuildingData[ActorData.dataIndex].HealthData;
		attackData = Saves[Index].BuildingData[ActorData.dataIndex].AttackData;
	}
	else if (Actor->IsA<AAI>()) {
		healthData = Saves[Index].AIData[ActorData.dataIndex].HealthData;
		attackData = Saves[Index].AIData[ActorData.dataIndex].AttackData;
	}
	else
		return;

	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp) {
		healthComp->Health = healthData.Health;

		if (healthComp->Health == 0 && !(Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->FactionName == ""))
			healthComp->Death(nullptr);
	}

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		attackComp->ProjectileClass = attackData.ProjectileClass;
		attackComp->AttackTimer = attackData.AttackTimer;
		attackComp->bShowMercy = attackData.bShowMercy;

		if (!attackComp->OverlappingEnemies.IsEmpty())
			attackComp->SetComponentTickEnabled(true);
	}
}

void UDiplosimSaveGame::LoadOverlappingEnemies(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, FActorSaveData SavedData, int32 Index)
{
	FAttackData attackData;

	if (Actor->IsA<ABuilding>())
		attackData = Saves[Index].BuildingData[ActorData.dataIndex].AttackData;
	else if (Actor->IsA<AAI>())
		attackData = Saves[Index].AIData[ActorData.dataIndex].AttackData;
	else 
		return;

	if (!attackData.ActorNames.Contains(SavedData.Name))
		return;

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	attackComp->OverlappingEnemies.Add(SavedData.Actor);
}

void UDiplosimSaveGame::LoadTimers(ACamera* Camera, int32 Index, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	for (FTimerStruct timer : ActorData.SavedTimers) {
		timer.Actor = ActorData.Actor;

		for (FTimerParameterStruct& params : timer.Parameters) {
			if (params.ObjectName == "")
				continue;

			for (FActorSaveData data : SavedData) {
				if (params.ObjectName.StartsWith("BP_")) {
					if (data.Name != params.ObjectName)
						continue;

					params.Object = data.Actor;
				}
				else {
					UAttackComponent* attackComponent = data.Actor->FindComponentByClass<UAttackComponent>();

					if (attackComponent && data.Name != attackComponent->OnHitSound->GetName())
						params.Object = attackComponent->OnHitSound;
					else {
						TArray<UActorComponent*>components;
						data.Actor->GetComponents(components);

						for (UActorComponent* component : components) {
							if (params.ObjectName != component->GetName())
								continue;

							params.Object = component;

							break;
						}
					}
				}

				if (IsValid(params.Object))
					break;
			}
		}

		if (timer.ID == "RemoveDamageOverlay")
			timer.Actor->FindComponentByClass<UHealthComponent>()->ApplyDamageOverlay(true);

		Camera->TimerManager->Timers.AddTail(timer);
	}
}

void UDiplosimSaveGame::InitialiseObjects(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index)
{
	if (ActorData.Actor->IsA<AProjectile>()) {
		if (SavedData.Name == Saves[Index].ProjectileData[ActorData.dataIndex].OwnerName)
			Cast<AProjectile>(ActorData.Actor)->SpawnNiagaraSystems(SavedData.Actor);
	}
	else if (ActorData.Actor->IsA<AAI>()) {
		InitialiseAI(Camera, ActorData, Saves[Index].AIData[ActorData.dataIndex], SavedData);

		if (ActorData.Actor->IsA<ACitizen>())
			InitialiseCitizen(Camera, ActorData, Saves[Index].AIData[ActorData.dataIndex], SavedData);
	}
	else if (ActorData.Actor->IsA<ACamera>()) {
		ACamera* camera = Cast<ACamera>(ActorData.Actor);

		InitialiseConstructionManager(camera, ActorData, SavedData, Index);

		InitialiseCitizenManager(camera, ActorData, SavedData, Index);

		InitialiseFactions(camera, ActorData, SavedData, Index);

		InitialiseGamemode(camera, Gamemode, ActorData, SavedData, Index);
	}
	else if (ActorData.Actor->IsA<AResource>()) {
		InitialiseResources(Camera, ActorData, Saves[Index].ResourceData[ActorData.dataIndex], SavedData);
	}
}

void UDiplosimSaveGame::InitialiseAI(ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FActorSaveData SavedData)
{
	AAI* ai = Cast<AAI>(ActorData.Actor);
	FAIMovementData movementData = AIData.MovementData;
	FMoveStruct moveStruct;

	/*int32 count = 0;
	int32 max = 5;

	TArray<FString> list = { movementData->ChosenBuildingName, movementData->ActorToLookAtName, movementData->ActorName, movementData->LinkedPortalName, movementData->UltimateGoalName };
	if (ActorData.Actor->IsA<ACitizen>()) {
		FCitizenData* citizenData = &ActorData.AIData.CitizenData;
		list.Append({ citizenData->MothersName, citizenData->FathersName, citizenData->PartnersName });
		list.Append(citizenData->ChildrensNames);
		list.Append(citizenData->SiblingsNames);

		max = 3 + citizenData->ChildrensNames.Num() + citizenData->SiblingsNames.Num();
	}

	for (FString name : list)
		if (name == "")
			count++;*/

	if (SavedData.Name == movementData.ChosenBuildingName)
		ai->AIController->ChosenBuilding = Cast<ABuilding>(SavedData.Actor);

	if (SavedData.Name == movementData.ActorToLookAtName)
		ai->MovementComponent->ActorToLookAt = SavedData.Actor;

	if (SavedData.Name == movementData.ActorName)
		moveStruct.Actor = SavedData.Actor;

	if (SavedData.Name == movementData.LinkedPortalName)
		moveStruct.LinkedPortal = SavedData.Actor;

	if (SavedData.Name == movementData.UltimateGoalName)
		moveStruct.UltimateGoal = SavedData.Actor;

	moveStruct.Instance = AIData.MovementData.Instance;
	moveStruct.Location = AIData.MovementData.Location;

	ai->AIController->MoveRequest = moveStruct;
}

void UDiplosimSaveGame::InitialiseCitizen(ACamera* Camera, FActorSaveData& ActorData, FAIData& AIData, FActorSaveData SavedData)
{
	ACitizen* citizen = Cast<ACitizen>(ActorData.Actor);
	FCitizenData citizenData = AIData.CitizenData;

	if (SavedData.Name == citizenData.MothersName)
		citizen->BioComponent->Mother = Cast<ACitizen>(SavedData.Actor);

	if (SavedData.Name == citizenData.FathersName)
		citizen->BioComponent->Father = Cast<ACitizen>(SavedData.Actor);

	if (SavedData.Name == citizenData.PartnersName)
		citizen->BioComponent->Partner = Cast<ACitizen>(SavedData.Actor);

	if (citizenData.ChildrensNames.Contains(SavedData.Name))
		citizen->BioComponent->Children.Add(Cast<ACitizen>(SavedData.Actor));

	if (citizenData.SiblingsNames.Contains(SavedData.Name))
		citizen->BioComponent->Siblings.Add(Cast<ACitizen>(SavedData.Actor));
}

void UDiplosimSaveGame::InitialiseConstructionManager(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index)
{
	for (int32 i = 0; i < Camera->ConstructionManager->Construction.Num(); i++) {
		FConstructionStruct construction = Camera->ConstructionManager->Construction[i];
		FConstructionData constructionData = Saves[Index].CameraData.ConstructionData[i];

		if (constructionData.BuildingName == SavedData.Name)
			construction.Building = Cast<ABuilding>(SavedData.Actor);
		else if (constructionData.BuilderName == SavedData.Name)
			construction.Builder = Cast<ABuilder>(SavedData.Actor);
	}
}

void UDiplosimSaveGame::InitialiseCitizenManager(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index)
{
	FCitizenManagerData cmData = Saves[Index].CameraData.CitizenManagerData;

	int32 count = 0;
	for (FPersonalityData personalityData : cmData.PersonalitiesData) {
		if (!personalityData.CitizenNames.Contains(SavedData.Name))
			continue;

		FPersonality personality;
		personality.Trait = personalityData.Trait;

		int32 index = Camera->CitizenManager->Personalities.Find(personality);

		Camera->CitizenManager->Personalities[index].Citizens.Add(Cast<ACitizen>(SavedData.Actor));
		count++;

		if (count == 2)
			break;
	}
}

void UDiplosimSaveGame::InitialiseFactions(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index)
{
	for (FFactionData data : Saves[Index].CameraData.FactionData) {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(data.Name);

		// Politics
		for (int32 i = 0; i < faction->Politics.Parties.Num(); i++) {
			FPartyStruct& party = faction->Politics.Parties[i];
			FPartyData partyData = data.PoliticsData.PartiesData[i];

			if (partyData.MembersName.Contains(SavedData.Name))
				party.Members.Add(Cast<ACitizen>(SavedData.Actor), *partyData.MembersName.Find(SavedData.Name));

			if (SavedData.Name == partyData.LeaderName)
				party.Leader = Cast<ACitizen>(SavedData.Actor);
		}

		// Police
		if (data.PoliceData.ArrestedNames.Contains(SavedData.Name))
			faction->Police.Arrested.Add(Cast<ACitizen>(SavedData.Actor), *data.PoliceData.ArrestedNames.Find(SavedData.Name));

		for (int32 i = 0; i < faction->Police.PoliceReports.Num(); i++) {
			FPoliceReport& report = faction->Police.PoliceReports[i];
			FPoliceReportData reportData = data.PoliceData.PoliceReportsData[i];

			if (SavedData.Name == reportData.Team1.InstigatorName)
				report.Team1.Instigator = Cast<ACitizen>(SavedData.Actor);

			if (reportData.Team1.AssistorsNames.Contains(SavedData.Name))
				report.Team1.Assistors.Add(Cast<ACitizen>(SavedData.Actor));

			if (SavedData.Name == reportData.Team2.InstigatorName)
				report.Team2.Instigator = Cast<ACitizen>(SavedData.Actor);

			if (reportData.Team2.AssistorsNames.Contains(SavedData.Name))
				report.Team2.Assistors.Add(Cast<ACitizen>(SavedData.Actor));

			if (reportData.WitnessesNames.Contains(SavedData.Name))
				report.Witnesses.Add(Cast<ACitizen>(SavedData.Actor));

			if (SavedData.Name == reportData.RespondingOfficerName)
				report.RespondingOfficer = Cast<ACitizen>(SavedData.Actor);

			if (reportData.AcussesTeam1Names.Contains(SavedData.Name))
				report.AcussesTeam1.Add(Cast<ACitizen>(SavedData.Actor));

			if (reportData.ImpartialNames.Contains(SavedData.Name))
				report.Impartial.Add(Cast<ACitizen>(SavedData.Actor));

			if (reportData.AcussesTeam2Names.Contains(SavedData.Name))
				report.AcussesTeam2.Add(Cast<ACitizen>(SavedData.Actor));
		}

		// Events
		for (int32 i = 0; i < faction->Police.PoliceReports.Num(); i++) {
			FEventStruct& event = faction->Events[i];
			FEventData eventData = data.EventsData[i];

			if (SavedData.Name == eventData.VenueName)
				event.Venue = Cast<ABuilding>(SavedData.Actor);

			if (eventData.WhitelistNames.Contains(SavedData.Name))
				event.Whitelist.Add(Cast<ACitizen>(SavedData.Actor));

			if (eventData.AttendeesNames.Contains(SavedData.Name))
				event.Attendees.Add(Cast<ACitizen>(SavedData.Actor));
		}

		// Army
		for (int32 i = 0; i < data.ArmiesData.Num(); i++) {
			if (!data.ArmiesData[i].CitizensNames.Contains(SavedData.Name))
				continue;

			Camera->ArmyManager->AddToArmy(i, { Cast<ACitizen>(SavedData.Actor) });
		}

		Camera->ConquestManager->DiplomacyComponent->SetFactionCulture(faction);
	}
}

void UDiplosimSaveGame::InitialiseGamemode(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, FActorSaveData SavedData, int32 Index)
{
	FGamemodeData* gamemodeData = &Saves[Index].CameraData.GamemodeData;

	for (int32 i = 0; i < Gamemode->WavesData.Num(); i++) {
		FWaveStruct& wave = Gamemode->WavesData[i];
		FWaveData waveData = gamemodeData->WaveData[i];

		TArray<FString> keys;
		waveData.DiedTo.GenerateKeyArray(keys);
		for (int32 j = 0; j < wave.DiedTo.Num(); j++) {
			if (keys[j] != SavedData.Name)
				continue;

			wave.DiedTo[j].Actor = SavedData.Actor;

			break;
		}

		if (waveData.Threats.Contains(SavedData.Name))
			wave.Threats.Add(SavedData.Actor);
	}

	if (!Gamemode->bSpawnedAllEnemies())
		Gamemode->SpawnAllEnemies();
}

void UDiplosimSaveGame::InitialiseResources(ACamera* Camera, FActorSaveData& ActorData, FResourceData& ResourceData, FActorSaveData SavedData)
{
	AResource* resource = Cast<AResource>(ActorData.Actor);

	for (int32 i = 0; i < resource->WorkerStruct.Num(); i++) {
		FWorkerStruct& workerStruct = resource->WorkerStruct[i];
		FWorkerData workerData = ResourceData.WorkersData[i];

		if (!workerData.CitizenNames.Contains(SavedData.Name))
			continue;

		workerStruct.Citizens.Add(Cast<ACitizen>(SavedData.Actor));

		break;
	}
}