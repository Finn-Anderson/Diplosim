#include "Player/Components/DiplosimSaveGame.h"

#include "Kismet/GameplayStatics.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/WidgetComponent.h"

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
	TArray<FActorSaveData> allNewActorData;

	TArray<AActor*> foundActors;

	TArray<AActor*> actors = { Camera, Camera->Grid };
	TArray<AActor*> potentialWetActors;

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AResource::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AAISpawner::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), AProjectile::StaticClass(), foundActors);
	actors.Append(foundActors);

	for (AActor* actor : actors)
	{
		if (!IsValid(actor) || actor->IsPendingKillPending() || (Camera->BuildComponent->Buildings.Contains(actor) && !actor->IsA<ABroch>()))
			continue;

		FActorSaveData actorData;
		actorData.Name = actor->GetName();
		actorData.Class = actor->GetClass();
		actorData.Transform = actor->GetActorTransform();

		if (actor->IsA<AGrid>())
			SaveWorld(actorData, actor, potentialWetActors);
		else if (actor->IsA<AResource>())
			SaveResource(actorData, actor);
		else if (actor->IsA<ACamera>()) {
			SaveCamera(actorData, actor);

			SaveCitizenManager(actorData, actor);

			SaveFactions(actorData, actor);

			SaveGamemode(actorData, actor);
		}
		else if (actor->IsA<AAI>()) {
			SaveAI(Camera, actorData, actor);

			if (actor->IsA<ACitizen>()) {
				SaveCitizen(Camera, actorData, actor);

				Saves[Index].CitizenNum++;
			}
		}
		else if (actor->IsA<ABuilding>())
			SaveBuilding(actorData, actor);
		else if (actor->IsA<AProjectile>())
			SaveProjectile(actorData, actor);
		else if (actor->IsA<AAISpawner>())
			SaveAISpawner(actorData, actor);

		SaveTimers(Camera, actorData, actor);

		SaveComponents(actorData, actor);

		allNewActorData.Add(actorData);
	}

	Saves[Index].SavedActors = allNewActorData;
	LastTimeUpdated = FDateTime::Now();

	UGameplayStatics::AsyncSaveGameToSlot(this, ID, 0);
}

void UDiplosimSaveGame::SaveWorld(FActorSaveData& ActorData, AActor* Actor, TArray<AActor*> PotentialWetActors)
{
	AGrid* grid = Cast<AGrid>(Actor);

	ActorData.WorldSaveData.Size = grid->Size;
	ActorData.WorldSaveData.Chunks = grid->Chunks;

	ActorData.WorldSaveData.Stream = grid->Camera->Stream;

	ActorData.WorldSaveData.LavaSpawnLocations = grid->LavaSpawnLocations;

	ActorData.WorldSaveData.Tiles.Empty();

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

			ActorData.WorldSaveData.Tiles.Add(t);
		}
	}

	ActorData.WorldSaveData.HISMData.Empty();
	ActorData.WorldSaveData.CloudsData.WetnessData.Empty();
	ActorData.WorldSaveData.CloudsData.CloudData.Empty();

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

			ActorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
		}

		data.CustomDataValues = hism->PerInstanceSMCustomData;

		ActorData.WorldSaveData.HISMData.Add(data);
	}

	ActorData.WorldSaveData.AtmosphereData.Calendar = grid->AtmosphereComponent->Calendar;
	ActorData.WorldSaveData.AtmosphereData.bRedSun = grid->AtmosphereComponent->bRedSun;
	ActorData.WorldSaveData.AtmosphereData.WindRotation = grid->AtmosphereComponent->WindRotation;

	ActorData.WorldSaveData.NaturalDisasterData.bDisasterChance = grid->AtmosphereComponent->NaturalDisasterComponent->bDisasterChance;
	ActorData.WorldSaveData.NaturalDisasterData.Frequency = grid->AtmosphereComponent->NaturalDisasterComponent->Frequency;
	ActorData.WorldSaveData.NaturalDisasterData.Intensity = grid->AtmosphereComponent->NaturalDisasterComponent->Intensity;

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

		ActorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
	}

	for (FCloudStruct cloud : grid->AtmosphereComponent->Clouds->Clouds) {
		FCloudData data;
		data.Transform = cloud.HISMCloud->GetRelativeTransform();
		data.Distance = cloud.Distance;
		data.bPrecipitation = cloud.Precipitation != nullptr;
		data.bHide = cloud.bHide;
		data.lightningFrequency = cloud.lightningFrequency;
		data.lightningTimer = cloud.lightningTimer;

		ActorData.WorldSaveData.CloudsData.CloudData.Add(data);
	}
}

void UDiplosimSaveGame::SaveResource(FActorSaveData& ActorData, AActor* Actor)
{
	AResource* resource = Cast<AResource>(Actor);

	ActorData.ResourceData.HISMData.Name = resource->ResourceHISM->GetName();

	ActorData.ResourceData.HISMData.Transforms.Empty();
	for (int32 i = 0; i < resource->ResourceHISM->GetInstanceCount(); i++) {
		FTransform transform;
		resource->ResourceHISM->GetInstanceTransform(i, transform);

		ActorData.ResourceData.HISMData.Transforms.Add(transform);
	}

	ActorData.ResourceData.HISMData.CustomDataValues = resource->ResourceHISM->PerInstanceSMCustomData;

	ActorData.ResourceData.Workers = resource->WorkerStruct;
}

