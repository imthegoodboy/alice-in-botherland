// Alice in Botherland - server authority: Stage 3 race, the final choice, endings and results

#include "Game/AIBGameMode.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Game/AIBBotController.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBCharacter.h"
#include "Player/AIBVehicle.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBRaceCourse.h"
#include "World/AIBGuardianSite.h"
#include "World/AIBLayout.h"
#include "AliceInBotherland.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBFinale"

// ---------------------------------------------------------------------------------------------------------------------
// Stage 3 - Race to the Guardian
// ---------------------------------------------------------------------------------------------------------------------

void AAIBGameMode::Stage3_Enter()
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBGuardianSite* Guardian = WorldSys() ? WorldSys()->GetGuardian() : nullptr;
	if (Guardian)
	{
		GS->GuardianLocation = Guardian->GetSwitchLocation();
	}

	if (GS->bRaceSuddenDeath)
	{
		// Second attempt: shorter road, same machines.
		Stage3_PlaceVehicles(true);
		return;
	}

	GS->Finalists.Reset();
	for (AAIBPlayerState* PS : ActivePlayers())
	{
		GS->Finalists.Add(PS->Slot);
	}
	GS->RaceWinnerSlot = INDEX_NONE;
	GS->ChoiceOrder.Reset();
	GS->SwitchProgress.Init(0.f, GS->Finalists.Num());
	GS->RaceDistance.Init(0.f, GS->Finalists.Num());
	SetBotWander({}, FBox());

	if (GS->Finalists.Num() == 0)
	{
		Announce(LOCTEXT("NoOne", "NO CHAMPION REMAINS"), LOCTEXT("NoOneSub", "The island stays silent."));
		EnterPhase(EAIBPhase::Results);
		return;
	}
	if (GS->Finalists.Num() == 1)
	{
		const AAIBPlayerState* PS = GS->GetPlayerBySlot(GS->Finalists[0]);
		GS->RaceWinnerSlot = GS->Finalists[0];
		GS->ChoiceOrder = GS->Finalists;
		Announce(FText::Format(LOCTEXT("Walkover", "{0} ADVANCES UNOPPOSED"), FText::FromString(PS ? PS->GetPlayerName().ToUpper() : FString())),
			LOCTEXT("WalkoverSub", "Their rival forfeited. The Guardian awaits."), FLinearColor(1.f, 0.85f, 0.4f));
		EnterPhase(EAIBPhase::Stage3Finish);
		return;
	}

	Stage3_PlaceVehicles(false);
	Cue(TEXT("SFX_Stinger_Title"));
	Cue(TEXT("MUS_Race"));
}

void AAIBGameMode::Stage3_PlaceVehicles(bool bSuddenDeath)
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBRaceCourse* Course = WorldSys() ? WorldSys()->GetRaceCourse() : nullptr;
	if (!Course)
	{
		return;
	}
	Vehicles.SetNum(GS->Finalists.Num());
	SwitchHold.Init(0.f, GS->Finalists.Num());
	SwitchInteracting.Init(false, GS->Finalists.Num());
	for (int32 Index = 0; Index < GS->Finalists.Num(); ++Index)
	{
		const int32 Slot = GS->Finalists[Index];
		const FTransform Start = Course->GetStart(Index, bSuddenDeath);
		AAIBVehicle* Vehicle = Vehicles[Index];
		if (!Vehicle)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Vehicle = GetWorld()->SpawnActor<AAIBVehicle>(AAIBVehicle::StaticClass(), Start, Params);
			Vehicles[Index] = Vehicle;
		}
		else
		{
			Vehicle->ResetTo(Start);
		}
		if (!Vehicle)
		{
			continue;
		}
		Vehicle->SetDriverSlot(Slot);
		Vehicle->SetInputLocked(true);

		AController* Controller = GetControllerForSlot(Slot);
		if (AAIBCharacter* Character = GetCharacterForSlot(Slot))
		{
			Character->SetHiddenForRace(true);
			Character->SetMovementLocked(true);
			Character->SetActorLocation(Start.GetLocation() + FVector(0.f, 0.f, -5000.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (Controller && Controller->GetPawn() != Vehicle)
		{
			Controller->UnPossess();
			Controller->Possess(Vehicle);
			Controller->SetControlRotation(Start.Rotator());
		}
		if (GS->SwitchProgress.IsValidIndex(Index))
		{
			GS->SwitchProgress[Index] = 0.f;
		}
	}
}

void AAIBGameMode::Stage3_StartRace()
{
	RaceStartTime = Now();
	for (AAIBVehicle* Vehicle : Vehicles)
	{
		if (Vehicle)
		{
			Vehicle->SetInputLocked(false);
		}
	}
	Cue(TEXT("SFX_Go"));
}

void AAIBGameMode::SetSwitchInteract(AController* Controller, bool bHolding)
{
	const AAIBGameState* GS = GetAIBGameState();
	const AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}
	const int32 Index = GS->Finalists.IndexOfByKey(PS->Slot);
	if (SwitchInteracting.IsValidIndex(Index))
	{
		SwitchInteracting[Index] = bHolding;
	}
}

