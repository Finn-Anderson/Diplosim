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
	TArray<FActorSaveData> allNewActorData;

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

	int32 citizenNum = 0;

	for (AActor* actor : actors)
	{
		if (!IsValid(actor) || actor->IsPendingKillPending() || (Camera->BuildComponent->Buildings.Contains(actor) && !actor->IsA<ABroch>()))
			continue;

		FActorSaveData actorData;
		actorData.Name = actor->GetName();
		SetActorData("ActorClass", actor->GetClass(), actorData);
		SetActorData("ActorTransform", actor->GetActorTransform(), actorData);

		if (actor->IsA<AGrid>())
			SaveWorld(actorData, actor, Index, potentialWetActors);
		else if (actor->IsA<AResource>())
			SaveResource(Camera, actorData, actor);
		else if (actor->IsA<ACamera>()) {
			SaveCamera(actorData, actor, Index);

			SaveCitizenManager(actorData, actor, Index);

			SaveFactions(actorData, actor, Index);

			SaveGamemode(actorData, actor, Index);
		}
		else if (actor->IsA<AAI>()) {
			SaveAI(Camera, actorData, actor);

			if (actor->IsA<ACitizen>()) {
				SaveCitizen(Camera, actorData, actor);

				citizenNum++;
			}
		}
		else if (actor->IsA<ABuilding>())
			SaveBuilding(actorData, actor);
		else if (actor->IsA<AProjectile>())
			SaveProjectile(actorData, actor);
		else if (actor->IsA<AAISpawner>())
			SaveAISpawner(actorData, actor);

		if (Camera->Grid->AtmosphereComponent->GetFireComponent(actor) != nullptr)
			SetActorData("Fire0", -1, actorData);

		SaveTimers(Camera, actorData, actor);

		SaveComponents(actorData, actor);

		allNewActorData.Add(actorData);
	}

	Saves[Index].SavedActors = allNewActorData;
	Saves[Index].CitizenNum = citizenNum;
	LastTimeUpdated = FDateTime::Now();

	UGameplayStatics::AsyncSaveGameToSlot(this, ID, 0);
}

void UDiplosimSaveGame::SaveWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TArray<AActor*> PotentialWetActors)
{
	AGrid* grid = Cast<AGrid>(Actor);
	
	SetActorData("Size", grid->Size, ActorData);
	SetActorData("Chunks", grid->Chunks, ActorData);

	Saves[Index].Stream = grid->Camera->Stream;

	for (int32 i = 0; i < grid->LavaSpawnLocations.Num(); i++)
		SetActorData("Lava" + FString::FromInt(i), grid->LavaSpawnLocations[i], ActorData);

	auto bound = grid->GetMapBounds();
	for (TArray<FTileStruct>& row : grid->Storage) {
		for (FTileStruct& tile : row) {
			FString key = "Tile" + FString::FromInt(tile.X + tile.Y + bound);

			SetActorData(key + "Level", tile.Level, ActorData);
			SetActorData(key + "Fertility", tile.Fertility, ActorData);
			SetActorData(key + "X", tile.X, ActorData);
			SetActorData(key + "Y", tile.Y, ActorData);
			SetActorData(key + "Rotation", tile.Rotation, ActorData);
			SetActorData(key + "bRamp", tile.bRamp, ActorData);
			SetActorData(key + "bRiver", tile.bRiver, ActorData);
			SetActorData(key + "bEdge", tile.bEdge, ActorData);
			SetActorData(key + "bMineral", tile.bMineral, ActorData);
			SetActorData(key + "bUnique", tile.bUnique, ActorData);
		}
	}

	TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea };

	UCloudComponent* clouds = grid->AtmosphereComponent->Clouds;

	for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
		FHISMData data;

		int32 count = 0;
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

			FString key = hism->GetName() + FString::FromInt(count);
			SetActorData(key + "WetLocation", transform, ActorData);
			SetActorData(key + "WetValue", clouds->WetnessStruct[index].Value, ActorData);
			SetActorData(key + "WetIncrement", clouds->WetnessStruct[index].Increment, ActorData);

			count++;
		}

		data.CustomDataValues = hism->PerInstanceSMCustomData;

		SetActorData(hism->GetName(), data, ActorData);
	}

	FCalendarStruct calendar = grid->AtmosphereComponent->Calendar;
	SetActorData("CalendarPeriod", calendar.Period, ActorData);
	SetActorData("CalendarIndex", calendar.Index, ActorData);
	SetActorData("CalendarHour", calendar.Hour, ActorData);
	SetActorData("CalendarYear", calendar.Year, ActorData);

	SetActorData("bRedSun", grid->AtmosphereComponent->bRedSun, ActorData);
	SetActorData("WindRotation", grid->AtmosphereComponent->WindRotation, ActorData);
	SetActorData("SunRotation", grid->AtmosphereComponent->Sun->GetRelativeTransform(), ActorData);
	SetActorData("MoonRotation", grid->AtmosphereComponent->Moon->GetRelativeTransform(), ActorData);

	SetActorData("DisasterChance", grid->AtmosphereComponent->NaturalDisasterComponent->DisasterChance, ActorData);
	SetActorData("DisasterFrequency", grid->AtmosphereComponent->NaturalDisasterComponent->Frequency, ActorData);
	SetActorData("DisasterIntensity", grid->AtmosphereComponent->NaturalDisasterComponent->Intensity, ActorData);

	for (AActor* a : PotentialWetActors) {
		FWetnessStruct wetnessStruct;
		wetnessStruct.Actor = a;

		int32 index = clouds->WetnessStruct.Find(wetnessStruct);

		if (index == INDEX_NONE)
			continue;

		SetActorData(a->GetName() + "WetLocation", a->GetActorTransform(), ActorData);
		SetActorData(a->GetName() + "WetValue", clouds->WetnessStruct[index].Value, ActorData);
		SetActorData(a->GetName() + "WetIncrement", clouds->WetnessStruct[index].Increment, ActorData);
	}

	int32 cloudCount = 0;
	for (FCloudStruct cloud : grid->AtmosphereComponent->Clouds->Clouds) {
		SetActorData("CloudTransform" + FString::FromInt(cloudCount), cloud.HISMCloud->GetRelativeTransform(), ActorData);
		SetActorData("CloudDistance" + FString::FromInt(cloudCount), cloud.Distance, ActorData);
		SetActorData("CloudPercipitation" + FString::FromInt(cloudCount), cloud.Precipitation != nullptr, ActorData);
		SetActorData("CloudHide" + FString::FromInt(cloudCount), cloud.bHide, ActorData);
		SetActorData("CloudLightningFrequency" + FString::FromInt(cloudCount), cloud.lightningFrequency, ActorData);
		SetActorData("CloudLightningTimer" + FString::FromInt(cloudCount), cloud.lightningTimer, ActorData);

		cloudCount++;
	}
}