void UDiplosimSaveGame::SaveCamera(FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);
	ActorData.CameraData.ColonyName = camera->ColonyName;

	ActorData.CameraData.ConstructionData.Empty();
	for (FConstructionStruct constructionStruct : camera->ConstructionManager->Construction) {
		FConstructionData data;
		data.BuildingName = constructionStruct.Building->GetName();
		data.BuildPercentage = constructionStruct.BuildPercentage;
		data.Status = constructionStruct.Status;

		if (IsValid(constructionStruct.Builder))
			data.BuilderName = constructionStruct.Builder->GetName();

		ActorData.CameraData.ConstructionData.Add(data);
	}
}

void UDiplosimSaveGame::SaveCitizenManager(FActorSaveData& ActorData, AActor* Actor)
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

	ActorData.CameraData.CitizenManagerData = cmData;
}

void UDiplosimSaveGame::SaveFactions(FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	ActorData.CameraData.FactionData.Empty();

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

		ActorData.CameraData.FactionData.Add(data);
	}
}

void UDiplosimSaveGame::SaveGamemode(FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	ADiplosimGameModeBase* gamemode = camera->GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
	FGamemodeData* gamemodeData = &ActorData.CameraData.GamemodeData;

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

void UDiplosimSaveGame::SaveAI(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	AAI* ai = Cast<AAI>(Actor);
	FAIData* data = &ActorData.AIData;

	ADiplosimGameModeBase* gamemode = Camera->GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", ai);

	data->Colour = ai->Colour;

	if (ai->MovementComponent->bSetPoints)
		data->MovementData.Points = ai->MovementComponent->TempPoints;
	else
		data->MovementData.Points = ai->MovementComponent->Points;

	data->MovementData.CurrentAnim = ai->MovementComponent->CurrentAnim;
	data->MovementData.LastUpdatedTime = ai->MovementComponent->LastUpdatedTime;
	data->MovementData.Transform = ai->MovementComponent->Transform;

	if (IsValid(ai->MovementComponent->ActorToLookAt))
		data->MovementData.ActorToLookAtName = ai->MovementComponent->ActorToLookAt->GetName();

	if (IsValid(ai->AIController->ChosenBuilding))
		data->MovementData.ChosenBuildingName = ai->AIController->ChosenBuilding->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetGoalActor()))
		data->MovementData.ActorName = ai->AIController->MoveRequest.GetGoalActor()->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetLinkedPortal()))
		data->MovementData.LinkedPortalName = ai->AIController->MoveRequest.GetLinkedPortal()->GetName();

	if (IsValid(ai->AIController->MoveRequest.GetUltimateGoalActor()))
		data->MovementData.UltimateGoalName = ai->AIController->MoveRequest.GetUltimateGoalActor()->GetName();

	data->MovementData.Instance = ai->AIController->MoveRequest.GetGoalInstance();
	data->MovementData.Location = ai->AIController->MoveRequest.GetLocation();

	if (gamemode->Snakes.Contains(ai))
		data->bSnake = true;

	if (faction != nullptr)
		data->FactionName = faction->Name;
}