void AAIBGameMode::Stage3_Tick(float DeltaSeconds)
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBRaceCourse* Course = WorldSys() ? WorldSys()->GetRaceCourse() : nullptr;
	AAIBGuardianSite* Guardian = WorldSys() ? WorldSys()->GetGuardian() : nullptr;
	if (!Course || !Guardian || GS->RaceWinnerSlot != INDEX_NONE)
	{
		return;
	}
	const FVector Switch = Guardian->GetSwitchLocation();
	const bool bLog = Now() >= NextVehicleLog;
	if (bLog)
	{
		NextVehicleLog = Now() + 2.f;
	}
	for (int32 Index = 0; Index < Vehicles.Num(); ++Index)
	{
		AAIBVehicle* Vehicle = Vehicles[Index];
		if (!Vehicle || !GS->Finalists.IsValidIndex(Index))
		{
			continue;
		}
		const AAIBPlayerState* PS = GS->GetPlayerBySlot(GS->Finalists[Index]);
		const FVector Location = Vehicle->GetActorLocation();
		if (bLog)
		{
			UChaosWheeledVehicleMovementComponent* Move = Vehicle->GetChaos();
			UE_LOG(LogAIB, Log, TEXT("Race[%d] %s ctrl=%s speed=%.1f km/h throttle=%.2f brake=%.2f rpm=%.0f gear=%d ground=%d loc=%s sim=%d"),
				Index, PS ? *PS->GetPlayerName() : TEXT("?"), *GetNameSafe(Vehicle->GetController()), Vehicle->GetSpeedKmh(),
				Move ? Move->GetThrottleInput() : -1.f, Move ? Move->GetBrakeInput() : -1.f, Move ? Move->GetEngineRotationSpeed() : -1.f,
				Move ? Move->GetCurrentGear() : -99, Move ? (int32)Move->IsMovingOnGround() : -1, *Location.ToCompactString(),
				(int32)Vehicle->GetMesh()->IsSimulatingPhysics());
		}
		if (GS->RaceDistance.IsValidIndex(Index))
		{
			GS->RaceDistance[Index] = Course->RemainingDistance(Location);
		}

		// Acid sea or off the island: back onto the road.
		if (Location.Z < AIBLayout::SeaLevel + 40.f || !AIBLayout::IslandBounds.IsInside(FVector2D(Location)))
		{
			Vehicle->ResetTo(Course->GetResetTransform(Location));
			if (AAIBPlayerController* PC = Cast<AAIBPlayerController>(Vehicle->GetController()))
			{
				PC->ClientNotify(LOCTEXT("CarAcid", "The acid nearly took you. Back on the road!"), TEXT("SFX_Wrong"));
			}
		}

		const bool bInZone = FVector::Dist2D(Location, Switch) < AAIBGuardianSite::SwitchRadius && Vehicle->GetSpeedKmh() < 25.f;
		const bool bIsBot = PS && PS->bBot && !PS->bDisconnected;
		const bool bHolding = bInZone && (bIsBot || (SwitchInteracting.IsValidIndex(Index) && SwitchInteracting[Index]));
		SwitchHold[Index] = bHolding ? SwitchHold[Index] + DeltaSeconds : FMath::Max(0.f, SwitchHold[Index] - DeltaSeconds * 2.f);
		if (GS->SwitchProgress.IsValidIndex(Index))
		{
			GS->SwitchProgress[Index] = FMath::Clamp(SwitchHold[Index] / AIB::SwitchHold, 0.f, 1.f);
		}
		if (SwitchHold[Index] >= AIB::SwitchHold)
		{
			Stage3_Claim(Index);
			return;
		}
	}
}

