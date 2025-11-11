#include "Player/Components/SaveGameComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/SaveGame.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"

#include "Player/Camera.h"
#include "Player/Managers/CitizenManager.h"
#include "Player/Managers/ConstructionManager.h"
#include "Player/Components/CameraMovementComponent.h"
#include "Player/Components/BuildComponent.h"
#include "Universal/DiplosimUserSettings.h"
#include "Universal/EggBasket.h"
#include "Universal/HealthComponent.h"
#include "Map/Grid.h"
#include "Map/Atmosphere/AtmosphereComponent.h"
#include "Map/Atmosphere/NaturalDisasterComponent.h"
#include "Map/Atmosphere/Clouds.h"
#include "Map/AIVisualiser.h"
#include "AI/Citizen.h"
#include "AI/Clone.h"
#include "AI/AttackComponent.h"
#include "AI/Projectile.h"
#include "AI/AIMovementComponent.h"
#include "AI/DiplosimAIController.h"
#include "Buildings/Misc/Broch.h"
#include "Buildings/Work/Service/Builder.h"

USaveGameComponent::USaveGameComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CurrentID = "";
	CurrentSaveGame = nullptr;

	Camera = nullptr;
}

void USaveGameComponent::StartNewSave()
{
	CurrentSaveGame = Cast<UDiplosimSaveGame>(UGameplayStatics::CreateSaveGameObject(UDiplosimSaveGame::StaticClass()));
	CurrentID = FGuid::NewGuid().ToString();

	SaveGameSave("", true);
}