void UDiplosimSaveGame::SaveCitizen(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	ACitizen* citizen = Cast<ACitizen>(Actor);
	FAIData* data = &ActorData.AIData;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction("", citizen);

	if (faction != nullptr) {
		if (faction->Rebels.Contains(citizen))
			data->CitizenData.bRebel = true;

		data->CitizenData.EnterLocation = citizen->BuildingComponent->EnterLocation;
	}

	if (IsValid(citizen->Carrying.Type)) {
		data->CitizenData.ResourceCarryClass = citizen->Carrying.Type->GetClass();
		data->CitizenData.CarryAmount = citizen->Carrying.Amount;
	}

	if (citizen->BioComponent->Mother != nullptr)
		data->CitizenData.MothersName = citizen->BioComponent->Mother->GetName();

	if (citizen->BioComponent->Father != nullptr)
		data->CitizenData.FathersName = citizen->BioComponent->Father->GetName();

	if (citizen->BioComponent->Partner != nullptr)
		data->CitizenData.PartnersName = citizen->BioComponent->Partner->GetName();

	data->CitizenData.ChildrensNames.Empty();
	data->CitizenData.SiblingsNames.Empty();

	for (ACitizen* child : citizen->BioComponent->Children)
		data->CitizenData.ChildrensNames.Add(child->GetName());

	for (ACitizen* sibling : citizen->BioComponent->Siblings)
		data->CitizenData.SiblingsNames.Add(sibling->GetName());

	data->CitizenData.HoursTogetherWithPartner = citizen->BioComponent->HoursTogetherWithPartner;
	data->CitizenData.bMarried = citizen->BioComponent->bMarried;
	data->CitizenData.Sex = citizen->BioComponent->Sex;
	data->CitizenData.Age = citizen->BioComponent->Age;
	data->CitizenData.Name = citizen->BioComponent->Name;
	data->CitizenData.EducationLevel = citizen->BioComponent->EducationLevel;
	data->CitizenData.EducationProgress = citizen->BioComponent->EducationProgress;
	data->CitizenData.PaidForEducationLevel = citizen->BioComponent->PaidForEducationLevel;
	data->CitizenData.bAdopted = citizen->BioComponent->bAdopted;
	data->CitizenData.SpeedBeforeOld = citizen->BioComponent->SpeedBeforeOld;
	data->CitizenData.MaxHealthBeforeOld = citizen->BioComponent->MaxHealthBeforeOld;

	data->CitizenData.Spirituality = citizen->Spirituality;
	data->CitizenData.TimeOfAcquirement = citizen->BuildingComponent->TimeOfAcquirement;
	data->CitizenData.VoicePitch = citizen->VoicePitch;
	data->CitizenData.Balance = citizen->Balance;
	data->CitizenData.HoursWorked = citizen->BuildingComponent->HoursWorked;
	data->CitizenData.Hunger = citizen->Hunger;
	data->CitizenData.Energy = citizen->Energy;
	data->CitizenData.bGain = citizen->bGain;
	data->CitizenData.bHasBeenLeader = citizen->bHasBeenLeader;
	data->CitizenData.HealthIssues = citizen->HealthIssues;
	data->CitizenData.Modifiers = citizen->HappinessComponent->Modifiers;
	data->CitizenData.SadTimer = citizen->HappinessComponent->SadTimer;
	data->CitizenData.MassStatus = citizen->HappinessComponent->MassStatus;
	data->CitizenData.FestivalStatus = citizen->HappinessComponent->FestivalStatus;
	data->CitizenData.bConversing = citizen->bConversing;
	data->CitizenData.ConversationHappiness = citizen->HappinessComponent->ConversationHappiness;
	data->CitizenData.FamilyDeathHappiness = citizen->HappinessComponent->FamilyDeathHappiness;
	data->CitizenData.WitnessedDeathHappiness = citizen->HappinessComponent->WitnessedDeathHappiness;
	data->CitizenData.Genetics = citizen->Genetics;
	data->CitizenData.bSleep = citizen->bSleep;
	data->CitizenData.HoursSleptToday = citizen->HoursSleptToday;

	data->CitizenData.PersonalityTraits.Empty();
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
		data->CitizenData.PersonalityTraits.Add(personality->Trait);
}

void UDiplosimSaveGame::SaveBuilding(FActorSaveData& ActorData, AActor* Actor)
{
	ABuilding* building = Cast<ABuilding>(Actor);

	ActorData.BuildingData.FactionName = building->FactionName;
	ActorData.BuildingData.Capacity = building->Capacity;

	ActorData.BuildingData.Seed = building->SeedNum;
	ActorData.BuildingData.ChosenColour = building->ChosenColour;
	ActorData.BuildingData.Tier = building->Tier;

	ActorData.BuildingData.TargetList = building->TargetList;

	ActorData.BuildingData.Storage = building->Storage;
	ActorData.BuildingData.Basket = building->Basket;

	double* time = building->Camera->Grid->AIVisualiser->DestructingActors.Find(building);
	if (time != nullptr)
		ActorData.BuildingData.DeathTime = *time;

	ActorData.BuildingData.bOperate = building->bOperate;

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

	ActorData.BuildingData.OccupiedData = occData;
}

void UDiplosimSaveGame::SaveProjectile(FActorSaveData& ActorData, AActor* Actor)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);

	ActorData.ProjectileData.OwnerName = projectile->GetOwner()->GetName();
	ActorData.ProjectileData.Velocity = projectile->ProjectileMovementComponent->Velocity;
}

void UDiplosimSaveGame::SaveAISpawner(FActorSaveData& ActorData, AActor* Actor)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);

	ActorData.SpawnerData.Colour = spawner->Colour;
	ActorData.SpawnerData.IncrementSpawned = spawner->IncrementSpawned;
}

void UDiplosimSaveGame::SaveTimers(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	for (FTimerStruct timer : Camera->TimerManager->Timers) {
		if (timer.Actor != Actor)
			continue;

		for (FTimerParameterStruct param : timer.Parameters) {
			if (!IsValid(param.Object))
				continue;

			param.ObjectClass = param.Object->GetClass();
			param.ObjectName = param.Object->GetName();
		}

		ActorData.SavedTimers.Add(timer);
	}
}