void UDiplosimSaveGame::SaveResource(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	AResource* resource = Cast<AResource>(Actor);
	FHISMData data;

	int32 fireCount = 0;
	for (int32 i = 0; i < resource->ResourceHISM->GetInstanceCount(); i++) {
		FTransform transform;
		resource->ResourceHISM->GetInstanceTransform(i, transform);

		data.Transforms.Add(transform);

		if (Camera->Grid->AtmosphereComponent->GetFireComponent(Actor, i) != nullptr)
			SetActorData("Fire" + fireCount, i, ActorData);

		fireCount++;
	}

	data.CustomDataValues = resource->ResourceHISM->PerInstanceSMCustomData;
	SetActorData("ResourceHISM", data, ActorData);

	int32 workerCount = 0;
	for (FWorkerStruct worker : resource->WorkerStruct) {
		int32 citizenCount = 0;
		for (ACitizen* citizen : worker.Citizens)
			SetActorData("Workers" + FString::FromInt(workerCount) + FString::FromInt(citizenCount), citizen->GetName(), ActorData);

		SetActorData("Workers" + FString::FromInt(workerCount), worker.Instance, ActorData);
	}
}

void UDiplosimSaveGame::SaveCamera(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);
	Saves[Index].CameraData.ColonyName = camera->ColonyName;

	Saves[Index].CameraData.ConstructionData.Empty();
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

	Saves[Index].CameraData.FactionData.Empty();

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
	data->CitizenData.Sexuality = citizen->BioComponent->Sexuality;
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
	data->CitizenData.DecayingHappiness = citizen->HappinessComponent->DecayingHappiness;
	data->CitizenData.Genetics = citizen->Genetics;
	data->CitizenData.bSleep = citizen->bSleep;
	data->CitizenData.HoursSleptToday = citizen->HoursSleptToday;

	data->MovementData.Transform.SetScale3D(FVector(1.0f));

	data->CitizenData.PersonalityTraits.Empty();
	for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
		data->CitizenData.PersonalityTraits.Add(personality->Trait);
}