void USaveGameComponent::SaveGameSave(FString Name, bool bAutosave)
{
	TArray<FActorSaveData> allNewActorData;

	TArray<AActor*> foundActors;

	TArray<AActor*> actors = { Camera, Camera->Grid };
	TArray<AActor*> potentialWetActors;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AResource::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);
	potentialWetActors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProjectile::StaticClass(), foundActors);
	actors.Append(foundActors);

	for (AActor* actor : actors)
	{
		if (!IsValid(actor) || actor->IsPendingKillPending())
			continue;

		FActorSaveData actorData;
		actorData.Name = actor->GetName();
		actorData.Class = actor->GetClass();
		actorData.Transform = actor->GetActorTransform();

		if (actor->IsA<AGrid>()) {
			AGrid* grid = Cast<AGrid>(actor);

			actorData.WorldSaveData.Size = grid->Size;
			actorData.WorldSaveData.Chunks = grid->Chunks;

			actorData.WorldSaveData.Stream = grid->Stream;

			actorData.WorldSaveData.LavaSpawnLocations = grid->LavaSpawnLocations;

			actorData.WorldSaveData.Tiles.Empty();

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

					actorData.WorldSaveData.Tiles.Add(t);
				}
			}

			actorData.WorldSaveData.HISMData.Empty();
			actorData.WorldSaveData.CloudsData.WetnessData.Empty();
			actorData.WorldSaveData.CloudsData.CloudData.Empty();

			TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea, grid->HISMWall };

			UCloudComponent* clouds = grid->AtmosphereComponent->Clouds;

			for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
				FHISMData data;
				data.Name = hism->GetName();

				for (int32 i = 0; i < hism->GetInstanceCount(); i++) {
					FTransform transform;
					hism->GetInstanceTransform(i, transform);

					data.Transforms.Add(transform);

					if (hism == grid->HISMLava || hism == grid->HISMRiver || hism == grid->HISMWall)
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

					actorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
				}
				
				data.CustomDataValues = hism->PerInstanceSMCustomData;

				actorData.WorldSaveData.HISMData.Add(data);
			}

			actorData.WorldSaveData.AtmosphereData.Calendar = grid->AtmosphereComponent->Calendar;
			actorData.WorldSaveData.AtmosphereData.bRedSun = grid->AtmosphereComponent->bRedSun;
			actorData.WorldSaveData.AtmosphereData.WindRotation = grid->AtmosphereComponent->WindRotation;

			actorData.WorldSaveData.NaturalDisasterData.bDisasterChance = grid->AtmosphereComponent->NaturalDisasterComponent->bDisasterChance;
			actorData.WorldSaveData.NaturalDisasterData.Frequency = grid->AtmosphereComponent->NaturalDisasterComponent->Frequency;
			actorData.WorldSaveData.NaturalDisasterData.Intensity = grid->AtmosphereComponent->NaturalDisasterComponent->Intensity;

			for (AActor* a : potentialWetActors) {
				FWetnessStruct wetnessStruct;
				wetnessStruct.Actor = a;

				int32 index = clouds->WetnessStruct.Find(wetnessStruct);

				if (index == INDEX_NONE)
					continue;

				FWetnessData wetnessData;
				wetnessData.Location = a->GetActorLocation();
				wetnessData.Value = clouds->WetnessStruct[index].Value;
				wetnessData.Increment = clouds->WetnessStruct[index].Increment;

				actorData.WorldSaveData.CloudsData.WetnessData.Add(wetnessData);
			}

			for (FCloudStruct cloud : grid->AtmosphereComponent->Clouds->Clouds) {
				FCloudData data;
				data.Transform = cloud.HISMCloud->GetRelativeTransform();
				data.Distance = cloud.Distance;
				data.bPrecipitation = cloud.Precipitation != nullptr;
				data.bHide = cloud.bHide;
				data.lightningFrequency = cloud.lightningFrequency;
				data.lightningTimer = cloud.lightningTimer;

				actorData.WorldSaveData.CloudsData.CloudData.Add(data);
			}
		}
		else if (actor->IsA<AResource>()) {
			AResource* resource = Cast<AResource>(actor);

			actorData.ResourceData.HISMData.Name = resource->ResourceHISM->GetName(); 
			
			actorData.ResourceData.HISMData.Transforms.Empty();
			for (int32 i = 0; i < resource->ResourceHISM->GetInstanceCount(); i++) {
				FTransform transform;
				resource->ResourceHISM->GetInstanceTransform(i, transform);

				actorData.ResourceData.HISMData.Transforms.Add(transform);
			}

			actorData.ResourceData.HISMData.CustomDataValues = resource->ResourceHISM->PerInstanceSMCustomData;

			actorData.ResourceData.Workers = resource->WorkerStruct;
		}
		else if (actor->IsA<ACamera>()) {
			ACamera* camera = Cast<ACamera>(actor);

			actorData.CameraData.ConstructionData.Empty();
			for (FConstructionStruct constructionStruct : camera->ConstructionManager->Construction) {
				FConstructionData data;
				data.BuildingName = constructionStruct.Building->GetName();
				data.BuildPercentage = constructionStruct.BuildPercentage;
				data.Status = constructionStruct.Status;

				if (IsValid(constructionStruct.Builder))
					data.BuilderName = constructionStruct.Builder->GetName();

				actorData.CameraData.ConstructionData.Add(data);
			}

			FCitizenManagerData cmData;

			for (FPersonality personality : camera->CitizenManager->Personalities) {
				FPersonalityData data;
				data.Trait = personality.Trait;

				for (ACitizen* citizen : personality.Citizens)
					data.CitizenNames.Add(citizen->GetName());

				cmData.PersonalitiesData.Add(data);
			}

			for (ACitizen* citizen : camera->CitizenManager->Infectible)
				cmData.InfectibleNames.Add(citizen->GetName());

			for (ACitizen* citizen : camera->CitizenManager->Infected)
				cmData.InfectedNames.Add(citizen->GetName());

			for (ACitizen* citizen : camera->CitizenManager->Injured)
				cmData.InjuredNames.Add(citizen->GetName());

			for (AAI* enemy : camera->CitizenManager->Enemies)
				cmData.EnemyNames.Add(enemy->GetName());

			cmData.IssuePensionHour = camera->CitizenManager->IssuePensionHour;

			actorData.CameraData.CitizenManagerData = cmData;

			actorData.CameraData.FactionData.Empty();
			for (FFactionStruct faction : camera->ConquestManager->Factions) {
				FFactionData data;
				data.Name = faction.Name;
				data.Flag = faction.Flag;
				data.FlagColour = faction.FlagColour;
				data.AtWar = faction.AtWar;
				data.Allies = faction.Allies;
				data.Happiness = faction.Happiness;
				data.WarFatigue = faction.WarFatigue;
				data.PartyInPower = faction.PartyInPower;
				data.LargestReligion = faction.LargestReligion;
				data.RebelCooldownTimer = faction.RebelCooldownTimer;
				data.ResearchStruct = faction.ResearchStruct;
				data.ResearchIndex = faction.ResearchIndex;
				data.PrayStruct = faction.PrayStruct;
				data.Resources = faction.Resources;
				data.AccessibleBuildLocations = faction.AccessibleBuildLocations;
				data.InaccessibleBuildLocations = faction.InaccessibleBuildLocations;
				data.RoadBuildLocations = faction.RoadBuildLocations;
				data.FailedBuild = faction.FailedBuild;

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

				data.ArmiesData.Empty();
				for (FArmyStruct army : faction.Armies) {
					FArmyData armyData;
					armyData.bGroup = army.bGroup;

					for (ACitizen* citizen : army.Citizens)
						armyData.CitizensNames.Add(citizen->GetName());

					data.ArmiesData.Add(armyData);
				}

				actorData.CameraData.FactionData.Add(data);
			}
			
			ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();
			FGamemodeData* gamemodeData = &actorData.CameraData.GamemodeData;

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
			
			gamemodeData->bOngoingRaid = gamemode->bOngoingRaid;
			gamemodeData->CrystalOpacity = camera->Grid->CrystalMesh->GetCustomPrimitiveData().Data[0];
			gamemodeData->TargetOpacity = gamemode->TargetOpacity;
		}
		else if (actor->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(actor);

			FAIData* data = &actorData.AIData;

			FFactionStruct* faction = Camera->ConquestManager->GetFaction("", ai);

			data->MovementData.Points = ai->MovementComponent->Points;
			data->MovementData.CurrentAnim = ai->MovementComponent->CurrentAnim;
			data->MovementData.LastUpdatedTime = ai->MovementComponent->LastUpdatedTime;
			data->MovementData.Transform = ai->MovementComponent->Transform;
			data->MovementData.ActorToLookAtName = ai->MovementComponent->ActorToLookAt->GetName();
			data->MovementData.TempPoints = ai->MovementComponent->TempPoints;
			data->MovementData.bSetPoints = ai->MovementComponent->bSetPoints;

			data->MovementData.ChosenBuildingName = ai->AIController->ChosenBuilding->GetName();
			data->MovementData.ActorName = ai->AIController->MoveRequest.GetGoalActor()->GetName();
			data->MovementData.LinkedPortalName = ai->AIController->MoveRequest.GetLinkedPortal()->GetName();
			data->MovementData.UltimateGoalName = ai->AIController->MoveRequest.GetUltimateGoalActor()->GetName();
			data->MovementData.Instance = ai->AIController->MoveRequest.GetGoalInstance();
			data->MovementData.Location = ai->AIController->MoveRequest.GetLocation();

			if (faction != nullptr) {
				data->FactionName = faction->Name;

				if (ai->IsA<ACitizen>()) {
					ACitizen* citizen = Cast<ACitizen>(ai);

					if (faction->Rebels.Contains(citizen))
						data->CitizenData.bRebel = true;
					
					data->CitizenData.EnterLocation = citizen->Building.EnterLocation;
				}
			}

			if (ai->IsA<ACitizen>()) {
				ACitizen* citizen = Cast<ACitizen>(ai);

				data->CitizenData.ResourceCarryClass = citizen->Carrying.Type->GetClass();
				data->CitizenData.CarryAmount = citizen->Carrying.Amount;

				if (citizen->BioStruct.Mother != nullptr)
					data->CitizenData.MothersName = citizen->BioStruct.Mother->GetName();

				if (citizen->BioStruct.Father != nullptr)
					data->CitizenData.FathersName = citizen->BioStruct.Father->GetName();

				if (citizen->BioStruct.Partner != nullptr)
					data->CitizenData.PartnersName = citizen->BioStruct.Partner->GetName();

				data->CitizenData.ChildrensNames.Empty();
				data->CitizenData.SiblingsNames.Empty();

				for (ACitizen* child : citizen->BioStruct.Children)
					data->CitizenData.ChildrensNames.Add(child->GetName());

				for (ACitizen* sibling : citizen->BioStruct.Siblings)
					data->CitizenData.SiblingsNames.Add(sibling->GetName());

				data->CitizenData.HoursTogetherWithPartner = citizen->BioStruct.HoursTogetherWithPartner;
				data->CitizenData.bMarried = citizen->BioStruct.bMarried;
				data->CitizenData.Sex = citizen->BioStruct.Sex;
				data->CitizenData.Age = citizen->BioStruct.Age;
				data->CitizenData.Name = citizen->BioStruct.Name;
				data->CitizenData.EducationLevel = citizen->BioStruct.EducationLevel;
				data->CitizenData.EducationProgress = citizen->BioStruct.EducationProgress;
				data->CitizenData.PaidForEducationLevel = citizen->BioStruct.PaidForEducationLevel;
				data->CitizenData.bAdopted = citizen->BioStruct.bAdopted;

				data->CitizenData.Spirituality = citizen->Spirituality;
				data->CitizenData.TimeOfAcquirement = citizen->TimeOfAcquirement;
				data->CitizenData.VoicePitch = citizen->VoicePitch;
				data->CitizenData.Balance = citizen->Balance;
				data->CitizenData.HoursWorked = citizen->HoursWorked;
				data->CitizenData.Hunger = citizen->Hunger;
				data->CitizenData.Energy = citizen->Energy;
				data->CitizenData.bGain = citizen->bGain;
				data->CitizenData.SpeedBeforeOld = citizen->SpeedBeforeOld;
				data->CitizenData.MaxHealthBeforeOld = citizen->MaxHealthBeforeOld;
				data->CitizenData.bHasBeenLeader = citizen->bHasBeenLeader;
				data->CitizenData.MassStatus = citizen->MassStatus;
				data->CitizenData.HealthIssues = citizen->HealthIssues;
				data->CitizenData.Happiness = citizen->Happiness;
				data->CitizenData.SadTimer = citizen->SadTimer;
				data->CitizenData.bHolliday = citizen->bHolliday;
				data->CitizenData.FestivalStatus = citizen->FestivalStatus;
				data->CitizenData.bConversing = citizen->bConversing;
				data->CitizenData.ConversationHappiness = citizen->ConversationHappiness;
				data->CitizenData.FamilyDeathHappiness = citizen->FamilyDeathHappiness;
				data->CitizenData.WitnessedDeathHappiness = citizen->WitnessedDeathHappiness;
				data->CitizenData.Genetics = citizen->Genetics;
				data->CitizenData.bSleep = citizen->bSleep;
				data->CitizenData.HoursSleptToday = citizen->HoursSleptToday;

				data->CitizenData.PersonalityTraits.Empty();
				for (FPersonality* personality : Camera->CitizenManager->GetCitizensPersonalities(citizen))
					data->CitizenData.PersonalityTraits.Add(personality->Trait);
			}
		}
		else if (actor->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(actor);

			if (Camera->BuildComponent->Buildings.Contains(building))
				continue;

			actorData.BuildingData.FactionName = building->FactionName;
			actorData.BuildingData.Capacity = building->Capacity;

			actorData.BuildingData.Seed = building->SeedNum;
			actorData.BuildingData.ChosenColour = building->ChosenColour;
			actorData.BuildingData.Tier = building->Tier;

			actorData.BuildingData.TargetList = building->TargetList;

			actorData.BuildingData.Storage = building->Storage;
			actorData.BuildingData.Basket = building->Basket;

			actorData.BuildingData.DeathTime = building->DeathTime;
			actorData.BuildingData.bOperate = building->bOperate;

			if (IsA<AWork>())
				actorData.BuildingData.WorkHours = Cast<AWork>(building)->WorkHours;

			TArray<FOccupantData> occData;

			for (FOccupantStruct occupant : building->Occupied) {
				FOccupantData occupantData;
				occupantData.OccupantName = occupant.Occupant->GetName();

				for (ACitizen* visitor : occupant.Visitors)
					occupantData.VisitorNames.Add(visitor->GetName());

				occData.Add(occupantData);
			}

			actorData.BuildingData.OccupantsData = occData;
		}
		else if (actor->IsA<AProjectile>()) {
			AProjectile* projectile = Cast<AProjectile>(actor);

			actorData.ProjectileData.OwnerName = projectile->GetOwner()->GetName();
			actorData.ProjectileData.Velocity = projectile->ProjectileMovementComponent->Velocity;
		}

		for (FTimerStruct timer : Camera->CitizenManager->Timers) {
			if (timer.Actor != actor)
				continue;

			for (FTimerParameterStruct param : timer.Parameters) {
				if (!IsValid(param.Object))
					continue;

				param.ObjectClass = param.Object->GetClass();
				param.ObjectName = param.Object->GetName();
			}

			actorData.SavedTimers.Add(timer);
		}

		UHealthComponent* healthComp = actor->FindComponentByClass<UHealthComponent>();
		if (healthComp)
			actorData.HealthData.Health = healthComp->Health;

		UAttackComponent* attackComp = actor->FindComponentByClass<UAttackComponent>();
		if (attackComp) {
			actorData.AttackData.ProjectileClass = attackComp->ProjectileClass;
			actorData.AttackData.AttackTimer = attackComp->AttackTimer;
			actorData.AttackData.bShowMercy = attackComp->bShowMercy;

			for (AActor* enemies : attackComp->OverlappingEnemies)
				actorData.AttackData.ActorNames.Add(enemies->GetName());
		}

		allNewActorData.Add(actorData);
	}

	if (Name == "") {
		CreateNewSaveStruct(Name, bAutosave, allNewActorData);

		if (bAutosave)
			CapAutosaves();
	}
	else {
		FSave s;
		s.SaveName = Name;

		int32 index = CurrentSaveGame->Saves.Find(s);

		if (index == INDEX_NONE) {
			CreateNewSaveStruct(Name, bAutosave, allNewActorData);
		}
		else {
			CurrentSaveGame->Saves[index].SavedActors = allNewActorData;
			CurrentSaveGame->Saves[index].bAutosave = bAutosave;
		}
	}

	CurrentSaveGame->LastTimeUpdated = FDateTime::Now();

	UGameplayStatics::AsyncSaveGameToSlot(CurrentSaveGame, CurrentID, 0);

	if (bAutosave)
		StartAutosaveTimer();
}