void UDiplosimSaveGame::SaveComponents(FActorSaveData& ActorData, AActor* Actor)
{
	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp)
		ActorData.HealthData.Health = healthComp->Health;

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		ActorData.AttackData.ProjectileClass = attackComp->ProjectileClass;
		ActorData.AttackData.AttackTimer = attackComp->AttackTimer;
		ActorData.AttackData.bShowMercy = attackComp->bShowMercy;

		for (AActor* enemies : attackComp->OverlappingEnemies)
			ActorData.AttackData.ActorNames.Add(enemies->GetName());
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

	TArray<AActor*> actors;
	TArray<AActor*> foundActors;

	TArray<UClass*> classes = { AAI::StaticClass(), AEggBasket::StaticClass(), ABuilding::StaticClass(), AAISpawner::StaticClass(), AProjectile::StaticClass() };

	for (UClass* clss : classes) {
		UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), clss, foundActors);
		actors.Append(foundActors);
	}

	for (AActor* a : actors)
		a->Destroy();

	TMap<FString, FActorSaveData> aiToName;
	TArray<FWetnessData> wetnessData;

	for (FActorSaveData& actorData : Saves[Index].SavedActors) {
		AActor* actor = nullptr;

		UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), actorData.Class, foundActors);

		if (foundActors.Num() == 1 && (foundActors[0]->IsA<ACamera>() || foundActors[0]->IsA<AGrid>() || foundActors[0]->IsA<AResource>())) {
			actor = foundActors[0];

			actor->SetActorTransform(actorData.Transform);
		}
		else {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = Camera->GetWorld()->SpawnActor<AActor>(actorData.Class, actorData.Transform, params);
		}

		if (actor->IsA<AGrid>())
			LoadWorld(actorData, actor, wetnessData);
		else if (actor->IsA<AResource>())
			LoadResource(actorData, actor);
		else if (actor->IsA<AEggBasket>())
			LoadEggBasket(Camera, actorData, actor);
		else if (actor->IsA<ACamera>()) {
			LoadCamera(actorData, actor);

			LoadFactions(actorData, actor);

			LoadGamemode(gamemode, actorData, actor);
		}
		else if (actor->IsA<AAI>()) {
			LoadAI(Camera, actorData, actor, aiToName);

			if (actor->IsA<ACitizen>())
				LoadCitizen(Camera, actorData, actor);
		}
		else if (actor->IsA<ABuilding>())
			LoadBuilding(Camera, actorData, actor, aiToName);
		else if (actor->IsA<AProjectile>())
			LoadProjectile(actorData, actor);
		else if (actor->IsA<AAISpawner>())
			LoadAISpawner(actorData, actor);

		actorData.Actor = actor;
	}

	for (FActorSaveData actorData : Saves[Index].SavedActors) {
		LoadTimers(Camera, Index, actorData);

		TArray<FActorSaveData> savedData = Saves[Index].SavedActors;

		LoadComponents(Camera, actorData, actorData.Actor, savedData);

		InitialiseObjects(Camera, gamemode, actorData, savedData);
	}

	for (FWetnessData data : wetnessData)
		Camera->Grid->AtmosphereComponent->Clouds->RainCollisionHandler(data.Location, data.Value, data.Increment);
}

void UDiplosimSaveGame::LoadWorld(FActorSaveData& ActorData, AActor* Actor, TArray<FWetnessData>& WetnessData)
{
	AGrid* grid = Cast<AGrid>(Actor);

	grid->Size = ActorData.WorldSaveData.Size;
	grid->Chunks = ActorData.WorldSaveData.Chunks;

	grid->Camera->Stream = ActorData.WorldSaveData.Stream;

	grid->LavaSpawnLocations = ActorData.WorldSaveData.LavaSpawnLocations;

	grid->Clear();

	grid->InitialiseStorage();
	auto bound = grid->GetMapBounds();

	for (FTileData t : ActorData.WorldSaveData.Tiles) {
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

		int32 index = ActorData.WorldSaveData.HISMData.Find(data);
		TArray<FTransform> transforms = ActorData.WorldSaveData.HISMData[index].Transforms;

		hism->AddInstances(transforms, false, true, true);
		hism->PerInstanceSMCustomData = ActorData.WorldSaveData.HISMData[index].CustomDataValues;
		hism->BuildTreeIfOutdated(true, true);

		hism->PartialNavigationUpdates(transforms);
	}

	grid->AtmosphereComponent->Calendar = ActorData.WorldSaveData.AtmosphereData.Calendar;
	grid->AtmosphereComponent->bRedSun = ActorData.WorldSaveData.AtmosphereData.bRedSun;
	grid->AtmosphereComponent->WindRotation = ActorData.WorldSaveData.AtmosphereData.WindRotation;

	if (grid->AtmosphereComponent->bRedSun)
		grid->AtmosphereComponent->NaturalDisasterComponent->AlterSunGradually(0.15f, -1.00f);

	grid->AtmosphereComponent->Clouds->ProcessRainEffect.Empty();
	grid->AtmosphereComponent->Clouds->WetnessStruct.Empty();
	WetnessData = ActorData.WorldSaveData.CloudsData.WetnessData;

	for (FCloudData data : ActorData.WorldSaveData.CloudsData.CloudData) {
		FCloudStruct cloudStruct = grid->AtmosphereComponent->Clouds->CreateCloud(data.Transform, data.bPrecipitation ? 100 : 0);
		cloudStruct.Distance = data.Distance;
		cloudStruct.bHide = data.bHide;
		cloudStruct.lightningFrequency = data.lightningFrequency;
		cloudStruct.lightningTimer = data.lightningTimer;

		grid->AtmosphereComponent->Clouds->Clouds.Add(cloudStruct);
	}

	grid->SetupEnvironment(true);
	grid->AIVisualiser->ResetToDefaultValues();
}

void UDiplosimSaveGame::LoadResource(FActorSaveData& ActorData, AActor* Actor)
{
	AResource* resource = Cast<AResource>(Actor);

	resource->ResourceHISM->AddInstances(ActorData.ResourceData.HISMData.Transforms, false, true, true);
	resource->ResourceHISM->PerInstanceSMCustomData = ActorData.ResourceData.HISMData.CustomDataValues;

	resource->WorkerStruct = ActorData.ResourceData.Workers;

	resource->ResourceHISM->BuildTreeIfOutdated(true, true);
}

