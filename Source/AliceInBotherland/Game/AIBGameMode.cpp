// Alice in Botherland - server authority: core, lobby, seating, phase machine

#include "Game/AIBGameMode.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Game/AIBBotController.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBCharacter.h"
#include "Player/AIBVehicle.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBLandingSite.h"
#include "World/AIBLayout.h"
#include "AliceInBotherland.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameSession.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBGameMode"

namespace
{
	/** Development: -AIBTimeScale=0.3 shortens every phase timer (fast end-to-end testing). */
	float PhaseTimeScale()
	{
		static const float Scale = []
		{
			float Value = 1.f;
#if !UE_BUILD_SHIPPING
			FParse::Value(FCommandLine::Get(), TEXT("AIBTimeScale="), Value);
#endif
			return FMath::Clamp(Value, 0.05f, 4.f);
		}();
		return Scale;
	}
}

AAIBGameMode::AAIBGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = AAIBGameState::StaticClass();
	PlayerStateClass = AAIBPlayerState::StaticClass();
	PlayerControllerClass = AAIBPlayerController::StaticClass();
	DefaultPawnClass = AAIBCharacter::StaticClass();
	bUseSeamlessTravel = false;
	bDelayedStart = false;
	InactivePlayerStateLifeSpan = 600.f;
}

AAIBGameState* AAIBGameMode::GetAIBGameState() const
{
	return GetGameState<AAIBGameState>();
}

UAIBWorldSubsystem* AAIBGameMode::WorldSys() const
{
	return GetWorld()->GetSubsystem<UAIBWorldSubsystem>();
}

float AAIBGameMode::Now() const
{
	const AAIBGameState* GS = GetAIBGameState();
	return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void AAIBGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	AutoBots = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bots"), 0), 0, AIB::MaxPlayers - 1);
	bAutoStart = UGameplayStatics::HasOption(Options, TEXT("AutoStart"));
	bAutoReady = bAutoStart || UGameplayStatics::HasOption(Options, TEXT("AutoReady"));
	Random.Initialize(FDateTime::Now().GetTicks() & 0x7FFFFFFF);
	if (GameSession)
	{
		GameSession->MaxPlayers = 8; // 5 champions + room for spectators
	}
}

FString AAIBGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	// "Name" is overwritten with the online nickname (a machine id on LAN); the menu's champion name wins.
	const FString Champion = UGameplayStatics::ParseOption(Options, TEXT("Champion")).Left(16);
	if (!Champion.IsEmpty() && NewPlayerController)
	{
		ChangeName(NewPlayerController, Champion, false);
	}
	return Error;
}

void AAIBGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	if (AAIBGameState* GS = GetAIBGameState())
	{
		GS->MatchSeed = Random.RandRange(1, 1 << 30);
	}
	EnterPhase(EAIBPhase::Lobby);
	for (int32 Index = 0; Index < AutoBots; ++Index)
	{
		const int32 Slot = FindFreeSlot();
		if (Slot != INDEX_NONE)
		{
			SpawnBot(Slot, FString(), false);
		}
	}
}

void AAIBGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}
	// Seats are held by humans (or their ghosts); bots give way to joining humans.
	int32 HumanSeats = 0;
	if (const AAIBGameState* GS = GetAIBGameState())
	{
		for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
		{
			if (!PS->bBot || PS->bDisconnected)
			{
				++HumanSeats;
			}
		}
		if (!GS->bMatchInProgress && HumanSeats >= AIB::MaxPlayers)
		{
			ErrorMessage = TEXT("This lobby is full (5/5 champions).");
		}
	}
}

void AAIBGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AAIBPlayerController* PC = Cast<AAIBPlayerController>(NewPlayer);
	AAIBPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AAIBPlayerState>() : nullptr;
	AAIBGameState* GS = GetAIBGameState();
	if (!PC || !PS || !GS)
	{
		return;
	}
	PS->bHost = PC->IsLocalController();
	PS->bBot = false;

	if (TryReclaimSeat(PC))
	{
		return;
	}

	if (!GS->bMatchInProgress)
	{
		PS->Slot = INDEX_NONE;
		if (!SeatPlayer(PS))
		{
			UE_LOG(LogAIB, Warning, TEXT("No seat for %s, spectating."), *PS->GetPlayerName());
		}
		else
		{
			SystemChat(FText::Format(LOCTEXT("Joined", "{0} joined as champion of {1}."), FText::FromString(PS->GetPlayerName()), PS->GetCommunityName()));
			Cue(TEXT("SFX_Join"));
		}
	}
	else
	{
		PS->Slot = INDEX_NONE;
		PS->Status = EAIBPlayerStatus::Eliminated;
		SystemChat(FText::Format(LOCTEXT("Spectator", "{0} is watching the trials."), FText::FromString(PS->GetPlayerName())));
	}
}