void AAIBGameMode::Stage3_TimeUp()
{
	AAIBGameState* GS = GetAIBGameState();
	if (!GS->bRaceSuddenDeath)
	{
		GS->bRaceSuddenDeath = true;
		Cue(TEXT("VO_Stage3_SuddenDeath"));
		Announce(LOCTEXT("S3Sudden", "TIME! SUDDEN DEATH RACE"), LOCTEXT("S3SuddenSub", "The road is shorter. 45 seconds."), FLinearColor(1.f, 0.4f, 0.1f));
		EnterPhase(EAIBPhase::Stage3Briefing, 6.f);
		return;
	}
	// Still nobody: whoever is closest to the Guardian claims first choice.
	int32 Best = 0;
	for (int32 Index = 1; Index < GS->RaceDistance.Num(); ++Index)
	{
		if (GS->RaceDistance[Index] < GS->RaceDistance[Best])
		{
			Best = Index;
		}
	}
	Stage3_Claim(Best);
}

void AAIBGameMode::Stage3_Claim(int32 FinalistIndex)
{
	AAIBGameState* GS = GetAIBGameState();
	if (!GS->Finalists.IsValidIndex(FinalistIndex) || GS->RaceWinnerSlot != INDEX_NONE)
	{
		return;
	}
	const int32 Winner = GS->Finalists[FinalistIndex];
	GS->RaceWinnerSlot = Winner;
	GS->ChoiceOrder.Reset();
	GS->ChoiceOrder.Add(Winner);
	for (int32 Slot : GS->Finalists)
	{
		if (Slot != Winner)
		{
			const AAIBPlayerState* Other = GS->GetPlayerBySlot(Slot);
			if (Other && Other->IsActiveChampion())
			{
				GS->ChoiceOrder.Add(Slot);
			}
		}
	}
	if (GS->SwitchProgress.IsValidIndex(FinalistIndex))
	{
		GS->SwitchProgress[FinalistIndex] = 1.f;
	}
	AAIBPlayerState* PS = GS->GetPlayerBySlot(Winner);
	if (PS)
	{
		PS->RaceTime = Now() - RaceStartTime;
	}
	for (AAIBVehicle* Vehicle : Vehicles)
	{
		if (Vehicle)
		{
			Vehicle->SetInputLocked(true);
		}
	}
	Cue(TEXT("SFX_Switch"));
	Cue(TEXT("SFX_Guardian_Rumble"));
	Cue(TEXT("VO_Stage3_Arrived"));
	Announce(FText::Format(LOCTEXT("Claimed", "{0} REACHED THE GUARDIAN FIRST"), FText::FromString(PS ? PS->GetPlayerName().ToUpper() : FString())),
		LOCTEXT("ClaimedSub", "The first choice is theirs. Winning the race is not winning the island."), FLinearColor(1.f, 0.85f, 0.4f));
	EnterPhase(EAIBPhase::Stage3Finish);
}