void UDiplosimSaveGame::LoadEggBasket(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	AEggBasket* eggBasket = Cast<AEggBasket>(Actor);

	FTileStruct* tile = Camera->Grid->GetTileFromLocation(ActorData.Transform.GetLocation());

	eggBasket->Grid = Camera->Grid;
	eggBasket->Tile = tile;

	Camera->Grid->ResourceTiles.RemoveSingle(tile);
}

void UDiplosimSaveGame::LoadCamera(FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);
	camera->ColonyName = ActorData.CameraData.ColonyName;

	camera->Start = false;
	camera->bStartMenu = false;
	camera->Cancel();

	camera->ClearPopupUI();
	camera->SetInteractStatus(camera->WidgetComponent->GetAttachmentRootActor(), false);

	camera->Detach();
	camera->MovementComponent->TargetLength = 3000.0f;
	camera->MovementComponent->MovementLocation = ActorData.Transform.GetLocation();

	camera->ConstructionManager->Construction.Empty();

	camera->CitizenManager->IssuePensionHour = ActorData.CameraData.CitizenManagerData.IssuePensionHour;
}

void UDiplosimSaveGame::LoadFactions(FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	camera->ConquestManager->Factions.Empty();
	for (FFactionData data : ActorData.CameraData.FactionData) {
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

		camera->ConquestManager->Factions.Add(faction);
	}
}

void UDiplosimSaveGame::LoadGamemode(ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, AActor* Actor)
{
	ACamera* camera = Cast<ACamera>(Actor);

	FGamemodeData* gamemodeData = &ActorData.CameraData.GamemodeData;

	Gamemode->bOngoingRaid = gamemodeData->bOngoingRaid;
	camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, gamemodeData->CrystalOpacity);
	Gamemode->TargetOpacity = gamemodeData->TargetOpacity;

	if (gamemodeData->CrystalOpacity != Gamemode->TargetOpacity)
		Gamemode->SetActorTickEnabled(true);
}

void UDiplosimSaveGame::LoadAI(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TMap<FString, FActorSaveData>& AIToName)
{
	AAI* ai = Cast<AAI>(Actor);

	FAIData* data = &ActorData.AIData;

	AIToName.Add(ActorData.Name, ActorData);

	UHierarchicalInstancedStaticMeshComponent* hism;

	if (data->FactionName == "") {
		if (data->bSnake)
			hism = Camera->Grid->AIVisualiser->HISMSnake;
		else
			hism = Camera->Grid->AIVisualiser->HISMEnemy;
	}
	else {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(data->FactionName);

		if (ai->IsA<AClone>()) {
			faction->Clones.Add(ai);
			hism = Camera->Grid->AIVisualiser->HISMClone;
		}
		else if (data->CitizenData.bRebel) {
			faction->Rebels.Add(Cast<ACitizen>(ai));
			hism = Camera->Grid->AIVisualiser->HISMRebel;
		}
		else {
			faction->Citizens.Add(Cast<ACitizen>(ai));
			hism = Camera->Grid->AIVisualiser->HISMCitizen;
		}
	}

	ai->Colour = data->Colour;

	ai->MovementComponent->Points = data->MovementData.Points;
	ai->MovementComponent->CurrentAnim = data->MovementData.CurrentAnim;
	ai->MovementComponent->LastUpdatedTime = data->MovementData.LastUpdatedTime;

	Camera->Grid->AIVisualiser->AddInstance(ai, hism, data->MovementData.Transform);
}