void USaveGameComponent::LoadGameSave(FString SlotName, class UDiplosimSaveGame* SaveGame, int32 Index)
{
	CurrentID = SlotName;
	CurrentSaveGame = SaveGame;
	CurrentSaveGame->LastTimeUpdated = FDateTime::Now();

	Camera->Cancel();

	for (FTimerStruct timer : Camera->CitizenManager->Timers)
		Camera->CitizenManager->RemoveTimer(timer.ID, timer.Actor);

	Camera->ConquestManager->Factions = CurrentSaveGame->Saves[Index].Factions;

	for (FFactionStruct& faction : Camera->ConquestManager->Factions) {
		faction.Buildings.Empty();
		faction.Citizens.Empty();
		faction.Rebels.Empty();
		faction.Clones.Empty();
	}

	ADiplosimGameModeBase* gamemode = GetWorld()->GetAuthGameMode<ADiplosimGameModeBase>();

	TArray<AActor*> actors;
	TArray<AActor*> foundActors;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEggBasket::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding::StaticClass(), foundActors);
	actors.Append(foundActors);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProjectile::StaticClass(), foundActors);
	actors.Append(foundActors);

	for (AActor* a : actors)
		a->Destroy();

	TMap<FString, FActorSaveData> aiToName;
	TArray<FWetnessData> wetnessData;

	for (FActorSaveData actorData : CurrentSaveGame->Saves[Index].SavedActors) {
		AActor* actor = nullptr;

		if (foundActors.Num() == 1) {
			if (foundActors[0]->GetName() != actorData.Name)
				continue;

			actor = foundActors[0];

			actor->SetActorTransform(actorData.Transform);

			break;
		}

		if (!IsValid(actor)) {
			FActorSpawnParameters params;
			params.bNoFail = true;

			actor = GetWorld()->SpawnActor<AActor>(actorData.Class, actorData.Transform, params);
		}

		if (actor->IsA<AGrid>()) {
			AGrid* grid = Cast<AGrid>(actor);

			grid->Size = actorData.WorldSaveData.Size;
			grid->Chunks = actorData.WorldSaveData.Chunks;

			grid->Stream = actorData.WorldSaveData.Stream;

			grid->LavaSpawnLocations = actorData.WorldSaveData.LavaSpawnLocations;

			grid->Clear();

			grid->InitialiseStorage();
			auto bound = grid->GetMapBounds();

			for (FTileData t : actorData.WorldSaveData.Tiles) {
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

			grid->SpawnTiles(true);

			TArray<UHierarchicalInstancedStaticMeshComponent*> hisms = { grid->HISMFlatGround, grid->HISMGround, grid->HISMLava, grid->HISMRampGround, grid->HISMRiver, grid->HISMSea, grid->HISMWall };
			
			for (UHierarchicalInstancedStaticMeshComponent* hism : hisms) {
				FHISMData data;
				data.Name = hism->GetName();

				int32 index = actorData.WorldSaveData.HISMData.Find(data);
				hism->PerInstanceSMCustomData = actorData.WorldSaveData.HISMData[index].CustomDataValues;

				hism->BuildTreeIfOutdated(true, true);
			}

			grid->AtmosphereComponent->Calendar = actorData.WorldSaveData.AtmosphereData.Calendar;
			grid->AtmosphereComponent->bRedSun = actorData.WorldSaveData.AtmosphereData.bRedSun;
			grid->AtmosphereComponent->WindRotation = actorData.WorldSaveData.AtmosphereData.WindRotation;

			if (grid->AtmosphereComponent->bRedSun)
				grid->AtmosphereComponent->NaturalDisasterComponent->AlterSunGradually(0.15f, -1.00f);

			grid->AtmosphereComponent->Clouds->ProcessRainEffect.Empty();
			grid->AtmosphereComponent->Clouds->WetnessStruct.Empty();
			wetnessData = actorData.WorldSaveData.CloudsData.WetnessData;

			grid->AtmosphereComponent->Clouds->Clear();
			
			for (FCloudData data : actorData.WorldSaveData.CloudsData.CloudData) {
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
		else if (actor->IsA<AResource>()) {
			AResource* resource = Cast<AResource>(actor);

			resource->ResourceHISM->AddInstances(actorData.ResourceData.HISMData.Transforms, true, true, false);
			resource->ResourceHISM->PerInstanceSMCustomData = actorData.ResourceData.HISMData.CustomDataValues;

			resource->WorkerStruct = actorData.ResourceData.Workers;

			resource->ResourceHISM->BuildTreeIfOutdated(true, true);
		}
		else if (actor->IsA<AEggBasket>()) {
			AEggBasket* eggBasket = Cast<AEggBasket>(actor);

			FTileStruct* tile = Camera->Grid->GetTileFromLocation(actorData.Transform.GetLocation());

			eggBasket->Grid = Camera->Grid;
			eggBasket->Tile = tile;

			Camera->Grid->ResourceTiles.RemoveSingle(tile);
		}
		else if (actor->IsA<ACamera>()) {
			ACamera* camera = Cast<ACamera>(actor);
			camera->Start = false;
			camera->Cancel();

			camera->ClearPopupUI();
			camera->SetInteractStatus(camera->WidgetComponent->GetAttachmentRootActor(), false);

			camera->Detach();
			camera->MovementComponent->TargetLength = 3000.0f;

			camera->ConstructionManager->Construction.Empty();

			camera->CitizenManager->IssuePensionHour = actorData.CameraData.CitizenManagerData.IssuePensionHour;

			camera->ConquestManager->FactionsToRemove.Append(camera->ConquestManager->Factions);
			for (FFactionData data : actorData.CameraData.FactionData) {
				FFactionStruct faction;
				faction.Name = data.Name;
				faction.Flag = data.Flag;
				faction.FlagColour = data.FlagColour;
				faction.AtWar = data.AtWar;
				faction.Allies = data.Allies;
				faction.Happiness = data.Happiness;
				faction.WarFatigue = data.WarFatigue;
				faction.PartyInPower = data.PartyInPower;
				faction.LargestReligion = data.LargestReligion;
				faction.RebelCooldownTimer = data.RebelCooldownTimer;
				faction.ResearchStruct = data.ResearchStruct;
				faction.ResearchIndex = data.ResearchIndex;
				faction.PrayStruct = data.PrayStruct;
				faction.Resources = data.Resources;
				faction.AccessibleBuildLocations = data.AccessibleBuildLocations;
				faction.InaccessibleBuildLocations = data.InaccessibleBuildLocations;
				faction.RoadBuildLocations = data.RoadBuildLocations;
				faction.FailedBuild = data.FailedBuild;

				camera->ConquestManager->Factions.Add(faction);
			}

			FGamemodeData* gamemodeData = &actorData.CameraData.GamemodeData;

			gamemode->bOngoingRaid = gamemodeData->bOngoingRaid;
			camera->Grid->CrystalMesh->SetCustomPrimitiveDataFloat(0, gamemodeData->CrystalOpacity);
			gamemode->TargetOpacity = gamemodeData->TargetOpacity;

			gamemode->SetActorTickEnabled(true);
		}
		else if (actor->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(actor);

			FAIData* data = &actorData.AIData;

			aiToName.Add(actorData.Name, actorData);

			UHierarchicalInstancedStaticMeshComponent* hism;

			if (actorData.AIData.FactionName == "") {
				hism = Camera->Grid->AIVisualiser->HISMEnemy;
			}
			else {
				FFactionStruct* faction = Camera->ConquestManager->GetFaction(actorData.AIData.FactionName);

				if (ai->IsA<AClone>()) {
					faction->Clones.Add(ai);
					hism = Camera->Grid->AIVisualiser->HISMClone;
				}
				else if (actorData.AIData.CitizenData.bRebel) {
					faction->Rebels.Add(Cast<ACitizen>(ai));
					hism = Camera->Grid->AIVisualiser->HISMRebel;
				}
				else {
					faction->Citizens.Add(Cast<ACitizen>(ai));
					hism = Camera->Grid->AIVisualiser->HISMCitizen;
				}
			}

			ai->MovementComponent->Points = data->MovementData.Points;
			ai->MovementComponent->CurrentAnim = data->MovementData.CurrentAnim;
			ai->MovementComponent->LastUpdatedTime = data->MovementData.LastUpdatedTime;
			ai->MovementComponent->Transform = data->MovementData.Transform;
			ai->MovementComponent->TempPoints = data->MovementData.TempPoints;
			ai->MovementComponent->bSetPoints = data->MovementData.bSetPoints;

			if (ai->IsA<ACitizen>()) {
				ACitizen* citizen = Cast<ACitizen>(ai);

				citizen->Carrying.Type = Cast<AResource>(data->CitizenData.ResourceCarryClass->GetDefaultObject());
				citizen->Carrying.Amount = data->CitizenData.CarryAmount;

				citizen->BioStruct.HoursTogetherWithPartner = data->CitizenData.HoursTogetherWithPartner;
				citizen->BioStruct.bMarried = data->CitizenData.bMarried;
				citizen->BioStruct.Sex = data->CitizenData.Sex;
				citizen->BioStruct.Age = data->CitizenData.Age;
				citizen->BioStruct.Name = data->CitizenData.Name;
				citizen->BioStruct.EducationLevel = data->CitizenData.EducationLevel;
				citizen->BioStruct.EducationProgress = data->CitizenData.EducationProgress;
				citizen->BioStruct.PaidForEducationLevel = data->CitizenData.PaidForEducationLevel;
				citizen->BioStruct.bAdopted = data->CitizenData.bAdopted;

				citizen->Spirituality = data->CitizenData.Spirituality;
				citizen->TimeOfAcquirement = data->CitizenData.TimeOfAcquirement;
				citizen->VoicePitch = data->CitizenData.VoicePitch;
				citizen->Balance = data->CitizenData.Balance;
				citizen->HoursWorked = data->CitizenData.HoursWorked;
				citizen->Hunger = data->CitizenData.Hunger;
				citizen->Energy = data->CitizenData.Energy;
				citizen->bGain = data->CitizenData.bGain;
				citizen->SpeedBeforeOld = data->CitizenData.SpeedBeforeOld;
				citizen->MaxHealthBeforeOld = data->CitizenData.MaxHealthBeforeOld;
				citizen->bHasBeenLeader = data->CitizenData.bHasBeenLeader;
				citizen->MassStatus = data->CitizenData.MassStatus;
				citizen->HealthIssues = data->CitizenData.HealthIssues;
				citizen->Happiness = data->CitizenData.Happiness;
				citizen->SadTimer = data->CitizenData.SadTimer;
				citizen->bHolliday = data->CitizenData.bHolliday;
				citizen->FestivalStatus = data->CitizenData.FestivalStatus;
				citizen->ConversationHappiness = data->CitizenData.ConversationHappiness;
				citizen->FamilyDeathHappiness = data->CitizenData.FamilyDeathHappiness;
				citizen->WitnessedDeathHappiness = data->CitizenData.WitnessedDeathHappiness;
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

			Camera->Grid->AIVisualiser->AddInstance(ai, hism, ai->MovementComponent->Transform);
		}
		else if (actor->IsA<ABuilding>()) {
			ABuilding* building = Cast<ABuilding>(actor);

			building->BuildingMesh->SetCanEverAffectNavigation(true);
			building->BuildingMesh->bReceivesDecals = true;

			building->FactionName = actorData.BuildingData.FactionName;

			FFactionStruct* faction = Camera->ConquestManager->GetFaction(building->FactionName);
			faction->Buildings.Add(building);

			if (building->IsA<ABroch>())
				faction->EggTimer = Cast<ABroch>(building);

			building->SetSeed(actorData.BuildingData.Seed);
			building->SetTier(actorData.BuildingData.Tier);

			building->Capacity = actorData.BuildingData.Capacity;

			FLinearColor colour = actorData.BuildingData.ChosenColour;
			building->SetBuildingColour(colour.R, colour.G, colour.B);

			building->TargetList = actorData.BuildingData.TargetList;

			building->Storage = actorData.BuildingData.Storage;
			building->Basket = actorData.BuildingData.Basket;

			building->DeathTime = actorData.BuildingData.DeathTime;
			building->bOperate = actorData.BuildingData.bOperate;

			if (building->IsA<AWork>())
				Cast<AWork>(building)->WorkHours = actorData.BuildingData.WorkHours;

			for (FOccupantData data : actorData.BuildingData.OccupantsData) {
				FActorSaveData aiData = *aiToName.Find(data.OccupantName);

				SetupCitizenBuilding(actorData.Name, building, aiData, false);

				FOccupantStruct occupant;
				occupant.Occupant = Cast<ACitizen>(aiData.Actor);

				for (FString name : data.VisitorNames) {
					FActorSaveData visitorData = *aiToName.Find(name);

					occupant.Visitors.Add(Cast<ACitizen>(visitorData.Actor));

					SetupCitizenBuilding(actorData.Name, building, visitorData, true);
				}

				building->Occupied.Add(occupant);
			}

			TArray<FOccupantData> occData;

			for (FOccupantStruct occupant : building->Occupied) {
				FOccupantData occupantData;
				occupantData.OccupantName = occupant.Occupant->GetName();

				for (ACitizen* visitor : occupant.Visitors)
					occupantData.VisitorNames.Add(visitor->GetName());

				occData.Add(occupantData);
			}

			actorData.BuildingData.OccupantsData = occData;
		}
		else if (actor->IsA<AProjectile>()) {
			AProjectile* projectile = Cast<AProjectile>(actor);

			projectile->ProjectileMovementComponent->Velocity = actorData.ProjectileData.Velocity;
		}

		UHealthComponent* healthComp = actor->FindComponentByClass<UHealthComponent>();
		if (healthComp) {
			healthComp->Health = actorData.HealthData.Health;

			if (healthComp->Health == 0)
				healthComp->Death(nullptr);
		}

		actorData.Actor = actor;
	}

	for (FActorSaveData actorData : CurrentSaveGame->Saves[Index].SavedActors) {
		for (FTimerStruct timer : actorData.SavedTimers) {
			timer.Actor = actorData.Actor;

			for (FTimerParameterStruct params : timer.Parameters) {
				for (FActorSaveData data : CurrentSaveGame->Saves[Index].SavedActors) {
					if (params.ObjectClass == USoundBase::StaticClass()) {
						UAttackComponent* attackComponent = data.Actor->FindComponentByClass<UAttackComponent>();

						if (!attackComponent)
							continue;

						params.Object = attackComponent->OnHitSound;
					}
					else if (params.ObjectClass == timer.Actor->GetClass()) {
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

			Camera->CitizenManager->Timers.AddTail(timer);
		}

		TArray<FActorSaveData> savedData = CurrentSaveGame->Saves[Index].SavedActors;

		UAttackComponent* attackComp = actorData.Actor->FindComponentByClass<UAttackComponent>();
		if (attackComp) {
			attackComp->ProjectileClass = actorData.AttackData.ProjectileClass;
			attackComp->AttackTimer = actorData.AttackData.AttackTimer;
			attackComp->bShowMercy = actorData.AttackData.bShowMercy;

			for (FString name : actorData.AttackData.ActorNames)
				attackComp->OverlappingEnemies.Add(GetSaveActorFromName(savedData, name));

			if (!attackComp->OverlappingEnemies.IsEmpty())
				attackComp->SetComponentTickEnabled(true);
		}

		if (actorData.Actor->IsA<AProjectile>()) {
			Cast<AProjectile>(actorData.Actor)->SpawnNiagaraSystems(GetSaveActorFromName(savedData, actorData.ProjectileData.OwnerName));
		}
		else if (actorData.Actor->IsA<AAI>()) {
			AAI* ai = Cast<AAI>(actorData.Actor);
			FAIMovementData* movementData = &actorData.AIData.MovementData;

			ai->AIController->ChosenBuilding = Cast<ABuilding>(GetSaveActorFromName(savedData, movementData->ChosenBuildingName));

			ai->MovementComponent->ActorToLookAt = GetSaveActorFromName(savedData, movementData->ActorToLookAtName);

			FMoveStruct moveStruct;
			moveStruct.Actor = GetSaveActorFromName(savedData, movementData->ActorName);
			moveStruct.LinkedPortal = GetSaveActorFromName(savedData, movementData->LinkedPortalName);
			moveStruct.UltimateGoal = GetSaveActorFromName(savedData, movementData->UltimateGoalName);

			moveStruct.Instance = actorData.AIData.MovementData.Instance;
			moveStruct.Location = actorData.AIData.MovementData.Location;

			ai->AIController->MoveRequest = moveStruct;

			if (ai->IsA<ACitizen>()) {
				ACitizen* citizen = Cast<ACitizen>(ai);
				FCitizenData* citizenData = &actorData.AIData.CitizenData;

				citizen->BioStruct.Mother = Cast<ACitizen>(GetSaveActorFromName(savedData, citizenData->MothersName));

				citizen->BioStruct.Father = Cast<ACitizen>(GetSaveActorFromName(savedData, citizenData->FathersName));

				citizen->BioStruct.Partner = Cast<ACitizen>(GetSaveActorFromName(savedData, citizenData->PartnersName));

				for (FString name : citizenData->ChildrensNames)
					citizen->BioStruct.Children.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				for (FString name : citizenData->SiblingsNames)
					citizen->BioStruct.Siblings.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));
			}
		}
		else if (actorData.Actor->IsA<ACamera>()) {
			ACamera* camera = Cast<ACamera>(actorData.Actor);

			for (FConstructionData constructionData : actorData.CameraData.ConstructionData) {
				FConstructionStruct constructionStruct;
				constructionStruct.Status = constructionData.Status;
				constructionStruct.BuildPercentage = constructionData.BuildPercentage;
				constructionStruct.Building = Cast<ABuilding>(GetSaveActorFromName(savedData, constructionData.BuildingName));

				if (constructionStruct.Status == EBuildStatus::Construction)
					constructionStruct.Building->SetConstructionMesh();

				if (constructionData.BuilderName != "")
					constructionStruct.Builder = Cast<ABuilder>(GetSaveActorFromName(savedData, constructionData.BuilderName));

				camera->ConstructionManager->Construction.Add(constructionStruct);
			}

			FCitizenManagerData cmData = actorData.CameraData.CitizenManagerData;

			for (FPersonalityData personalityData : cmData.PersonalitiesData) {
				FPersonality personality;
				personality.Trait = personalityData.Trait;

				int32 index = camera->CitizenManager->Personalities.Find(personality);

				for (FString name : personalityData.CitizenNames)
					camera->CitizenManager->Personalities[index].Citizens.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));
			}

			for (FString name : cmData.InfectibleNames)
				camera->CitizenManager->Infectible.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

			for (FString name : cmData.InfectedNames)
				camera->CitizenManager->Infected.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

			for (FString name : cmData.InjuredNames)
				camera->CitizenManager->Injured.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

			for (FString name : cmData.EnemyNames)
				camera->CitizenManager->Enemies.Add(Cast<AAI>(GetSaveActorFromName(savedData, name)));

			for (FFactionData data : actorData.CameraData.FactionData) {
				FFactionStruct* faction = camera->ConquestManager->GetFaction(data.Name);

				for (FPartyData partyData : data.PoliticsData.PartiesData) {
					FPartyStruct party;

					party.Party = partyData.Party;
					party.Agreeable = partyData.Agreeable;

					for (auto element : partyData.MembersName)
						party.Members.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, element.Key)), element.Value);

					if (partyData.LeaderName != "")
						party.Leader = Cast<ACitizen>(GetSaveActorFromName(savedData, partyData.LeaderName));

					faction->Politics.Parties.Add(party);
				}

				for (FString name : data.PoliticsData.RepresentativesNames)
					faction->Politics.Representatives.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				for (FString name : data.PoliticsData.VotesData.ForNames)
					faction->Politics.Votes.For.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				for (FString name : data.PoliticsData.VotesData.AgainstNames)
					faction->Politics.Votes.Against.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				for (FString name : data.PoliticsData.PredictionsData.ForNames)
					faction->Politics.Predictions.For.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				for (FString name : data.PoliticsData.PredictionsData.AgainstNames)
					faction->Politics.Predictions.Against.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

				faction->Politics.BribeValue = data.PoliticsData.BribeValue;
				faction->Politics.Laws = data.PoliticsData.Laws;
				faction->Politics.ProposedBills = data.PoliticsData.ProposedBills;

				for (auto element : data.PoliceData.ArrestedNames)
					faction->Police.Arrested.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, element.Key)), element.Value);

				for (FPoliceReportData reportData : data.PoliceData.PoliceReportsData) {
					FPoliceReport report;
					report.Type = reportData.Type;
					report.Location = reportData.Location;

					if (reportData.Team1.InstigatorName != "")
						report.Team1.Instigator = Cast<ACitizen>(GetSaveActorFromName(savedData, reportData.Team1.InstigatorName));

					for (FString name : reportData.Team1.AssistorsNames)
						report.Team1.Assistors.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					if (reportData.Team2.InstigatorName != "")
						report.Team2.Instigator = Cast<ACitizen>(GetSaveActorFromName(savedData, reportData.Team2.InstigatorName));

					for (FString name : reportData.Team2.AssistorsNames)
						report.Team2.Assistors.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					for (auto element : reportData.WitnessesNames)
						report.Witnesses.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, element.Key)), element.Value);

					if (reportData.RespondingOfficerName != "")
						report.RespondingOfficer = Cast<ACitizen>(GetSaveActorFromName(savedData, reportData.RespondingOfficerName));

					for (FString name : reportData.AcussesTeam1Names)
						report.AcussesTeam1.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					for (FString name : reportData.ImpartialNames)
						report.Impartial.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					for (FString name : reportData.AcussesTeam2Names)
						report.AcussesTeam2.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					faction->Police.PoliceReports.Add(report);
				}

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
						event.Venue = Cast<ABuilding>(GetSaveActorFromName(savedData, eventData.VenueName));

					for (FString name : eventData.WhitelistNames)
						event.Whitelist.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					for (FString name : eventData.AttendeesNames)
						event.Attendees.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					faction->Events.Add(event);
				}

				for (FArmyData armyData : data.ArmiesData) {
					TArray<ACitizen*> citizens;
					for (FString name : armyData.CitizensNames)
						citizens.Add(Cast<ACitizen>(GetSaveActorFromName(savedData, name)));

					camera->ConquestManager->CreateArmy(data.Name, citizens, armyData.bGroup, true);
				}
			}

			FGamemodeData* gamemodeData = &actorData.CameraData.GamemodeData;

			for (FWaveData waveData : gamemodeData->WaveData) {
				FWaveStruct wave;
				wave.SpawnLocations = waveData.SpawnLocations;

				for (auto element : waveData.DiedTo) {
					FDiedToStruct diedTo;
					diedTo.Actor = GetSaveActorFromName(savedData, element.Key);
					diedTo.Kills = element.Value;

					wave.DiedTo.Add(diedTo);
				}

				for (FString name : waveData.Threats)
					wave.Threats.Add(GetSaveActorFromName(savedData, name));

				wave.NumKilled = waveData.NumKilled;
				wave.EnemiesData = waveData.EnemiesData;

				gamemode->WavesData.Add(wave);
			}

			if (!gamemode->bSpawnedAllEnemies())
				gamemode->SpawnAllEnemies();
		}
	}

	for (FWetnessData data : wetnessData)
		Camera->Grid->AtmosphereComponent->Clouds->RainCollisionHandler(data.Location, data.Value, data.Increment);

	Camera->BuildUIInstance->AddToViewport();

	StartAutosaveTimer();
}