bool AAIBGameMode::TryReclaimSeat(AAIBPlayerController* PC)
{
	AAIBPlayerState* PS = PC->GetPlayerState<AAIBPlayerState>();
	AAIBGameState* GS = GetAIBGameState();
	// A ghost holding a seat for a disconnected human with the same name (or restored inactive state) gives it back.
	for (AAIBPlayerState* Seated : GS->GetSeatedPlayers())
	{
		if (!Seated->bDisconnected || !Seated->bBot)
		{
			continue;
		}
		const bool bSameSlot = PS->Slot != INDEX_NONE && PS->Slot == Seated->Slot;
		const bool bSameName = Seated->GetPlayerName().Equals(PS->GetPlayerName(), ESearchCase::IgnoreCase);
		if (!bSameSlot && !bSameName)
		{
			continue;
		}
		AAIBBotController* Ghost = Cast<AAIBBotController>(Seated->GetOwner());
		// Restore the freshest state from the ghost.
		const FString Name = PS->GetPlayerName();
		Seated->CopyProperties(PS);
		PS->SetPlayerName(Name);
		PS->bBot = false;
		PS->bDisconnected = false;
		PS->ReconnectDeadline = 0.f;
		PS->bHost = PC->IsLocalController();
		APawn* Pawn = Ghost ? Ghost->GetPawn() : nullptr;
		if (Ghost)
		{
			Ghost->UnPossess();
			Ghost->Destroy();
		}
		if (Pawn)
		{
			PC->Possess(Pawn);
		}
		SystemChat(FText::Format(LOCTEXT("Reconnected", "{0} reconnected."), FText::FromString(Name)));
		Cue(TEXT("SFX_Join"));
		return true;
	}
	return false;
}

void AAIBGameMode::Logout(AController* Exiting)
{
	AAIBPlayerState* PS = Exiting ? Exiting->GetPlayerState<AAIBPlayerState>() : nullptr;
	AAIBGameState* GS = GetAIBGameState();
	if (PS && GS && !PS->bBot && PS->Slot != INDEX_NONE)
	{
		const bool bInMatch = GS->bMatchInProgress;
		const bool bStillCompeting = !bInMatch || PS->IsActiveChampion();
		if (bStillCompeting && GS->Phase != EAIBPhase::Results)
		{
			// Keep the seat with a ghost while they have time to come back.
			const FString Name = PS->GetPlayerName();
			const int32 Slot = PS->Slot;
			APawn* Pawn = Exiting->GetPawn();
			AAIBBotController* Ghost = SpawnBot(INDEX_NONE, Name, true);
			if (Ghost)
			{
				AAIBPlayerState* GhostPS = Ghost->GetPlayerState<AAIBPlayerState>();
				PS->CopyProperties(GhostPS);
				GhostPS->SetPlayerName(Name);
				GhostPS->Slot = Slot;
				GhostPS->bBot = true;
				GhostPS->bReady = false;
				GhostPS->bDisconnected = true;
				GhostPS->ReconnectDeadline = Now() + (bInMatch ? AIB::StageReconnect : AIB::LobbyReconnect);
				if (Pawn)
				{
					Exiting->UnPossess();
					Ghost->Possess(Pawn);
				}
			}
			PS->Slot = INDEX_NONE;
			SystemChat(FText::Format(LOCTEXT("Disconnected", "{0} disconnected. Waiting {1} seconds for them to return."),
				FText::FromString(Name), FText::AsNumber(FMath::RoundToInt(bInMatch ? AIB::StageReconnect : AIB::LobbyReconnect))));
		}
		Cue(TEXT("SFX_Leave"));
	}
	Super::Logout(Exiting);
}

// ---------------------------------------------------------------------------------------------------------------------
// Seating & bots
// ---------------------------------------------------------------------------------------------------------------------

int32 AAIBGameMode::FindFreeSlot() const
{
	const AAIBGameState* GS = GetAIBGameState();
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		if (!GS->GetPlayerBySlot(Slot))
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

bool AAIBGameMode::SeatPlayer(AAIBPlayerState* PS)
{
	int32 Slot = FindFreeSlot();
	if (Slot == INDEX_NONE)
	{
		// Replace a bot (never a disconnected human's ghost).
		for (AAIBPlayerState* Seated : GetAIBGameState()->GetSeatedPlayers())
		{
			if (Seated->bBot && !Seated->bDisconnected)
			{
				Slot = Seated->Slot;
				RemoveBot(Cast<AAIBBotController>(Seated->GetOwner()));
				break;
			}
		}
	}
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	PS->Slot = Slot;
	PS->ResetForNewMatch();
	PS->bReady = false;
	PS->ForceNetUpdate();
	return true;
}

AAIBBotController* AAIBGameMode::SpawnBot(int32 Slot, const FString& Name, bool bGhost)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAIBBotController* Bot = GetWorld()->SpawnActor<AAIBBotController>(AAIBBotController::StaticClass(), FTransform::Identity, Params);
	if (!Bot)
	{
		return nullptr;
	}
	AAIBPlayerState* PS = Bot->GetPlayerState<AAIBPlayerState>();
	if (!PS)
	{
		Bot->Destroy();
		return nullptr;
	}
	PS->SetIsABot(true);
	PS->bBot = true;
	PS->Slot = Slot;
	PS->ResetForNewMatch();
	PS->BotTemper = Random.FRandRange(0.1f, 0.9f);
	Bot->Skill = Random.FRandRange(0.45f, 0.85f);
	if (Name.IsEmpty())
	{
		TArray<FString> Taken;
		for (AAIBPlayerState* Seated : GetAIBGameState()->GetSeatedPlayers())
		{
			Taken.Add(Seated->GetPlayerName());
		}
		PS->SetPlayerName(AIB::RandomBotName(Random, Taken));
	}
	else
	{
		PS->SetPlayerName(Name);
	}
	PS->bReady = !bGhost;
	if (!bGhost && Slot != INDEX_NONE)
	{
		SystemChat(FText::Format(LOCTEXT("BotJoined", "{0} (AI) takes the seat of {1}."), FText::FromString(PS->GetPlayerName()), PS->GetCommunityName()));
	}
	return Bot;
}