void AAIBGameMode::Stage3_Cleanup()
{
	for (AAIBVehicle* Vehicle : Vehicles)
	{
		if (!Vehicle)
		{
			continue;
		}
		if (AController* Controller = Vehicle->GetController())
		{
			Controller->UnPossess();
		}
		Vehicle->Destroy();
	}
	Vehicles.Reset();
	SwitchHold.Reset();
	SwitchInteracting.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
// Final - The Guardian's Choice
// ---------------------------------------------------------------------------------------------------------------------

void AAIBGameMode::Final_Enter()
{
	AAIBGameState* GS = GetAIBGameState();
	Stage3_Cleanup();
	AAIBGuardianSite* Guardian = WorldSys() ? WorldSys()->GetGuardian() : nullptr;

	// Drop finalists who forfeited since the race.
	GS->ChoiceOrder.RemoveAll([GS](int32 Slot)
	{
		const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		return !PS || !PS->IsActiveChampion();
	});
	if (GS->ChoiceOrder.Num() == 0)
	{
		Announce(LOCTEXT("NoFinal", "NO CHAMPION REMAINS"), LOCTEXT("NoFinalSub", "The island stays silent."));
		EnterPhase(EAIBPhase::Results);
		return;
	}
	for (int32 Index = 0; Index < GS->ChoiceOrder.Num(); ++Index)
	{
		const int32 Slot = GS->ChoiceOrder[Index];
		const FTransform Stand = Guardian ? Guardian->GetFinalStand(Index) : FTransform::Identity;
		AAIBCharacter* Character = EnsureCharacter(Slot, Stand);
		if (Character)
		{
			Character->SetHiddenForRace(false);
			Character->MulticastRestore();
			TeleportSlot(Slot, Stand);
		}
	}
	SetAllMovementLocked(true);
	GS->ChooserIndex = 0;
	GS->PendingOption = EAIBFinalChoice::None;
	GS->ConfirmEndTime = 0.f;
	GS->Choices.Reset();
	GS->Ending = EAIBEnding::None;
	GS->ChampionSlot = INDEX_NONE;
	Cue(TEXT("SFX_Guardian_Rumble"));
	Cue(TEXT("MUS_Final"));
}

void AAIBGameMode::Final_BeginChoice()
{
	AAIBGameState* GS = GetAIBGameState();
	GS->PendingOption = EAIBFinalChoice::None;
	GS->ConfirmEndTime = 0.f;
	bBotSelected = false;
	const int32 Slot = GS->GetCurrentChooserSlot();
	const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
	BotFinalActTime = Now() + Random.FRandRange(3.f, 9.f);
	if (PS)
	{
		Announce(FText::Format(LOCTEXT("Chooses", "{0} MUST CHOOSE"), FText::FromString(PS->GetPlayerName().ToUpper())),
			GS->ChooserIndex == 0 ? LOCTEXT("ChoosesFirst", "First choice, earned at the switch.") : LOCTEXT("ChoosesSecond", "A life was given. Now the choice passes on."),
			PS->GetCommunityColor());
	}
}

void AAIBGameMode::SubmitFinalSelect(AController* Controller, EAIBFinalChoice Choice)
{
	AAIBGameState* GS = GetAIBGameState();
	const AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || GS->Phase != EAIBPhase::FinalChoice || PS->Slot != GS->GetCurrentChooserSlot() || Choice == EAIBFinalChoice::None)
	{
		return;
	}
	GS->PendingOption = Choice;
	GS->ConfirmEndTime = Now() + AIB::FinalConfirm;
	// The confirmation window may run past the selection timer.
	GS->PhaseEndTime = FMath::Max(GS->PhaseEndTime, GS->ConfirmEndTime + 0.2f);
	GS->ForceNetUpdate();
}