void USaveGameComponent::DeleteGameSave(FString SlotName, UDiplosimSaveGame* SaveGame, int32 Index, bool bSlot)
{
	if (bSlot)
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	else
		SaveGame->Saves.RemoveAt(Index);
}

TMap<FString, class UDiplosimSaveGame*> USaveGameComponent::LoadAllSavedGames()
{
	TMap<FString, class UDiplosimSaveGame*> gameSavesList;

	const FString& saveDir = UKismetSystemLibrary::GetProjectSavedDirectory() / "SaveGames";

	TArray<FString> foundFiles;
	const FString& ext = ".sav";

	IFileManager::Get().FindFiles(foundFiles, *saveDir, *ext);

	TArray<FString> fileNames;
	for (const FString& name : foundFiles)
		fileNames.AddUnique(name.LeftChop(4));

	TArray<UDiplosimSaveGame*> sortedGames;
	TArray<FString> sortedNames;
	for (const FString& name : fileNames) {
		UDiplosimSaveGame* gameSave = Cast<UDiplosimSaveGame>(UGameplayStatics::LoadGameFromSlot(name, 0));

		int32 index = 0;

		for (int32 i = 0; i < sortedGames.Num(); i++) {
			if (gameSave->LastTimeUpdated > sortedGames[i]->LastTimeUpdated)
				break;

			index++;
		}

		sortedGames.Insert(gameSave, index);
		sortedNames.Insert(name, index);
	}

	for (int32 i = 0; i < sortedGames.Num(); i++)
		gameSavesList.Add(sortedNames[i], sortedGames[i]);

	return gameSavesList;
}