void AAIBGameMode::RemoveBot(AAIBBotController* Bot)
{
	if (!Bot)
	{
		return;
	}
	if (APawn* Pawn = Bot->GetPawn())
	{
		Pawn->Destroy();
	}
	if (AAIBPlayerState* PS = Bot->GetPlayerState<AAIBPlayerState>())
	{
		PS->Slot = INDEX_NONE;
	}
	Bot->Destroy();
}

void AAIBGameMode::RequestReady(AAIBPlayerController* PC, bool bReady)
{
	AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	const AAIBGameState* GS = GetAIBGameState();
	if (!PS || PS->Slot == INDEX_NONE || GS->bMatchInProgress)
	{
		return;
	}
	PS->bReady = bReady;
	PS->ForceNetUpdate();
	Cue(bReady ? TEXT("SFX_Ready") : TEXT("SFX_UI_Back"));
}

void AAIBGameMode::RequestAddBot(AAIBPlayerController* PC)
{
	const AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || !PS->bHost || GetAIBGameState()->bMatchInProgress)
	{
		return;
	}
	const int32 Slot = FindFreeSlot();
	if (Slot != INDEX_NONE)
	{
		SpawnBot(Slot, FString(), false);
	}
}

void AAIBGameMode::RequestFillBots(AAIBPlayerController* PC)
{
	const AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || !PS->bHost || GetAIBGameState()->bMatchInProgress)
	{
		return;
	}
	for (int32 Slot = FindFreeSlot(); Slot != INDEX_NONE; Slot = FindFreeSlot())
	{
		if (!SpawnBot(Slot, FString(), false))
		{
			break;
		}
	}
}

void AAIBGameMode::RequestRemoveSeat(AAIBPlayerController* PC, int32 Slot)
{
	const AAIBPlayerState* HostPS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	AAIBGameState* GS = GetAIBGameState();
	if (!HostPS || !HostPS->bHost || GS->bMatchInProgress)
	{
		return;
	}
	AAIBPlayerState* Target = GS->GetPlayerBySlot(Slot);
	if (!Target || Target == HostPS)
	{
		return;
	}
	if (Target->bBot)
	{
		RemoveBot(Cast<AAIBBotController>(Target->GetOwner()));
	}
	else if (APlayerController* Kicked = Cast<APlayerController>(Target->GetOwner()))
	{
		if (GameSession)
		{
			GameSession->KickPlayer(Kicked, LOCTEXT("Kicked", "The host removed you from the lobby."));
		}
	}
}

void AAIBGameMode::RequestSkipCinematic(AAIBPlayerController* PC)
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || GS->Phase != EAIBPhase::ArrivalCinematic)
	{
		return;
	}
	PS->bWantsSkipCinematic = true;
	// Skip once every connected human asked.
	for (APlayerState* Other : GS->PlayerArray)
	{
		const AAIBPlayerState* O = Cast<AAIBPlayerState>(Other);
		if (O && !O->bBot && !O->bWantsSkipCinematic)
		{
			return;
		}
	}
	GS->PhaseEndTime = FMath::Min(GS->PhaseEndTime, Now() + 0.5f);
}

void AAIBGameMode::RequestReturnToLobby(AAIBPlayerController* PC)
{
	const AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (PS && PS->bHost && GetAIBGameState()->Phase == EAIBPhase::Results)
	{
		ResetToLobby();
	}
}

bool AAIBGameMode::LobbyIsReady() const
{
	const AAIBGameState* GS = GetAIBGameState();
	const TArray<AAIBPlayerState*> Seated = GS->GetSeatedPlayers();
	if (Seated.Num() < AIB::MaxPlayers)
	{
		return false;
	}
	for (const AAIBPlayerState* PS : Seated)
	{
		if (!PS->bReady || PS->bDisconnected)
		{
			return false;
		}
	}
	return true;
}