void UDiplosimSaveGame::SaveBuilding(FActorSaveData& ActorData, AActor* Actor)
{
	ABuilding* building = Cast<ABuilding>(Actor);

	SetActorData("FactionName", building->FactionName, ActorData);
	SetActorData("Capacity", building->Capacity, ActorData);

	SetActorData("SeedNum", building->SeedNum, ActorData);
	SetActorData("Tier", building->Tier, ActorData);

	SetActorData("Red", building->ChosenColour.R, ActorData);
	SetActorData("Green", building->ChosenColour.G, ActorData);
	SetActorData("Blue", building->ChosenColour.B, ActorData);

	for (int32 i = 0; i < building->TargetList.Num(); i++) {
		SetActorData("TargetResource" + FString::FromInt(i), building->TargetList[i].Resource.Get(), ActorData);
		SetActorData("TargetAmount" + FString::FromInt(i), building->TargetList[i].Amount, ActorData);
		SetActorData("TargetStored" + FString::FromInt(i), building->TargetList[i].Stored, ActorData);
		SetActorData("TargetUse" + FString::FromInt(i), building->TargetList[i].Use, ActorData);
	}

	for (int32 i = 0; i < building->Storage.Num(); i++) {
		SetActorData("StorageResource" + FString::FromInt(i), building->Storage[i].Resource.Get(), ActorData);
		SetActorData("StorageAmount" + FString::FromInt(i), building->Storage[i].Amount, ActorData);
		SetActorData("StorageStored" + FString::FromInt(i), building->Storage[i].Stored, ActorData);
		SetActorData("StorageUse" + FString::FromInt(i), building->Storage[i].Use, ActorData);
	}

	for (int32 i = 0; i < building->Basket.Num(); i++) {
		SetActorData("BasketID" + FString::FromInt(i), building->Basket[i].ID.ToString(), ActorData);
		SetActorData("BasketResource" + FString::FromInt(i), building->Basket[i].Item.Resource.Get(), ActorData);
		SetActorData("BasketAmount" + FString::FromInt(i), building->Basket[i].Item.Amount, ActorData);
		SetActorData("BasketStored" + FString::FromInt(i), building->Basket[i].Item.Stored, ActorData);
		SetActorData("BasketUse" + FString::FromInt(i), building->Basket[i].Item.Use, ActorData);
	}

	double* time = building->Camera->Grid->AIVisualiser->DestructingActors.Find(building);
	if (time != nullptr)
		SetActorData("DeathTime", *time, ActorData);

	SetActorData("bOperate", building->bOperate, ActorData);

	for (int32 i = 0; i < building->Occupied.Num(); i++) {
		if (IsValid(building->Occupied[i].Citizen)) {
			SetActorData("OccupiedCitizen" + FString::FromInt(i), building->Occupied[i].Citizen->GetName(), ActorData);

			for (int32 j = 0; j < building->Occupied[i].Visitors.Num(); j++)
				SetActorData("OccupiedVisitor" + FString::FromInt(i) + FString::FromInt(j), building->Occupied[i].Visitors[j]->GetName(), ActorData);
		}

		SetActorData("OccupiedAmount" + FString::FromInt(i), building->Occupied[i].Amount, ActorData);
		SetActorData("OccupiedbBlocked" + FString::FromInt(i), building->Occupied[i].bBlocked, ActorData);

		int32 workHoursCount = 0;
		for (auto element : building->Occupied[i].WorkHours) {
			SetActorData("WorkHourKey" + FString::FromInt(workHoursCount), element.Key, ActorData);
			SetActorData("WorkHourValue" + FString::FromInt(workHoursCount), (uint8)element.Value, ActorData);
			workHoursCount++;
		}
	}
}

void UDiplosimSaveGame::SaveProjectile(FActorSaveData& ActorData, AActor* Actor)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);

	SetActorData("ProjectileOwner", projectile->GetOwner()->GetName(), ActorData);
	SetActorData("ProjectileVelocity", projectile->ProjectileMovementComponent->Velocity, ActorData);
}

void UDiplosimSaveGame::SaveAISpawner(FActorSaveData& ActorData, AActor* Actor)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);

	SetActorData("Red", spawner->Colour.R, ActorData);
	SetActorData("Green", spawner->Colour.G, ActorData);
	SetActorData("Blue", spawner->Colour.B, ActorData);
	SetActorData("IncrementSpawned", spawner->IncrementSpawned, ActorData);
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
		}

		ActorData.SavedTimers.Add(timer);
	}
}