void UDiplosimSaveGame::LoadCitizen(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	ACitizen* citizen = Cast<ACitizen>(Actor);

	FAIData* data = &ActorData.AIData;

	if (IsValid(data->CitizenData.ResourceCarryClass)) {
		citizen->Carrying.Type = Cast<AResource>(data->CitizenData.ResourceCarryClass->GetDefaultObject());
		citizen->Carrying.Amount = data->CitizenData.CarryAmount;
	}

	citizen->BioComponent->HoursTogetherWithPartner = data->CitizenData.HoursTogetherWithPartner;
	citizen->BioComponent->bMarried = data->CitizenData.bMarried;
	citizen->BioComponent->Sex = data->CitizenData.Sex;
	citizen->BioComponent->Age = data->CitizenData.Age;
	citizen->BioComponent->Name = data->CitizenData.Name;
	citizen->BioComponent->EducationLevel = data->CitizenData.EducationLevel;
	citizen->BioComponent->EducationProgress = data->CitizenData.EducationProgress;
	citizen->BioComponent->PaidForEducationLevel = data->CitizenData.PaidForEducationLevel;
	citizen->BioComponent->bAdopted = data->CitizenData.bAdopted;
	citizen->BioComponent->SpeedBeforeOld = data->CitizenData.SpeedBeforeOld;
	citizen->BioComponent->MaxHealthBeforeOld = data->CitizenData.MaxHealthBeforeOld;

	citizen->Spirituality = data->CitizenData.Spirituality;
	citizen->BuildingComponent->TimeOfAcquirement = data->CitizenData.TimeOfAcquirement;
	citizen->VoicePitch = data->CitizenData.VoicePitch;
	citizen->Balance = data->CitizenData.Balance;
	citizen->BuildingComponent->HoursWorked = data->CitizenData.HoursWorked;
	citizen->Hunger = data->CitizenData.Hunger;
	citizen->Energy = data->CitizenData.Energy;
	citizen->bGain = data->CitizenData.bGain;
	citizen->bHasBeenLeader = data->CitizenData.bHasBeenLeader;
	citizen->HealthIssues = data->CitizenData.HealthIssues;
	citizen->HappinessComponent->Modifiers = data->CitizenData.Modifiers;
	citizen->HappinessComponent->SadTimer = data->CitizenData.SadTimer;
	citizen->HappinessComponent->MassStatus = data->CitizenData.MassStatus;
	citizen->HappinessComponent->FestivalStatus = data->CitizenData.FestivalStatus;
	citizen->HappinessComponent->ConversationHappiness = data->CitizenData.ConversationHappiness;
	citizen->HappinessComponent->FamilyDeathHappiness = data->CitizenData.FamilyDeathHappiness;
	citizen->HappinessComponent->WitnessedDeathHappiness = data->CitizenData.WitnessedDeathHappiness;
	citizen->Genetics = data->CitizenData.Genetics;
	citizen->bSleep = data->CitizenData.bSleep;
	citizen->HoursSleptToday = data->CitizenData.HoursSleptToday;

	if (data->CitizenData.bConversing)
		Camera->PlayAmbientSound(citizen->AmbientAudioComponent, Camera->CitizenManager->GetConversationSound(citizen), citizen->VoicePitch);

	for (FGeneticsStruct& genetic : citizen->Genetics)
		citizen->ApplyGeneticAffect(genetic);

	for (FString trait : data->CitizenData.PersonalityTraits) {
		FPersonality personality;
		personality.Trait = trait;

		int32 index = Camera->CitizenManager->Personalities.Find(personality);
		Camera->CitizenManager->Personalities[index].Citizens.Add(citizen);

		citizen->ApplyTraitAffect(Camera->CitizenManager->Personalities[index].Affects);
	}
}

void UDiplosimSaveGame::LoadBuilding(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TMap<FString, FActorSaveData>& AIToName)
{
	ABuilding* building = Cast<ABuilding>(Actor);

	building->BuildingMesh->SetCanEverAffectNavigation(true);
	building->BuildingMesh->bReceivesDecals = true;

	building->FactionName = ActorData.BuildingData.FactionName;

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(building->FactionName);
	if (faction != nullptr) {
		faction->Buildings.Add(building);

		if (building->IsA<ABroch>())
			faction->EggTimer = Cast<ABroch>(building);
	}

	building->SetSeed(ActorData.BuildingData.Seed);
	building->SetTier(ActorData.BuildingData.Tier);

	building->Capacity = ActorData.BuildingData.Capacity;

	FLinearColor colour = ActorData.BuildingData.ChosenColour;
	building->SetBuildingColour(colour.R, colour.G, colour.B);

	building->TargetList = ActorData.BuildingData.TargetList;

	building->Storage = ActorData.BuildingData.Storage;
	building->Basket = ActorData.BuildingData.Basket;

	if (ActorData.BuildingData.DeathTime != 0.0f)
		building->Camera->Grid->AIVisualiser->DestructingActors.Add(building, ActorData.BuildingData.DeathTime);

	building->bOperate = ActorData.BuildingData.bOperate;

	for (FCapacityData data : ActorData.BuildingData.OccupiedData) {
		FCapacityStruct capacityStruct;

		if (data.CitizenName != "") {
			FActorSaveData aiData = *AIToName.Find(data.CitizenName);

			Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, aiData, false);

			capacityStruct.Citizen = Cast<ACitizen>(aiData.Actor);

			for (FString name : data.VisitorNames) {
				FActorSaveData visitorData = *AIToName.Find(name);

				capacityStruct.Visitors.Add(Cast<ACitizen>(visitorData.Actor));

				Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, visitorData, true);
			}
		}

		capacityStruct.Amount = data.Amount;
		capacityStruct.WorkHours = data.WorkHours;
		capacityStruct.bBlocked = data.bBlocked;

		building->Occupied.Add(capacityStruct);
	}
}

void UDiplosimSaveGame::LoadProjectile(FActorSaveData& ActorData, AActor* Actor)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);

	projectile->ProjectileMovementComponent->Velocity = ActorData.ProjectileData.Velocity;
}

void UDiplosimSaveGame::LoadAISpawner(FActorSaveData& ActorData, AActor* Actor)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);

	spawner->Colour = ActorData.SpawnerData.Colour;
	spawner->IncrementSpawned = ActorData.SpawnerData.IncrementSpawned;
}