void AAIBGameMode::TickLobby()
{
	AAIBGameState* GS = GetAIBGameState();
	if (bAutoReady && GS->Phase == EAIBPhase::Lobby)
	{
		for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
		{
			PS->bReady = true;
		}
	}
	if (bAutoStart && GS->Phase == EAIBPhase::Lobby)
	{
		for (int32 Slot = FindFreeSlot(); Slot != INDEX_NONE; Slot = FindFreeSlot())
		{
			SpawnBot(Slot, FString(), false);
		}
	}

	if (GS->Phase == EAIBPhase::Lobby && LobbyIsReady())
	{
		// No banner: the lobby panel shows the big countdown itself.
		EnterPhase(EAIBPhase::LobbyCountdown);
		Cue(TEXT("SFX_Ready"));
	}
	else if (GS->Phase == EAIBPhase::LobbyCountdown && !LobbyIsReady())
	{
		EnterPhase(EAIBPhase::Lobby);
		Announce(LOCTEXT("Cancelled", "COUNTDOWN CANCELLED"), LOCTEXT("CancelledSub", "A champion is no longer ready"), FLinearColor(1.f, 0.3f, 0.3f));
		Cue(TEXT("SFX_Wrong"));
	}
}

void AAIBGameMode::BeginMatch()
{
	AAIBGameState* GS = GetAIBGameState();
	GS->bMatchInProgress = true;
	GS->MatchSeed = Random.RandRange(1, 1 << 30);
	GS->Ending = EAIBEnding::None;
	GS->ChampionSlot = INDEX_NONE;
	GS->Finalists.Reset();
	GS->ChoiceOrder.Reset();
	GS->Choices.Reset();
	GS->RaceWinnerSlot = INDEX_NONE;
	GS->Stage1Last = FAIBStage1Round();
	GS->Stage2Last.Reset();
	for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
	{
		PS->ResetForNewMatch();
		PS->bReady = true;
	}
	BotTrust.Reset();
	Cue(TEXT("VO_Lobby_Ready"));

#if !UE_BUILD_SHIPPING
	// Development: -AIBJump=stage1|stage2|race|final starts straight at a trial (humans are eliminated first).
	FString Jump;
	if (FParse::Value(FCommandLine::Get(), TEXT("AIBJump="), Jump))
	{
		const int32 Keep = Jump == TEXT("stage2") ? AIB::Stage1Advance : (Jump == TEXT("race") || Jump == TEXT("final")) ? AIB::Stage2Advance : AIB::MaxPlayers;
		TArray<AAIBPlayerState*> Active = ActivePlayers();
		Active.StableSort([](const AAIBPlayerState& A, const AAIBPlayerState& B) { return !A.bBot && B.bBot; });
		while (Active.Num() > Keep)
		{
			Eliminate(Active[0], Keep == AIB::Stage1Advance ? 1 : 2, false);
			Active.RemoveAt(0);
		}
		SpawnChampionsAtLanding();
		EAIBPhase Target = EAIBPhase::Stage1Briefing;
		if (Jump == TEXT("stage2"))
		{
			Target = EAIBPhase::Stage2Briefing;
		}
		else if (Jump == TEXT("race"))
		{
			Target = EAIBPhase::Stage3Briefing;
		}
		else if (Jump == TEXT("final"))
		{
			GS->ChoiceOrder.Reset();
			for (AAIBPlayerState* PS : ActivePlayers())
			{
				GS->ChoiceOrder.Add(PS->Slot);
			}
			GS->Finalists = GS->ChoiceOrder;
			GS->RaceWinnerSlot = GS->ChoiceOrder.Num() > 0 ? GS->ChoiceOrder[0] : INDEX_NONE;
			Target = EAIBPhase::FinalBriefing;
		}
		UE_LOG(LogAIB, Log, TEXT("Dev jump to %s"), *Jump);
		FadeToPhase(Target);
		return;
	}
#endif
	EnterPhase(EAIBPhase::ArrivalCinematic);
}

void AAIBGameMode::ResetToLobby()
{
	AAIBGameState* GS = GetAIBGameState();
	Stage3_Cleanup();
	DestroyAllPawns();
	// Drop ghosts of players who never came back.
	for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
	{
		if (PS->bDisconnected)
		{
			RemoveBot(Cast<AAIBBotController>(PS->GetOwner()));
			continue;
		}
		PS->ResetForNewMatch();
		PS->bReady = PS->bBot;
	}
	// Spectators who joined mid-match get seats now.
	for (APlayerState* Raw : GS->PlayerArray)
	{
		AAIBPlayerState* PS = Cast<AAIBPlayerState>(Raw);
		if (PS && !PS->bBot && PS->Slot == INDEX_NONE)
		{
			SeatPlayer(PS);
		}
	}
	GS->bMatchInProgress = false;
	GS->Ending = EAIBEnding::None;
	GS->ChampionSlot = INDEX_NONE;
	GS->Finalists.Reset();
	GS->ChoiceOrder.Reset();
	GS->ChooserIndex = INDEX_NONE;
	GS->Choices.Reset();
	GS->RaceWinnerSlot = INDEX_NONE;
	GS->SwitchProgress.Reset();
	GS->RaceDistance.Reset();
	GS->Stage1Round = 0;
	GS->Stage2Round = 0;
	EnterPhase(EAIBPhase::Lobby);
	Announce(LOCTEXT("BackToLobby", "THE BEACON CALLS AGAIN"), LOCTEXT("BackToLobbySub", "Ready up for another match"));
}