void USaveGameComponent::CreateNewSaveStruct(FString Name, bool bAutosave, TArray<FActorSaveData> NewActorData)
{
	FCalendarStruct calendar = Camera->Grid->AtmosphereComponent->Calendar;

	FSave save;
	save.SaveName = Name;
	save.Period = calendar.Period;
	save.Day = calendar.Days[calendar.Index];
	save.Hour = calendar.Hour;

	if (Name == "")
		save.SaveName = FDateTime::Now().ToString();

	save.Factions = Camera->ConquestManager->Factions;
	save.SavedActors = NewActorData;

	save.bAutosave = bAutosave;

	CurrentSaveGame->Saves.Add(save);
}

void USaveGameComponent::CapAutosaves()
{
	int32 firstAutosaveIndex = INDEX_NONE;
	int32 count = 0;

	for (int32 i = 0; i < CurrentSaveGame->Saves.Num(); i++) {
		if (!CurrentSaveGame->Saves[i].bAutosave)
			continue;

		if (firstAutosaveIndex == INDEX_NONE)
			firstAutosaveIndex = i;

		count++;
	}

	if (count <= 3)
		return;

	CurrentSaveGame->Saves.RemoveAt(firstAutosaveIndex);
}

void USaveGameComponent::StartAutosaveTimer()
{
	int32 time = Camera->Settings->GetAutosaveTimer();

	if (time == 0)
		return;

	FTimerStruct* timer = Camera->CitizenManager->FindTimer("AutosaveTimer", Camera);

	if (timer == nullptr) {
		TArray<FTimerParameterStruct> params;
		Camera->CitizenManager->SetParameter("", params);
		Camera->CitizenManager->SetParameter(true, params);

		Camera->CitizenManager->CreateTimer("AutosaveTimer", Camera, time * 60.0f, "SaveGameSave", params, false, true);
	}
	else
		timer->Timer = 0.0f;
}