void AAIBGameMode::SubmitFinalConfirm(AController* Controller, bool bConfirm)
{
	AAIBGameState* GS = GetAIBGameState();
	const AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || GS->Phase != EAIBPhase::FinalChoice || PS->Slot != GS->GetCurrentChooserSlot() || GS->PendingOption == EAIBFinalChoice::None)
	{
		return;
	}
	if (bConfirm)
	{
		Final_Commit(GS->PendingOption, false);
	}
	else
	{
		GS->PendingOption = EAIBFinalChoice::None;
		GS->ConfirmEndTime = 0.f;
		GS->ForceNetUpdate();
	}
}

void AAIBGameMode::Final_Tick()
{
	AAIBGameState* GS = GetAIBGameState();
	if (GS->ConfirmEndTime > 0.f && Now() >= GS->ConfirmEndTime)
	{
		// Not confirmed in time: back to choosing (the default still applies when the main timer runs out).
		GS->PendingOption = EAIBFinalChoice::None;
		GS->ConfirmEndTime = 0.f;
		GS->ForceNetUpdate();
	}

	AAIBPlayerState* PS = GS->GetPlayerBySlot(GS->GetCurrentChooserSlot());
	if (!PS || !PS->bBot || PS->bDisconnected || Now() < BotFinalActTime)
	{
		return;
	}
	AController* Bot = Cast<AController>(PS->GetOwner());
	if (!bBotSelected)
	{
		// Temperament: hot-headed bots grab the crown, calm ones save their people. Second choosers lean to the crown.
		const float KingChance = GS->ChooserIndex == 0 ? FMath::Lerp(0.3f, 0.75f, PS->BotTemper) : FMath::Lerp(0.55f, 0.85f, PS->BotTemper);
		SubmitFinalSelect(Bot, Random.FRand() < KingChance ? EAIBFinalChoice::King : EAIBFinalChoice::Sacrifice);
		bBotSelected = true;
		BotFinalActTime = Now() + Random.FRandRange(1.2f, 2.8f);
	}
	else
	{
		SubmitFinalConfirm(Bot, true);
	}
}

void AAIBGameMode::Final_Commit(EAIBFinalChoice Choice, bool bAuto)
{
	AAIBGameState* GS = GetAIBGameState();
	const int32 Slot = GS->GetCurrentChooserSlot();
	AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
	if (!PS)
	{
		Final_AfterResolve();
		return;
	}
	PS->FinalChoice = Choice;
	FAIBFinalRecord Record;
	Record.Slot = Slot;
	Record.Choice = Choice;
	Record.bAuto = bAuto;
	GS->Choices.Add(Record);
	GS->PendingOption = EAIBFinalChoice::None;
	GS->ConfirmEndTime = 0.f;

	AAIBCharacter* Character = GetCharacterForSlot(Slot);
	const FText Name = FText::FromString(PS->GetPlayerName().ToUpper());
	if (Choice == EAIBFinalChoice::King)
	{
		PS->Status = EAIBPlayerStatus::Champion;
		GS->ChampionSlot = Slot;
		if (GS->ChooserIndex == 0)
		{
			GS->Ending = GS->ChoiceOrder.Num() == 1 ? EAIBEnding::SoloKing : EAIBEnding::KingFirst;
		}
		else
		{
			GS->Ending = EAIBEnding::SacrificeThenKing;
		}
		if (Character)
		{
			Character->MulticastCrown();
		}
		Cue(TEXT("SFX_Crown"));
		Announce(FText::Format(LOCTEXT("King", "{0} HAS CLAIMED THE ISLAND"), Name),
			bAuto ? LOCTEXT("KingAuto", "Time ran out. The Guardian crowned them by default.") : FText::Format(LOCTEXT("KingSub", "{0} will not follow them ashore."), PS->GetCommunityName()),
			FLinearColor(1.f, 0.78f, 0.3f));
	}
	else
	{
		PS->Status = EAIBPlayerStatus::Sacrificed;
		if (Character)
		{
			Character->MulticastSacrifice();
		}
		Cue(TEXT("SFX_Choir"));
		Announce(FText::Format(LOCTEXT("Sacrifice", "{0} GAVE THEIR LIFE"), Name),
			FText::Format(LOCTEXT("SacrificeSub", "{0} is carried to safety."), PS->GetCommunityName()), FLinearColor(0.8f, 0.9f, 1.f));
	}
	PS->ForceNetUpdate();
	GS->ForceNetUpdate();
	EnterPhase(EAIBPhase::FinalResolve);
}