void AAIBGameMode::HandleGhosts()
{
	AAIBGameState* GS = GetAIBGameState();
	for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
	{
		if (!PS->bDisconnected || Now() < PS->ReconnectDeadline)
		{
			continue;
		}
		const FString Name = PS->GetPlayerName();
		if (!GS->bMatchInProgress)
		{
			// Lobby: free the seat.
			RemoveBot(Cast<AAIBBotController>(PS->GetOwner()));
			SystemChat(FText::Format(LOCTEXT("SeatFreed", "{0} did not return. Their seat is open."), FText::FromString(Name)));
			continue;
		}
		PS->bDisconnected = false;
		PS->ReconnectDeadline = 0.f;
		if (PS->IsActiveChampion())
		{
			Eliminate(PS, 0, true);
			SystemChat(FText::Format(LOCTEXT("Forfeit", "{0} did not return and forfeits."), FText::FromString(Name)));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Phase machine
// ---------------------------------------------------------------------------------------------------------------------

float AAIBGameMode::DefaultDuration(EAIBPhase Phase) const
{
	switch (Phase)
	{
	case EAIBPhase::LobbyCountdown: return AIB::LobbyCountdown;
	case EAIBPhase::ArrivalCinematic: return AIB::ArrivalCinematic;
	case EAIBPhase::IslandExplore: return AIB::IslandExplore;
	case EAIBPhase::Stage1Briefing: return AIB::Stage1Briefing;
	case EAIBPhase::Stage1RoundIntro: return AIB::Stage1RoundIntro;
	case EAIBPhase::Stage1Selection: return AIB::Stage1Selection;
	case EAIBPhase::Stage1Reveal: return AIB::Stage1Reveal;
	case EAIBPhase::Stage1Results: return AIB::Stage1Results;
	case EAIBPhase::Stage1Elimination: return AIB::StageElimination;
	case EAIBPhase::Stage2Briefing: return AIB::Stage2Briefing;
	case EAIBPhase::Stage2Communication: return AIB::Stage2Communication;
	case EAIBPhase::Stage2Guess: return AIB::Stage2Guess;
	case EAIBPhase::Stage2Results: return AIB::Stage2Results;
	case EAIBPhase::Stage2Elimination: return AIB::StageElimination;
	case EAIBPhase::Stage3Briefing: return AIB::Stage3Briefing;
	case EAIBPhase::Stage3Countdown: return AIB::Stage3Countdown;
	case EAIBPhase::Stage3Race: return AIB::Stage3Race;
	case EAIBPhase::Stage3Finish: return AIB::Stage3Finish;
	case EAIBPhase::FinalBriefing: return AIB::FinalBriefing;
	case EAIBPhase::FinalChoice: return AIB::FinalSelect;
	case EAIBPhase::FinalResolve: return AIB::FinalResolve;
	case EAIBPhase::EndingCinematic: return AIB::EndingCinematic;
	default: return 0.f;
	}
}

void AAIBGameMode::EnterPhase(EAIBPhase Phase, float DurationOverride)
{
	AAIBGameState* GS = GetAIBGameState();
	const float Duration = (DurationOverride >= 0.f ? DurationOverride : DefaultDuration(Phase)) * PhaseTimeScale();
	bPhaseTimed = Duration > 0.f;
	PendingPhase = EAIBPhase::None;
	GS->ServerSetPhase(Phase, Duration);
	UE_LOG(LogAIB, Log, TEXT("Phase -> %s (%.1fs)"), *UEnum::GetValueAsString(Phase), Duration);

	switch (Phase)
	{
	case EAIBPhase::Lobby:
		Cue(TEXT("MUS_Lobby"));
		break;
	case EAIBPhase::ArrivalCinematic:
		Cue(TEXT("MUS_Arrival"));
		Cue(TEXT("SFX_ShipHorn"));
		for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
		{
			PS->bWantsSkipCinematic = false;
		}
		break;
	case EAIBPhase::IslandExplore:
	{
		SpawnChampionsAtLanding();
		Cue(TEXT("VO_Explore"));
		Cue(TEXT("MUS_Explore"));
		Announce(LOCTEXT("ExploreTitle", "THE LAST ISLAND"), LOCTEXT("ExploreSub", "Explore. The first trial begins soon."), FLinearColor(0.6f, 1.f, 0.5f));
		// Bots stroll around the landing area.
		TArray<FVector> Points;
		if (AAIBLandingSite* Landing = WorldSys() ? WorldSys()->GetLanding() : nullptr)
		{
			for (int32 Index = 0; Index < 12; ++Index)
			{
				const FTransform Spawn = Landing->GetDisembarkSpawn(Index % AIB::MaxPlayers);
				Points.Add(Spawn.GetLocation() + Spawn.GetRotation().GetForwardVector() * Random.FRandRange(400.f, 2500.f) + FVector(Random.FRandRange(-900.f, 900.f), Random.FRandRange(-900.f, 900.f), 0.f));
			}
		}
		SetBotWander(Points, FBox(FVector(-1e7), FVector(1e7)));
		break;
	}
	case EAIBPhase::Stage1Briefing:
		Stage1_Enter();
		break;
	case EAIBPhase::Stage1Selection:
		Cue(TEXT("VO_Stage1_Round"));
		ScheduleBotActions(1.5f, 8.f);
		break;
	case EAIBPhase::Stage2Briefing:
		Stage2_Enter();
		break;
	case EAIBPhase::Stage2Communication:
		ScheduleBotActions(2.f, 24.f);
		break;
	case EAIBPhase::Stage2Guess:
		Cue(TEXT("VO_Stage2_Guess"));
		ScheduleBotActions(1.f, 7.f);
		break;
	case EAIBPhase::Stage3Briefing:
		Stage3_Enter();
		break;
	case EAIBPhase::Stage3Countdown:
		Cue(TEXT("VO_Countdown"));
		break;
	case EAIBPhase::Stage3Race:
		Stage3_StartRace();
		break;
	case EAIBPhase::FinalBriefing:
		Final_Enter();
		break;
	case EAIBPhase::FinalChoice:
		Final_BeginChoice();
		break;
	case EAIBPhase::EndingCinematic:
		Ending_Enter();
		break;
	case EAIBPhase::Results:
		Results_Enter();
		break;
	default:
		break;
	}
}

void AAIBGameMode::FadeToPhase(EAIBPhase Phase)
{
	AAIBGameState* GS = GetAIBGameState();
	GS->FadeOutStartTime = Now();
	GS->FadeOutEndTime = Now() + AIB::FadeDuration;
	GS->ForceNetUpdate();
	PendingPhase = Phase;
	PendingPhaseTime = Now() + AIB::FadeDuration;
	bPhaseTimed = false;
}

void AAIBGameMode::OnPhaseTimerExpired()
{
	AAIBGameState* GS = GetAIBGameState();
	switch (GS->Phase)
	{
	case EAIBPhase::LobbyCountdown:
		BeginMatch();
		break;
	case EAIBPhase::ArrivalCinematic:
		EnterPhase(EAIBPhase::IslandExplore);
		break;
	case EAIBPhase::IslandExplore:
		FadeToPhase(EAIBPhase::Stage1Briefing);
		break;
	case EAIBPhase::Stage1Briefing:
		Stage1_BeginRound();
		break;
	case EAIBPhase::Stage1RoundIntro:
		EnterPhase(EAIBPhase::Stage1Selection);
		break;
	case EAIBPhase::Stage1Selection:
		Stage1_Resolve();
		break;
	case EAIBPhase::Stage1Reveal:
		EnterPhase(EAIBPhase::Stage1Results);
		break;
	case EAIBPhase::Stage1Results:
		Stage1_AfterResults();
		break;
	case EAIBPhase::Stage1Elimination:
		FadeToPhase(EAIBPhase::Stage2Briefing);
		break;
	case EAIBPhase::Stage2Briefing:
		Stage2_BeginRound();
		break;
	case EAIBPhase::Stage2Communication:
		EnterPhase(EAIBPhase::Stage2Guess);
		break;
	case EAIBPhase::Stage2Guess:
		Stage2_Resolve();
		break;
	case EAIBPhase::Stage2Results:
		Stage2_AfterResults();
		break;
	case EAIBPhase::Stage2Elimination:
		FadeToPhase(EAIBPhase::Stage3Briefing);
		break;
	case EAIBPhase::Stage3Briefing:
		EnterPhase(EAIBPhase::Stage3Countdown);
		break;
	case EAIBPhase::Stage3Countdown:
		EnterPhase(EAIBPhase::Stage3Race, GS->bRaceSuddenDeath ? AIB::Stage3SuddenDeathRace : AIB::Stage3Race);
		break;
	case EAIBPhase::Stage3Race:
		Stage3_TimeUp();
		break;
	case EAIBPhase::Stage3Finish:
		FadeToPhase(EAIBPhase::FinalBriefing);
		break;
	case EAIBPhase::FinalBriefing:
		EnterPhase(EAIBPhase::FinalChoice);
		break;
	case EAIBPhase::FinalChoice:
		// Timer ran out without a confirmed choice: Become the King by default.
		Final_Commit(EAIBFinalChoice::King, true);
		break;
	case EAIBPhase::FinalResolve:
		Final_AfterResolve();
		break;
	case EAIBPhase::EndingCinematic:
		EnterPhase(EAIBPhase::Results);
		break;
	default:
		break;
	}
}

void AAIBGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AAIBGameState* GS = GetAIBGameState();
	if (!GS || GS->Phase == EAIBPhase::None)
	{
		return;
	}

	HandleGhosts();

	++PerfFrames;
	const double RealNow = FPlatformTime::Seconds();
	if (PerfWindowStart <= 0.0)
	{
		PerfWindowStart = RealNow;
		PerfFrames = 0;
	}
	else if (RealNow - PerfWindowStart >= 10.0)
	{
		UE_LOG(LogAIB, Log, TEXT("Perf: %.1f fps during %s"), PerfFrames / (RealNow - PerfWindowStart), *UEnum::GetValueAsString(GS->Phase));
		PerfWindowStart = RealNow;
		PerfFrames = 0;
	}

	if (GS->Phase == EAIBPhase::Lobby || GS->Phase == EAIBPhase::LobbyCountdown)
	{
		TickLobby();
	}

	if (PendingPhase != EAIBPhase::None)
	{
		if (Now() >= PendingPhaseTime)
		{
			EnterPhase(PendingPhase);
		}
	}
	else if (bPhaseTimed && Now() >= GS->PhaseEndTime)
	{
		bPhaseTimed = false;
		OnPhaseTimerExpired();
	}

	// Early outs: everyone answered.
	if (GS->Phase == EAIBPhase::Stage1Selection || GS->Phase == EAIBPhase::Stage2Guess)
	{
		const TArray<int32>& Participants = GS->Phase == EAIBPhase::Stage1Selection ? GS->Stage1Participants : GS->Stage2Participants;
		bool bAll = Participants.Num() > 0;
		for (int32 Slot : Participants)
		{
			const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
			bAll &= PS && PS->bSubmitted;
		}
		if (bAll && GS->PhaseEndTime - Now() > 1.2f)
		{
			GS->PhaseEndTime = Now() + 1.2f;
		}
	}

	if (GS->Phase == EAIBPhase::Stage3Race)
	{
		Stage3_Tick(DeltaSeconds);
	}
	if (GS->Phase == EAIBPhase::FinalChoice)
	{
		Final_Tick();
	}
	if (GS->Phase == EAIBPhase::IslandExplore)
	{
		TickAcid();
	}
	TickBots(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------------------------------
// Pawns
// ---------------------------------------------------------------------------------------------------------------------

AController* AAIBGameMode::GetControllerForSlot(int32 Slot) const
{
	const AAIBPlayerState* PS = GetAIBGameState()->GetPlayerBySlot(Slot);
	return PS ? Cast<AController>(PS->GetOwner()) : nullptr;
}

AAIBCharacter* AAIBGameMode::GetCharacterForSlot(int32 Slot) const
{
	for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetSlot() == Slot)
		{
			return *It;
		}
	}
	return nullptr;
}

AAIBCharacter* AAIBGameMode::EnsureCharacter(int32 Slot, const FTransform& Transform)
{
	AController* Controller = GetControllerForSlot(Slot);
	if (!Controller)
	{
		return nullptr;
	}
	AAIBCharacter* Character = GetCharacterForSlot(Slot);
	if (!Character)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Character = GetWorld()->SpawnActor<AAIBCharacter>(AAIBCharacter::StaticClass(), Transform, Params);
	}
	if (Character && Character->GetController() != Controller)
	{
		if (Controller->GetPawn() && Controller->GetPawn() != Character)
		{
			Controller->UnPossess();
		}
		Controller->Possess(Character);
	}
	if (Character)
	{
		Character->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
		Controller->SetControlRotation(Transform.Rotator());
	}
	return Character;
}

void AAIBGameMode::TeleportSlot(int32 Slot, const FTransform& Transform)
{
	AAIBCharacter* Character = GetCharacterForSlot(Slot);
	if (!Character)
	{
		Character = EnsureCharacter(Slot, Transform);
	}
	if (!Character)
	{
		return;
	}
	Character->SetActorLocationAndRotation(Transform.GetLocation(), Transform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(Transform.Rotator());
		if (AAIBPlayerController* PC = Cast<AAIBPlayerController>(Controller))
		{
			PC->ClientFaceRotation(Transform.Rotator());
		}
	}
}

void AAIBGameMode::SpawnChampionsAtLanding()
{
	AAIBLandingSite* Landing = WorldSys() ? WorldSys()->GetLanding() : nullptr;
	for (AAIBPlayerState* PS : ActivePlayers())
	{
		const FTransform Spawn = Landing ? Landing->GetDisembarkSpawn(PS->Slot) : FTransform(FVector(AIBLayout::LandingBeach, 200.f));
		if (AAIBCharacter* Character = EnsureCharacter(PS->Slot, Spawn))
		{
			Character->MulticastRestore();
		}
	}
	SetAllMovementLocked(false);
}

void AAIBGameMode::DestroyAllPawns()
{
	TArray<AAIBCharacter*> Characters;
	for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
	{
		Characters.Add(*It);
	}
	for (AAIBCharacter* Character : Characters)
	{
		if (AController* Controller = Character->GetController())
		{
			Controller->UnPossess();
		}
		Character->Destroy();
	}
}

void AAIBGameMode::SetAllMovementLocked(bool bLocked)
{
	for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
	{
		It->SetMovementLocked(bLocked);
	}
}

void AAIBGameMode::TickAcid()
{
	AAIBLandingSite* Landing = WorldSys() ? WorldSys()->GetLanding() : nullptr;
	for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
	{
		AAIBCharacter* Character = *It;
		const float FeetZ = Character->GetActorLocation().Z - 90.f;
		const FVector2D XY(Character->GetActorLocation());
		const bool bOutside = !AIBLayout::IslandBounds.IsInside(XY);
		if ((FeetZ < AIBLayout::SeaLevel + 20.f || bOutside) && Landing && Character->GetSlot() != INDEX_NONE)
		{
			TeleportSlot(Character->GetSlot(), Landing->GetRespawn(Character->GetSlot()));
			if (AAIBPlayerController* PC = Cast<AAIBPlayerController>(Character->GetController()))
			{
				PC->ClientNotify(LOCTEXT("Acid", "The acid burns! Stay out of the water."), TEXT("SFX_Wrong"));
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

TArray<AAIBPlayerState*> AAIBGameMode::ActivePlayers() const
{
	return GetAIBGameState()->GetActivePlayers();
}

void AAIBGameMode::Eliminate(AAIBPlayerState* PS, int32 Stage, bool bForfeit)
{
	if (!PS || PS->Status != EAIBPlayerStatus::Active)
	{
		return;
	}
	PS->Status = bForfeit ? EAIBPlayerStatus::Forfeited : EAIBPlayerStatus::Eliminated;
	PS->EliminatedInStage = Stage;
	PS->ForceNetUpdate();
	if (AAIBCharacter* Character = GetCharacterForSlot(PS->Slot))
	{
		Character->MulticastEliminated();
	}
	UE_LOG(LogAIB, Log, TEXT("%s eliminated (stage %d%s)."), *PS->GetPlayerName(), Stage, bForfeit ? TEXT(", forfeit") : TEXT(""));

	// Forfeit during the race: the other finalist wins immediately.
	AAIBGameState* GS = GetAIBGameState();
	if (bForfeit && GS->Phase == EAIBPhase::Stage3Race && GS->Finalists.Contains(PS->Slot))
	{
		const int32 Other = GS->Finalists.IndexOfByPredicate([PS](int32 S) { return S != PS->Slot; });
		if (Other != INDEX_NONE)
		{
			Stage3_Claim(Other);
		}
	}
}

void AAIBGameMode::Announce(const FText& Title, const FText& Subtitle, const FLinearColor& Color)
{
	AAIBGameState* GS = GetAIBGameState();
	GS->BannerTitle = Title;
	GS->BannerSubtitle = Subtitle;
	GS->BannerColor = Color;
	GS->BannerTime = Now();
	GS->ForceNetUpdate();
}

void AAIBGameMode::Cue(FName CueId)
{
	if (AAIBGameState* GS = GetAIBGameState())
	{
		GS->MulticastCue(CueId);
	}
}

void AAIBGameMode::BroadcastChat(const FAIBChatMessage& Message)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AAIBPlayerController* PC = Cast<AAIBPlayerController>(It->Get());
		if (!PC)
		{
			continue;
		}
		if (Message.bSpectator)
		{
			// Spectator chatter never reaches champions still competing.
			const AAIBPlayerState* PS = PC->GetPlayerState<AAIBPlayerState>();
			if (PS && PS->IsActiveChampion() && GetAIBGameState()->bMatchInProgress)
			{
				continue;
			}
		}
		PC->ClientReceiveChat(Message);
	}
}

void AAIBGameMode::SystemChat(const FText& Text)
{
	FAIBChatMessage Message;
	Message.bSystem = true;
	Message.Text = Text.ToString();
	Message.ServerTime = Now();
	BroadcastChat(Message);
}

void AAIBGameMode::SubmitChat(AController* Controller, const FString& Text, int32 ToSlot, EAIBSymbol Claim)
{
	AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	const AAIBGameState* GS = GetAIBGameState();
	FString Clean = Text.TrimStartAndEnd().Left(160);
	if (!PS || Clean.IsEmpty())
	{
		return;
	}
	FAIBChatMessage Message;
	Message.FromSlot = PS->Slot;
	Message.FromName = PS->GetPlayerName();
	Message.Text = Clean;
	Message.ToSlot = ToSlot;
	Message.ClaimSymbol = Claim;
	Message.ServerTime = Now();
	Message.bSpectator = GS->bMatchInProgress && !PS->IsActiveChampion();

	// Free text claims: "Kestrel you have a spade" -> claim for bots to read.
	if (Message.ClaimSymbol == EAIBSymbol::None && !Message.bSpectator)
	{
		const EAIBSymbol Parsed = AIB::ParseSymbol(Clean);
		if (Parsed != EAIBSymbol::None)
		{
			for (const AAIBPlayerState* Other : GS->GetSeatedPlayers())
			{
				if (Other != PS && Clean.Contains(Other->GetPlayerName(), ESearchCase::IgnoreCase))
				{
					Message.ToSlot = Other->Slot;
					Message.ClaimSymbol = Parsed;
					break;
				}
			}
		}
	}
	if (GS->Phase == EAIBPhase::Stage2Communication && Message.ClaimSymbol != EAIBSymbol::None && Message.ToSlot != INDEX_NONE && !Message.bSpectator)
	{
		Stage2Claims.Add(Message.ToSlot, TPair<int32, EAIBSymbol>(PS->Slot, Message.ClaimSymbol));
	}
	BroadcastChat(Message);
}

#undef LOCTEXT_NAMESPACE