void USaveGameComponent::UpdateAutosave(int32 NewTime)
{
	if (NewTime == 0) {
		Camera->CitizenManager->RemoveTimer("AutosaveTimer", Camera);
	}
	else {
		FTimerStruct* timer = Camera->CitizenManager->FindTimer("AutosaveTimer", Camera);

		if (timer == nullptr)
			StartAutosaveTimer();
		else if (NewTime - timer->Timer > 0.0f)
			timer->Target = NewTime;
	}
}

AActor* USaveGameComponent::GetSaveActorFromName(TArray<FActorSaveData> SavedData, FString Name)
{
	FActorSaveData data;
	data.Name = Name;

	int32 i = SavedData.Find(data);

	return SavedData[i].Actor;
}

void USaveGameComponent::SetupCitizenBuilding(FString BuildingName, ABuilding* Building, FActorSaveData CitizenData, bool bVisitor)
{
	ACitizen* citizen = Cast<ACitizen>(CitizenData.Actor);

	if (Building->IsA<AHouse>())
		citizen->Building.House = Cast<AHouse>(Building);
	else if (Building->IsA<AOrphanage>() && bVisitor)
		citizen->Building.Orphanage = Cast<AOrphanage>(Building);
	else if (Building->IsA<ASchool>() && bVisitor)
		citizen->Building.School = Cast<ASchool>(Building);
	else
		citizen->Building.Employment = Cast<AWork>(Building);

	if (CitizenData.AIData.BuildingAtName == BuildingName) {
		Building->Enter(citizen);

		citizen->Building.EnterLocation = CitizenData.AIData.CitizenData.EnterLocation;
	}
}