void AAIBGameMode::Final_AfterResolve()
{
	AAIBGameState* GS = GetAIBGameState();
	const bool bLastWasSacrifice = GS->Choices.Num() > 0 && GS->Choices.Last().Choice == EAIBFinalChoice::Sacrifice;
	if (bLastWasSacrifice && GS->ChooserIndex + 1 < GS->ChoiceOrder.Num())
	{
		const AAIBPlayerState* Next = GS->GetPlayerBySlot(GS->ChoiceOrder[GS->ChooserIndex + 1]);
		if (Next && Next->IsActiveChampion())
		{
			GS->ChooserIndex++;
			Cue(TEXT("VO_Final_Second"));
			EnterPhase(EAIBPhase::FinalChoice);
			return;
		}
	}
	if (GS->Ending == EAIBEnding::None)
	{
		GS->Ending = GS->Choices.Num() >= 2 ? EAIBEnding::DoubleSacrifice : EAIBEnding::SoloSacrifice;
	}
	FadeToPhase(EAIBPhase::EndingCinematic);
}

void AAIBGameMode::Ending_Enter()
{
	AAIBGameState* GS = GetAIBGameState();
	switch (GS->Ending)
	{
	case EAIBEnding::KingFirst:
	case EAIBEnding::SoloKing:
		Cue(TEXT("VO_End_King"));
		Cue(TEXT("MUS_End_King"));
		break;
	case EAIBEnding::SacrificeThenKing:
		Cue(TEXT("VO_End_SacrificeKing"));
		Cue(TEXT("MUS_End_King"));
		break;
	case EAIBEnding::DoubleSacrifice:
		Cue(TEXT("VO_End_Double"));
		Cue(TEXT("MUS_End_Sacrifice"));
		break;
	default:
		Cue(TEXT("VO_End_SoloSacrifice"));
		Cue(TEXT("MUS_End_Sacrifice"));
		break;
	}
	Announce(AIB::EndingTitle(GS->Ending), FText::GetEmpty(), FLinearColor(1.f, 0.85f, 0.6f));
}

void AAIBGameMode::Results_Enter()
{
	const FAIBMatchRecord Record = BuildMatchRecord();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAIBPlayerController* PC = Cast<AAIBPlayerController>(It->Get()))
		{
			PC->ClientRecordMatch(Record);
		}
	}
	Cue(TEXT("MUS_Menu"));
	UE_LOG(LogAIB, Log, TEXT("Match finished. Ending=%s Champion=%s"), *UEnum::GetValueAsString(Record.Ending), *Record.ChampionName);
}

FAIBMatchRecord AAIBGameMode::BuildMatchRecord() const
{
	const AAIBGameState* GS = GetAIBGameState();
	FAIBMatchRecord Record;
	Record.Date = FDateTime::Now();
	Record.Ending = GS->Ending;
	if (const AAIBPlayerState* Champion = GS->GetPlayerBySlot(GS->ChampionSlot))
	{
		Record.ChampionName = Champion->GetPlayerName();
		Record.ChampionCommunity = Champion->Slot;
	}
	for (const AAIBPlayerState* PS : GS->GetSeatedPlayers())
	{
		FAIBMatchPlayerRecord Player;
		Player.Name = PS->GetPlayerName();
		Player.Community = PS->Slot;
		Player.Status = PS->Status;
		Player.EliminatedInStage = PS->EliminatedInStage;
		Player.bBot = PS->bBot && !PS->bDisconnected;
		Record.Players.Add(Player);
	}
	return Record;
}

#undef LOCTEXT_NAMESPACE