void UDiplosimSaveGame::SaveComponents(FActorSaveData& ActorData, AActor* Actor)
{
	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp)
		SetActorData("Health", healthComp->GetHealth(), ActorData);

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		if (*attackComp->ProjectileClass)
			SetActorData("ProjectileClass", attackComp->ProjectileClass->GetClass(), ActorData);

		SetActorData("AttackTimer", attackComp->AttackTimer, ActorData);
		SetActorData("bShowMercy", attackComp->bShowMercy, ActorData);

		for (int32 i = 0; i < attackComp->OverlappingEnemies.Num(); i++)
			SetActorData("Enemy" + FString::FromInt(i), attackComp->OverlappingEnemies[i]->GetName(), ActorData);
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

	TMap<FString, FActorSaveData*> aiToName;
	TMap<FString, FActorSaveData> wetNames;

	for (FActorSaveData& actorData : Saves[Index].SavedActors) {
		AActor* actor = nullptr;
		FTransform transform = GetActorData<FTransform>("ActorTransform", actorData);

		UGameplayStatics::GetAllActorsOfClass(Camera->GetWorld(), GetActorData<UClass*>("ActorClass", actorData), foundActors);

		if (foundActors.Num() == 1 && (foundActors[0]->IsA<ACamera>() || foundActors[0]->IsA<AGrid>() || foundActors[0]->IsA<AResource>())) {
			actor = foundActors[0];

			actor->SetActorTransform(transform);
		}
		else {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = Camera->GetWorld()->SpawnActor<AActor>(GetActorData<UClass*>("ActorClass", actorData), transform, params);
		}

		if (actor->IsA<AGrid>())
			LoadWorld(actorData, actor, Index, wetNames);
		else if (actor->IsA<AResource>())
			LoadResource(actorData, actor);
		else if (actor->IsA<AEggBasket>())
			LoadEggBasket(Camera, actorData, actor);
		else if (actor->IsA<ACamera>()) {
			LoadCamera(actorData, actor, Index);

			LoadFactions(actorData, actor, Index);

			LoadGamemode(gamemode, actorData, actor, Index);
		}
		else if (actor->IsA<AAI>()) {
			LoadAI(Camera, gamemode, actorData, actor, Index, aiToName);

			if (actor->IsA<ACitizen>())
				LoadCitizen(Camera, actorData, actor, Index);
		}
		else if (actor->IsA<ABuilding>())
			LoadBuilding(Camera, actorData, actor, Index, aiToName);
		else if (actor->IsA<AProjectile>())
			LoadProjectile(actorData, actor);
		else if (actor->IsA<AAISpawner>())
			LoadAISpawner(gamemode, actorData, actor);

		int32 fireCount = 0;
		while (true) {
			if (!actorData.Numbers.Contains("Fire" + FString::FromInt(fireCount)))
				break;

			Camera->Grid->AtmosphereComponent->SetOnFire(actor, GetActorData<int32>("Fire" + FString::FromInt(fireCount), actorData), true);
		}

		if (actorData.Transforms.Contains(actorData.Name + "WetLocation"))
			wetNames.Add(actorData.Name, actorData);

		LoadComponents(actorData, actor);

		actorData.Actor = actor;
	}

	for (FActorSaveData actorData : Saves[Index].SavedActors) {
		LoadTimers(Camera, Index, actorData, Saves[Index].SavedActors);

		for (FActorSaveData savedData : Saves[Index].SavedActors) {
			LoadOverlappingEnemies(Camera, actorData, actorData.Actor, savedData);

			InitialiseObjects(Camera, gamemode, actorData, savedData, Index);
		}
	}

	for (auto element : wetNames)
		Camera->Grid->AtmosphereComponent->Clouds->RainCollisionHandler(GetActorData<FVector>(element.Key + "WetLocation", element.Value), GetActorData<float>(element.Key + "WetValue", element.Value), GetActorData<float>(element.Key + "WetIncrement", element.Value));
}

void UDiplosimSaveGame::LoadWorld(FActorSaveData& ActorData, AActor* Actor, int32 Index, TMap<FString, FActorSaveData>& WetNames)
{
	AGrid* grid = Cast<AGrid>(Actor);

	grid->Size = GetActorData<int32>("Size", ActorData);
	grid->Chunks = GetActorData<int32>("Chunks", ActorData);

	grid->Camera->Stream = Saves[Index].Stream;

	int32 lavaCount = 0;
	while (true) {
		FString key = "lava" + FString::FromInt(lavaCount);

		if (!ActorData.Transforms.Contains(key))
			break;

		grid->LavaSpawnLocations.Add(GetActorData<FVector>(key, ActorData));
		lavaCount++;
	}

	grid->Clear();

	grid->InitialiseStorage();
	auto bound = grid->GetMapBounds();

	int32 tileCount = 0;
	while (true) {
		FString key = "Tile" + FString::FromInt(tileCount);

		if (!ActorData.Numbers.Contains(key + "Level"))
			break;

		int32 x = GetActorData<int32>(key + "X", ActorData);
		int32 y = GetActorData<int32>(key + "Y", ActorData);

		FTileStruct* tile = &grid->Storage[x + (bound / 2)][y + (bound / 2)];
		tile->Level = GetActorData<int32>(key + "Level", ActorData);
		tile->Fertility = GetActorData<int32>(key + "Fertility", ActorData);
		tile->X = x;
		tile->Y = y;
		tile->Rotation = GetActorData<FQuat>(key + "Rotation", ActorData);
		tile->bRamp = GetActorData<bool>(key + "bRamp", ActorData);
		tile->bRiver = GetActorData<bool>(key + "bRiver", ActorData);
		tile->bEdge = GetActorData<bool>(key + "bEdge", ActorData);
		tile->bMineral = GetActorData<bool>(key + "bMineral", ActorData);
		tile->bUnique = GetActorData<bool>(key + "bUnique", ActorData);

		tileCount++;
	}

	TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea };

	int32 wetnessCount = 0;
	for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
		FHISMData data = GetActorData<FHISMData>(hism->GetName(), ActorData);

		hism->AddInstances(data.Transforms, false, true, true);
		hism->PerInstanceSMCustomData = data.CustomDataValues;
		hism->BuildTreeIfOutdated(true, true);

		hism->PartialNavigationUpdates(data.Transforms);

		while (true) {
			FString key = hism->GetName() + FString::FromInt(wetnessCount);

			if (!ActorData.Transforms.Contains(key + "WetLocation"))
				break;

			WetNames.Add(key, ActorData);

			wetnessCount++;
		}
	}

	FCalendarStruct& calendar = grid->AtmosphereComponent->Calendar;
	calendar.Period = GetActorData<FString>("CalendarPeriod", ActorData);
	calendar.Index = GetActorData<int32>("CalendarIndex", ActorData);
	calendar.Hour = GetActorData<int32>("CalendarHour", ActorData);
	calendar.Year = GetActorData<int32>("CalendarYear", ActorData);

	grid->AtmosphereComponent->bRedSun = GetActorData<bool>("bRedSun", ActorData);
	grid->AtmosphereComponent->WindRotation = GetActorData<FRotator>("WindRotation", ActorData);
	grid->AtmosphereComponent->Sun->SetRelativeRotation(GetActorData<FRotator>("SunRotation", ActorData));
	grid->AtmosphereComponent->Moon->SetRelativeRotation(GetActorData<FRotator>("MoonRotation", ActorData));

	grid->AtmosphereComponent->NaturalDisasterComponent->DisasterChance = GetActorData<float>("DisasterChance", ActorData);
	grid->AtmosphereComponent->NaturalDisasterComponent->Frequency = GetActorData<float>("DisasterFrequency", ActorData);
	grid->AtmosphereComponent->NaturalDisasterComponent->Intensity = GetActorData<float>("DisasterIntensity", ActorData);

	if (grid->AtmosphereComponent->bRedSun)
		grid->AtmosphereComponent->NaturalDisasterComponent->AlterSunGradually(0.15f, -1.00f);

	grid->AtmosphereComponent->Clouds->ProcessRainEffect.Empty();
	grid->AtmosphereComponent->Clouds->WetnessStruct.Empty();

	int32 cloudCount = 0;
	while (true) {
		if (!ActorData.Transforms.Contains("CloudTransform" + FString::FromInt(cloudCount)))
			break;

		FCloudStruct cloudStruct = grid->AtmosphereComponent->Clouds->CreateCloud(GetActorData<FTransform>("CloudTransform" + FString::FromInt(cloudCount), ActorData), GetActorData<bool>("CloudPercipitation" + FString::FromInt(cloudCount), ActorData) ? 100 : 0);
		cloudStruct.Distance = GetActorData<double>("CloudDistance" + FString::FromInt(cloudCount), ActorData);
		cloudStruct.bHide = GetActorData<bool>("CloudHide" + FString::FromInt(cloudCount), ActorData);
		cloudStruct.lightningFrequency = GetActorData<float>("CloudLightningFrequency" + FString::FromInt(cloudCount), ActorData);
		cloudStruct.lightningTimer = GetActorData<float>("CloudLightningTimer" + FString::FromInt(cloudCount), ActorData);

		cloudCount++;
	}

	grid->SetupEnvironment(true);
	grid->AIVisualiser->ResetToDefaultValues();
}

