// Alice in Botherland - replicated match state

#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "World/AIBWorldSubsystem.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AAIBGameState::AAIBGameState()
{
	SetNetUpdateFrequency(30.f);
}

void AAIBGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAIBGameState, Phase);
	DOREPLIFETIME(AAIBGameState, RoomCode);
	DOREPLIFETIME(AAIBGameState, RoomCodeInternet);
	DOREPLIFETIME(AAIBGameState, bAITestMode);
	DOREPLIFETIME(AAIBGameState, PhaseStartTime);
	DOREPLIFETIME(AAIBGameState, PhaseEndTime);
	DOREPLIFETIME(AAIBGameState, PhaseSerial);
	DOREPLIFETIME(AAIBGameState, FadeOutStartTime);
	DOREPLIFETIME(AAIBGameState, FadeOutEndTime);
	DOREPLIFETIME(AAIBGameState, bMatchInProgress);
	DOREPLIFETIME(AAIBGameState, MatchSeed);
	DOREPLIFETIME(AAIBGameState, BannerTitle);
	DOREPLIFETIME(AAIBGameState, BannerSubtitle);
	DOREPLIFETIME(AAIBGameState, BannerTime);
	DOREPLIFETIME(AAIBGameState, BannerColor);
	DOREPLIFETIME(AAIBGameState, Stage1Round);
	DOREPLIFETIME(AAIBGameState, bStage1SuddenDeath);
	DOREPLIFETIME(AAIBGameState, Stage1AtRisk);
	DOREPLIFETIME(AAIBGameState, Stage1Participants);
	DOREPLIFETIME(AAIBGameState, Stage1Last);
	DOREPLIFETIME(AAIBGameState, Stage2Round);
	DOREPLIFETIME(AAIBGameState, bStage2SuddenDeath);
	DOREPLIFETIME(AAIBGameState, Stage2Participants);
	DOREPLIFETIME(AAIBGameState, Stage2Last);
	DOREPLIFETIME(AAIBGameState, Finalists);
	DOREPLIFETIME(AAIBGameState, Stage2Ledger);
	DOREPLIFETIME(AAIBGameState, MatchStartTime);
	DOREPLIFETIME(AAIBGameState, Timeline);
	DOREPLIFETIME(AAIBGameState, Fallen);
	DOREPLIFETIME(AAIBGameState, FallStrikeAt);
	DOREPLIFETIME(AAIBGameState, bRaceSuddenDeath);
	DOREPLIFETIME(AAIBGameState, SwitchProgress);
	DOREPLIFETIME(AAIBGameState, RaceDistance);
	DOREPLIFETIME(AAIBGameState, RaceWinnerSlot);
	DOREPLIFETIME(AAIBGameState, GuardianLocation);
	DOREPLIFETIME(AAIBGameState, ChoiceOrder);
	DOREPLIFETIME(AAIBGameState, ChooserIndex);
	DOREPLIFETIME(AAIBGameState, PendingOption);
	DOREPLIFETIME(AAIBGameState, ConfirmEndTime);
	DOREPLIFETIME(AAIBGameState, Choices);
	DOREPLIFETIME(AAIBGameState, Ending);
	DOREPLIFETIME(AAIBGameState, ChampionSlot);
}

void AAIBGameState::BeginPlay()
{
	Super::BeginPlay();

	// Every machine assembles the island's gameplay set pieces locally (they are not replicated).
	if (UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>())
	{
		WorldSys->EnsureWorldActors();
	}
}

AAIBPlayerState* AAIBGameState::GetPlayerBySlot(int32 Slot) const
{
	if (Slot == INDEX_NONE)
	{
		return nullptr;
	}
	for (APlayerState* PS : PlayerArray)
	{
		AAIBPlayerState* AIBPS = Cast<AAIBPlayerState>(PS);
		if (AIBPS && AIBPS->Slot == Slot)
		{
			return AIBPS;
		}
	}
	return nullptr;
}

TArray<AAIBPlayerState*> AAIBGameState::GetSeatedPlayers() const
{
	TArray<AAIBPlayerState*> Result;
	for (APlayerState* PS : PlayerArray)
	{
		AAIBPlayerState* AIBPS = Cast<AAIBPlayerState>(PS);
		if (AIBPS && AIBPS->Slot != INDEX_NONE)
		{
			Result.Add(AIBPS);
		}
	}
	Result.Sort([](const AAIBPlayerState& A, const AAIBPlayerState& B) { return A.Slot < B.Slot; });
	return Result;
}

TArray<AAIBPlayerState*> AAIBGameState::GetActivePlayers() const
{
	TArray<AAIBPlayerState*> Result = GetSeatedPlayers();
	Result.RemoveAll([](const AAIBPlayerState* PS) { return !PS->IsActiveChampion(); });
	return Result;
}

int32 AAIBGameState::CountActive() const
{
	return GetActivePlayers().Num();
}

float AAIBGameState::GetPhaseRemaining() const
{
	return FMath::Max(0.f, PhaseEndTime - GetServerWorldTimeSeconds());
}

float AAIBGameState::GetPhaseElapsed() const
{
	return FMath::Max(0.f, GetServerWorldTimeSeconds() - PhaseStartTime);
}

float AAIBGameState::GetPhaseAlpha() const
{
	const float Duration = PhaseEndTime - PhaseStartTime;
	return Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(GetPhaseElapsed() / Duration, 0.f, 1.f) : 1.f;
}

int32 AAIBGameState::GetCurrentChooserSlot() const
{
	return ChoiceOrder.IsValidIndex(ChooserIndex) ? ChoiceOrder[ChooserIndex] : INDEX_NONE;
}

void AAIBGameState::MulticastCue_Implementation(FName CueId)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAIBAudioSubsystem* Audio = GI->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->PlayCue(CueId);
		}
	}
}

void AAIBGameState::ServerSetPhase(EAIBPhase NewPhase, float Duration)
{
	const EAIBPhase Old = Phase;
	Phase = NewPhase;
	PhaseStartTime = GetServerWorldTimeSeconds();
	PhaseEndTime = PhaseStartTime + Duration;
	++PhaseSerial;
	FadeOutStartTime = -1.f;
	FadeOutEndTime = -1.f;
	ForceNetUpdate();
	OnRep_Phase(Old);
}

void AAIBGameState::OnRep_Phase(EAIBPhase OldPhase)
{
	OnPhaseChanged.Broadcast(OldPhase, Phase);
	LastBroadcastPhase = Phase;
}