void UDiplosimSaveGame::LoadComponents(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, TArray<FActorSaveData> SavedData)
{
	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp) {
		healthComp->Health = ActorData.HealthData.Health;

		if (healthComp->Health == 0 && !(Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->FactionName == ""))
			healthComp->Death(nullptr);
	}

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		attackComp->ProjectileClass = ActorData.AttackData.ProjectileClass;
		attackComp->AttackTimer = ActorData.AttackData.AttackTimer;
		attackComp->bShowMercy = ActorData.AttackData.bShowMercy;

		for (FString name : ActorData.AttackData.ActorNames)
			attackComp->OverlappingEnemies.Add(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name));

		if (!attackComp->OverlappingEnemies.IsEmpty())
			attackComp->SetComponentTickEnabled(true);
	}
}

void UDiplosimSaveGame::LoadTimers(ACamera* Camera, int32 Index, FActorSaveData& ActorData)
{
	for (FTimerStruct timer : ActorData.SavedTimers) {
		timer.Actor = ActorData.Actor;

		for (FTimerParameterStruct params : timer.Parameters) {
			for (FActorSaveData data : Saves[Index].SavedActors) {
				if (params.ObjectClass == USoundBase::StaticClass()) {
					UAttackComponent* attackComponent = data.Actor->FindComponentByClass<UAttackComponent>();

					if (!attackComponent)
						continue;

					params.Object = attackComponent->OnHitSound;
				}
				else if (params.ObjectClass == data.Actor->GetClass()) {
					if (data.Name != params.ObjectName)
						continue;

					params.Object = data.Actor;
				}
				else {
					TArray<UActorComponent*>components;
					data.Actor->GetComponents(components);

					for (UActorComponent* component : components) {
						if (!component->IsA(params.ObjectClass) || params.ObjectName != component->GetName())
							continue;

						params.Object = component;

						break;
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

void UDiplosimSaveGame::InitialiseObjects(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	if (ActorData.Actor->IsA<AProjectile>()) {
		Cast<AProjectile>(ActorData.Actor)->SpawnNiagaraSystems(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, ActorData.ProjectileData.OwnerName));
	}
	else if (ActorData.Actor->IsA<AAI>()) {
		InitialiseAI(Camera, ActorData, SavedData);

		if (ActorData.Actor->IsA<ACitizen>())
			InitialiseCitizen(Camera, ActorData, SavedData);
	}
	else if (ActorData.Actor->IsA<ACamera>()) {
		ACamera* camera = Cast<ACamera>(ActorData.Actor);

		InitialiseConstructionManager(camera, ActorData, SavedData);

		InitialiseCitizenManager(camera, ActorData, SavedData);

		InitialiseFactions(camera, ActorData, SavedData);

		InitialiseGamemode(camera, Gamemode, ActorData, SavedData);
	}
}

void UDiplosimSaveGame::InitialiseAI(ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	AAI* ai = Cast<AAI>(ActorData.Actor);
	FAIMovementData* movementData = &ActorData.AIData.MovementData;

	ai->AIController->ChosenBuilding = Cast<ABuilding>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, movementData->ChosenBuildingName));

	ai->MovementComponent->ActorToLookAt = Camera->SaveGameComponent->GetSaveActorFromName(SavedData, movementData->ActorToLookAtName);

	FMoveStruct moveStruct;
	moveStruct.Actor = Camera->SaveGameComponent->GetSaveActorFromName(SavedData, movementData->ActorName);
	moveStruct.LinkedPortal = Camera->SaveGameComponent->GetSaveActorFromName(SavedData, movementData->LinkedPortalName);
	moveStruct.UltimateGoal = Camera->SaveGameComponent->GetSaveActorFromName(SavedData, movementData->UltimateGoalName);

	moveStruct.Instance = ActorData.AIData.MovementData.Instance;
	moveStruct.Location = ActorData.AIData.MovementData.Location;

	ai->AIController->MoveRequest = moveStruct;
}

void UDiplosimSaveGame::InitialiseCitizen(ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	ACitizen* citizen = Cast<ACitizen>(ActorData.Actor);
	FCitizenData* citizenData = &ActorData.AIData.CitizenData;

	citizen->BioComponent->Mother = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, citizenData->MothersName));

	citizen->BioComponent->Father = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, citizenData->FathersName));

	citizen->BioComponent->Partner = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, citizenData->PartnersName));

	for (FString name : citizenData->ChildrensNames)
		citizen->BioComponent->Children.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

	for (FString name : citizenData->SiblingsNames)
		citizen->BioComponent->Siblings.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));
}

void UDiplosimSaveGame::InitialiseConstructionManager(ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	for (FConstructionData constructionData : ActorData.CameraData.ConstructionData) {
		FConstructionStruct constructionStruct;
		constructionStruct.Status = constructionData.Status;
		constructionStruct.BuildPercentage = constructionData.BuildPercentage;
		constructionStruct.Building = Cast<ABuilding>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, constructionData.BuildingName));

		if (constructionStruct.Status == EBuildStatus::Construction)
			constructionStruct.Building->SetConstructionMesh();

		if (constructionData.BuilderName != "")
			constructionStruct.Builder = Cast<ABuilder>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, constructionData.BuilderName));

		Camera->ConstructionManager->Construction.Add(constructionStruct);
	}
}