void UDiplosimSaveGame::LoadResource(FActorSaveData& ActorData, AActor* Actor)
{
	AResource* resource = Cast<AResource>(Actor);

	FHISMData data = GetActorData<FHISMData>("ResourceHISM", ActorData);

	int32 workerCount = 0;
	while (true) {
		if (!ActorData.Numbers.Contains("Workers" + FString::FromInt(workerCount)))
			break;

		FWorkerStruct workerStruct;
		workerStruct.Instance = GetActorData<int32>("Workers" + FString::FromInt(workerCount), ActorData);
		resource->WorkerStruct.Add(workerStruct);

		workerCount++;
	}

	resource->ResourceHISM->AddInstances(data.Transforms, false, true, true);
	resource->ResourceHISM->PerInstanceSMCustomData = data.CustomDataValues;
	resource->ResourceHISM->BuildTreeIfOutdated(true, true);
}

void UDiplosimSaveGame::LoadEggBasket(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor)
{
	AEggBasket* eggBasket = Cast<AEggBasket>(Actor);

	FTileStruct* tile = Camera->Grid->GetTileFromLocation(GetActorData<FVector>("ActorTransform", ActorData));

	eggBasket->Grid = Camera->Grid;
	eggBasket->Tile = tile;

	Camera->Grid->ResourceTiles.RemoveSingle(tile);
}

void UDiplosimSaveGame::LoadCamera(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);
	camera->ColonyName = Saves[Index].CameraData.ColonyName;

	camera->Start = false;
	camera->Cancel();

	camera->ClearPopupUI();
	camera->SetInteractStatus(camera->WidgetComponent->GetAttachmentRootActor(), false);

	camera->Detach();
	camera->MovementComponent->TargetLength = 3000.0f;
	camera->MovementComponent->MovementLocation = GetActorData<FVector>("ActorTransform", ActorData);

	camera->ConstructionManager->Construction.Empty();

	camera->CitizenManager->IssuePensionHour = Saves[Index].CameraData.CitizenManagerData.IssuePensionHour;
}

void UDiplosimSaveGame::LoadFactions(FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);

	camera->ConquestManager->Factions.Empty();
	for (FFactionData data : Saves[Index].CameraData.FactionData) {
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

void UDiplosimSaveGame::LoadGamemode(ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, AActor* Actor, int32 Index)
{
	ACamera* camera = Cast<ACamera>(Actor);

	FGamemodeData* gamemodeData = &Saves[Index].CameraData.GamemodeData;

	for (FWaveData waveData : gamemodeData->WaveData) {
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

	Gamemode->bOngoingRaid = gamemodeData->bOngoingRaid;
	camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, gamemodeData->CrystalOpacity);
	Gamemode->TargetOpacity = gamemodeData->TargetOpacity;

	if (gamemodeData->CrystalOpacity != Gamemode->TargetOpacity)
		Gamemode->SetActorTickEnabled(true);
}

void UDiplosimSaveGame::LoadAI(ACamera* Camera, ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, AActor* Actor, int32 Index, TMap<FString, FActorSaveData*>& AIToName)
{
	AAI* ai = Cast<AAI>(Actor);

	FAIData* data = &ActorData.AIData;

	AIToName.Add(ActorData.Name, &ActorData);

	UInstancedStaticMeshComponent* ism;

	if (data->FactionName == "") {
		if (data->bSnake)
			ism = Camera->Grid->AIVisualiser->HISMSnake;
		else
			ism = Camera->Grid->AIVisualiser->HISMEnemy;
	}
	else {
		FFactionStruct* faction = Camera->ConquestManager->GetFaction(data->FactionName);

		if (ai->IsA<AClone>()) {
			faction->Clones.Add(ai);
			ism = Camera->Grid->AIVisualiser->HISMClone;
		}
		else if (data->CitizenData.bRebel) {
			faction->Rebels.Add(Cast<ACitizen>(ai));
			ism = Camera->Grid->AIVisualiser->HISMRebel;
		}
		else {
			faction->Citizens.Add(Cast<ACitizen>(ai));
			ism = Camera->Grid->AIVisualiser->HISMCitizen;
		}
	}

	ai->Colour = data->Colour;

	ai->MovementComponent->Points = data->MovementData.Points;
	ai->MovementComponent->CurrentAnim = data->MovementData.CurrentAnim;
	ai->MovementComponent->LastUpdatedTime = data->MovementData.LastUpdatedTime;

	Camera->Grid->AIVisualiser->AddInstance(ai, ism, data->MovementData.Transform);

	if (!ai->IsA<AEnemy>())
		return; 
	
	FGamemodeData* gmData = &Saves[Index].CameraData.GamemodeData;

	if (gmData->EnemyNames.Contains(ActorData.Name))
		Gamemode->Enemies.Add(ai);

	if (gmData->SnakeNames.Contains(ActorData.Name))
		Gamemode->Snakes.Add(ai);
}

void UDiplosimSaveGame::LoadCitizen(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index)
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
	citizen->BioComponent->Sexuality = data->CitizenData.Sexuality;
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
	citizen->HappinessComponent->DecayingHappiness = data->CitizenData.DecayingHappiness;
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

	FCitizenManagerData cmData = Saves[Index].CameraData.CitizenManagerData;

	if (cmData.InfectibleNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Infectible.Add(citizen);

	if (cmData.InfectedNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Infected.Add(citizen);

	if (cmData.InjuredNames.Contains(ActorData.Name))
		Camera->DiseaseManager->Injured.Add(citizen);

	FFactionStruct* faction = Camera->ConquestManager->GetFaction(data->FactionName);
	for (FFactionData factionData : Saves[Index].CameraData.FactionData) {
		if (factionData.Name != data->FactionName || !factionData.PoliticsData.RepresentativesNames.Contains(ActorData.Name))
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

void UDiplosimSaveGame::LoadBuilding(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, int32 Index, TMap<FString, FActorSaveData*>& AIToName)
{
	ABuilding* building = Cast<ABuilding>(Actor);

	building->ToggleDecalComponentVisibility(false);
	building->BuildingMesh->SetCanEverAffectNavigation(true);
	building->BuildingMesh->bReceivesDecals = true;

	building->FactionName = GetActorData<FString>("FactionName", ActorData);

	bool bBuilt = true;
	for (FConstructionData constructionData : Saves[Index].CameraData.ConstructionData) {
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

	building->Capacity = GetActorData<int32>("Capacity", ActorData);
	building->StoreSocketLocations();

	building->SetSeed(GetActorData<int32>("SeedNum", ActorData));
	building->SetTier(GetActorData<int32>("Tier", ActorData));

	building->SetBuildingColour(GetActorData<float>("Red", ActorData), GetActorData<float>("Green", ActorData), GetActorData<float>("Blue", ActorData));

	int32 count = 0;
	while (true) {
		if (!ActorData.Classes.Contains("TargetResource" + FString::FromInt(count)))
			break;

		FItemStruct item;
		item.Resource = GetActorData<UClass*>("TargetResource" + FString::FromInt(count), ActorData);
		item.Amount = GetActorData<int32>("TargetAmount" + FString::FromInt(count), ActorData);
		item.Stored = GetActorData<int32>("TargetStored" + FString::FromInt(count), ActorData);
		item.Use = GetActorData<int32>("TargetUse" + FString::FromInt(count), ActorData);
		building->TargetList.Add(item);

		count++;
	}

	count = 0;
	while (true) {
		if (!ActorData.Classes.Contains("StorageResource" + FString::FromInt(count)))
			break;

		FItemStruct item;
		item.Resource = GetActorData<UClass*>("StorageResource" + FString::FromInt(count), ActorData);
		item.Amount = GetActorData<int32>("StorageAmount" + FString::FromInt(count), ActorData);
		item.Stored = GetActorData<int32>("StorageStored" + FString::FromInt(count), ActorData);
		item.Use = GetActorData<int32>("StorageUse" + FString::FromInt(count), ActorData);
		building->Storage.Add(item);

		count++;
	}

	count = 0;
	while (true) {
		if (!ActorData.Classes.Contains("BasketResource" + FString::FromInt(count)))
			break;

		FBasketStruct basket;
		basket.ID = FGuid(GetActorData<FString>("BasketID" + FString::FromInt(count), ActorData));
		basket.Item.Resource = GetActorData<UClass*>("BasketResource" + FString::FromInt(count), ActorData);
		basket.Item.Amount = GetActorData<int32>("BasketAmount" + FString::FromInt(count), ActorData);
		basket.Item.Stored = GetActorData<int32>("BasketStored" + FString::FromInt(count), ActorData);
		basket.Item.Use = GetActorData<int32>("BasketUse" + FString::FromInt(count), ActorData);
		building->Basket.Add(basket);

		count++;
	}

	if (ActorData.Numbers.Contains("DeathTime"))
		building->Camera->Grid->AIVisualiser->DestructingActors.Add(building, GetActorData<double>("DeathTime", ActorData));

	building->bOperate = GetActorData<bool>("bOperate", ActorData);

	building->Occupied.Empty();
	count = 0;
	while (true) {
		if (!ActorData.Classes.Contains("OccupiedAmount" + FString::FromInt(count)))
			break;

		FCapacityStruct capacityStruct;
		if (ActorData.Classes.Contains("OccupiedCitizen" + FString::FromInt(count))) {
			FActorSaveData* aiData = *AIToName.Find(GetActorData<FString>("OccupiedCitizen" + FString::FromInt(count), ActorData));
			capacityStruct.Citizen = Cast<ACitizen>(aiData->Actor);
			Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, aiData, false);

			int32 visitorCount = 0;
			while (true) {
				if (!ActorData.Classes.Contains("OccupiedVisitor" + FString::FromInt(count) + FString::FromInt(visitorCount)))
					break;

				FActorSaveData* visitorData = *AIToName.Find(GetActorData<FString>("OccupiedVisitor" + FString::FromInt(count) + FString::FromInt(visitorCount), ActorData));
				capacityStruct.Visitors.Add(Cast<ACitizen>(visitorData->Actor));
				Camera->SaveGameComponent->SetupCitizenBuilding(ActorData.Name, building, visitorData, true);
			}
		}

		capacityStruct.Amount = GetActorData<int32>("OccupiedAmount" + FString::FromInt(count), ActorData);
		capacityStruct.bBlocked = GetActorData<bool>("OccupiedBlocked" + FString::FromInt(count), ActorData);

		TMap<int32, EWorkType> workHours;
		int32 workHoursCount = 0;
		while (true) {
			if (!ActorData.Classes.Contains("WorkHourKey" + FString::FromInt(workHoursCount)))
				break;

			workHours.Add(GetActorData<int32>("WorkHourKey" + FString::FromInt(workHoursCount), ActorData), EWorkType(GetActorData<uint8>("WorkHourValue" + FString::FromInt(workHoursCount), ActorData)));
		}
		capacityStruct.WorkHours = workHours;

		building->Occupied.Add(capacityStruct);
		count++;
	}
}

void UDiplosimSaveGame::LoadProjectile(FActorSaveData& ActorData, AActor* Actor)
{
	AProjectile* projectile = Cast<AProjectile>(Actor);

	projectile->ProjectileMovementComponent->Velocity = GetActorData<FVector>("ProjectileVelocity", ActorData);
}

void UDiplosimSaveGame::LoadAISpawner(ADiplosimGameModeBase* Gamemode, FActorSaveData& ActorData, AActor* Actor)
{
	AAISpawner* spawner = Cast<AAISpawner>(Actor);

	spawner->Colour = FLinearColor(GetActorData<float>("Red", ActorData), GetActorData<float>("Green", ActorData), GetActorData<float>("Blue", ActorData));
	spawner->IncrementSpawned = GetActorData<int32>("IncrementSpawned", ActorData);

	Gamemode->SnakeSpawners.Add(spawner);
}

void UDiplosimSaveGame::LoadComponents(FActorSaveData& ActorData, AActor* Actor)
{
	UHealthComponent* healthComp = Actor->FindComponentByClass<UHealthComponent>();
	if (healthComp) {
		healthComp->Health = GetActorData<int32>("Health", ActorData);

		if (healthComp->GetHealth() == 0 && !(Actor->IsA<ABuilding>() && Cast<ABuilding>(Actor)->FactionName == ""))
			healthComp->Death(nullptr);
	}

	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	if (attackComp) {
		if (ActorData.Classes.Contains("ProjectileClass"))
			attackComp->ProjectileClass = GetActorData<UClass*>("ProjectileClass", ActorData);

		attackComp->AttackTimer = GetActorData<float>("AttackTimer", ActorData);
		attackComp->bShowMercy = GetActorData<bool>("bShowMercy", ActorData);

		if (!attackComp->OverlappingEnemies.IsEmpty())
			attackComp->SetComponentTickEnabled(true);
	}
}

void UDiplosimSaveGame::LoadOverlappingEnemies(ACamera* Camera, FActorSaveData& ActorData, AActor* Actor, FActorSaveData SavedData)
{
	UAttackComponent* attackComp = Actor->FindComponentByClass<UAttackComponent>();
	int32 enemyCount = 0;

	while (attackComp) {
		FString key = "Enemy" + FString::FromInt(enemyCount);
		if (!ActorData.Strings.Contains(key))
			break;

		if (SavedData.Name != GetActorData<FString>(key, ActorData))
			continue;

		attackComp->OverlappingEnemies.Add(SavedData.Actor);
	}
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
		if (SavedData.Name == GetActorData<FString>("ProjectileOwner", ActorData))
			Cast<AProjectile>(ActorData.Actor)->SpawnNiagaraSystems(SavedData.Actor);
	}
	else if (ActorData.Actor->IsA<AAI>()) {
		InitialiseAI(Camera, ActorData, SavedData);
	}
	else if (ActorData.Actor->IsA<ACamera>()) {
		ACamera* camera = Cast<ACamera>(ActorData.Actor);

		InitialiseConstructionManager(camera, ActorData, SavedData, Index);

		InitialiseCitizenManager(camera, ActorData, SavedData, Index);

		InitialiseFactions(camera, ActorData, SavedData, Index);

		InitialiseGamemode(camera, Gamemode, ActorData, SavedData, Index);
	}
	else if (ActorData.Actor->IsA<AResource>()) {
		InitialiseResources(Camera, ActorData, SavedData);
	}
}

void UDiplosimSaveGame::InitialiseAI(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData)
{
	AAI* ai = Cast<AAI>(ActorData.Actor);
	FAIMovementData* movementData = &ActorData.AIData.MovementData;
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

	if (SavedData.Name == movementData->ChosenBuildingName)
		ai->AIController->ChosenBuilding = Cast<ABuilding>(SavedData.Actor);

	if (SavedData.Name == movementData->ActorToLookAtName)
		ai->MovementComponent->ActorToLookAt = SavedData.Actor;

	if (SavedData.Name == movementData->ActorName)
		moveStruct.Actor = SavedData.Actor;

	if (SavedData.Name == movementData->LinkedPortalName)
		moveStruct.LinkedPortal = SavedData.Actor;

	if (SavedData.Name == movementData->UltimateGoalName)
		moveStruct.UltimateGoal = SavedData.Actor;

	if (ActorData.Actor->IsA<ACitizen>())
		InitialiseCitizen(Camera, ActorData, SavedData);

	moveStruct.Instance = ActorData.AIData.MovementData.Instance;
	moveStruct.Location = ActorData.AIData.MovementData.Location;

	ai->AIController->MoveRequest = moveStruct;
}

void UDiplosimSaveGame::InitialiseCitizen(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData)
{
	ACitizen* citizen = Cast<ACitizen>(ActorData.Actor);
	FCitizenData* citizenData = &ActorData.AIData.CitizenData;

	if (SavedData.Name == citizenData->MothersName)
		citizen->BioComponent->Mother = Cast<ACitizen>(SavedData.Actor);

	if (SavedData.Name == citizenData->FathersName)
		citizen->BioComponent->Father = Cast<ACitizen>(SavedData.Actor);

	if (SavedData.Name == citizenData->PartnersName)
		citizen->BioComponent->Partner = Cast<ACitizen>(SavedData.Actor);

	if (citizenData->ChildrensNames.Contains(SavedData.Name))
		citizen->BioComponent->Children.Add(Cast<ACitizen>(SavedData.Actor));

	if (citizenData->SiblingsNames.Contains(SavedData.Name))
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

void UDiplosimSaveGame::InitialiseResources(ACamera* Camera, FActorSaveData& ActorData, FActorSaveData SavedData)
{
	AResource* resource = Cast<AResource>(ActorData.Actor);

	for (int32 i = 0; i < resource->WorkerStruct.Num(); i++) {
		FWorkerStruct& workerStruct = resource->WorkerStruct[i];
		int32 citizenCount = 0;

		while (true) {
			if (!ActorData.Strings.Contains("Workers" + FString::FromInt(i) + FString::FromInt(citizenCount)))
				break;

			if (GetActorData<FString>("Workers" + FString::FromInt(i) + FString::FromInt(citizenCount), ActorData) != SavedData.Name)
				continue;

			workerStruct.Citizens.Add(Cast<ACitizen>(SavedData.Actor));
			citizenCount++;
		}
	}
}