void UDiplosimSaveGame::InitialiseCitizenManager(ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	FCitizenManagerData cmData = ActorData.CameraData.CitizenManagerData;

	for (FPersonalityData personalityData : cmData.PersonalitiesData) {
		FPersonality personality;
		personality.Trait = personalityData.Trait;

		int32 index = Camera->CitizenManager->Personalities.Find(personality);

		for (FString name : personalityData.CitizenNames)
			Camera->CitizenManager->Personalities[index].Citizens.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));
	}

	for (FString name : cmData.InfectibleNames)
		Camera->DiseaseManager->Infectible.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

	for (FString name : cmData.InfectedNames)
		Camera->DiseaseManager->Infected.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

	for (FString name : cmData.InjuredNames)
		Camera->DiseaseManager->Injured.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));
}

void UDiplosimSaveGame::InitialiseFactions(ACamera* Camera, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	for (FFactionData data : ActorData.CameraData.FactionData) {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(data.Name);

		// Politics
		for (FPartyData partyData : data.PoliticsData.PartiesData) {
			FPartyStruct party;

			party.Party = partyData.Party;
			party.Agreeable = partyData.Agreeable;

			for (auto element : partyData.MembersName)
				party.Members.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, element.Key)), element.Value);

			if (partyData.LeaderName != "")
				party.Leader = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, partyData.LeaderName));

			faction->Politics.Parties.Add(party);
		}

		for (FString name : data.PoliticsData.RepresentativesNames)
			faction->Politics.Representatives.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

		for (FString name : data.PoliticsData.VotesData.ForNames)
			faction->Politics.Votes.For.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

		for (FString name : data.PoliticsData.VotesData.AgainstNames)
			faction->Politics.Votes.Against.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

		for (FString name : data.PoliticsData.PredictionsData.ForNames)
			faction->Politics.Predictions.For.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

		for (FString name : data.PoliticsData.PredictionsData.AgainstNames)
			faction->Politics.Predictions.Against.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

		faction->Politics.BribeValue = data.PoliticsData.BribeValue;
		faction->Politics.Laws = data.PoliticsData.Laws;
		faction->Politics.ProposedBills = data.PoliticsData.ProposedBills;

		// Police
		for (auto element : data.PoliceData.ArrestedNames)
			faction->Police.Arrested.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, element.Key)), element.Value);

		for (FPoliceReportData reportData : data.PoliceData.PoliceReportsData) {
			FPoliceReport report;
			report.Type = reportData.Type;
			report.Location = reportData.Location;

			if (reportData.Team1.InstigatorName != "")
				report.Team1.Instigator = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, reportData.Team1.InstigatorName));

			for (FString name : reportData.Team1.AssistorsNames)
				report.Team1.Assistors.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			if (reportData.Team2.InstigatorName != "")
				report.Team2.Instigator = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, reportData.Team2.InstigatorName));

			for (FString name : reportData.Team2.AssistorsNames)
				report.Team2.Assistors.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			for (auto element : reportData.WitnessesNames)
				report.Witnesses.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, element.Key)), element.Value);

			if (reportData.RespondingOfficerName != "")
				report.RespondingOfficer = Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, reportData.RespondingOfficerName));

			for (FString name : reportData.AcussesTeam1Names)
				report.AcussesTeam1.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			for (FString name : reportData.ImpartialNames)
				report.Impartial.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			for (FString name : reportData.AcussesTeam2Names)
				report.AcussesTeam2.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			faction->Police.PoliceReports.Add(report);
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

			if (eventData.VenueName != "")
				event.Venue = Cast<ABuilding>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, eventData.VenueName));

			for (FString name : eventData.WhitelistNames)
				event.Whitelist.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			for (FString name : eventData.AttendeesNames)
				event.Attendees.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			faction->Events.Add(event);
		}

		// Army
		for (FArmyData armyData : data.ArmiesData) {
			TArray<ACitizen*> citizens;
			for (FString name : armyData.CitizensNames)
				citizens.Add(Cast<ACitizen>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

			Camera->ArmyManager->CreateArmy(data.Name, citizens, armyData.bGroup, true);
		}

		Camera->ConquestManager->DiplomacyComponent->SetFactionCulture(faction);
	}
}

void UDiplosimSaveGame::InitialiseGamemode(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, TArray<FActorSaveData> SavedData)
{
	FGamemodeData* gamemodeData = &ActorData.CameraData.GamemodeData;

	for (FWaveData waveData : gamemodeData->WaveData) {
		FWaveStruct wave;
		wave.SpawnLocations = waveData.SpawnLocations;

		for (auto element : waveData.DiedTo) {
			FDiedToStruct diedTo;
			diedTo.Actor = Camera->SaveGameComponent->GetSaveActorFromName(SavedData, element.Key);
			diedTo.Kills = element.Value;

			wave.DiedTo.Add(diedTo);
		}

		for (FString name : waveData.Threats)
			wave.Threats.Add(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name));

		wave.NumKilled = waveData.NumKilled;
		wave.EnemiesData = waveData.EnemiesData;

		Gamemode->WavesData.Add(wave);
	}

	for (FString name : gamemodeData->EnemyNames)
		Gamemode->Enemies.Add(Cast<AAI>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

	for (FString name : gamemodeData->SnakeNames)
		Gamemode->Snakes.Add(Cast<AAI>(Camera->SaveGameComponent->GetSaveActorFromName(SavedData, name)));

	if (!Gamemode->bSpawnedAllEnemies())
		Gamemode->SpawnAllEnemies();
}