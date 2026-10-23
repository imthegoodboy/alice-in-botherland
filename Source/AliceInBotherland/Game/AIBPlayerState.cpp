// Alice in Botherland - per champion replicated state

#include "Game/AIBPlayerState.h"
#include "Net/UnrealNetwork.h"

AAIBPlayerState::AAIBPlayerState()
{
	SetNetUpdateFrequency(20.f);
}

void AAIBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAIBPlayerState, Slot);
	DOREPLIFETIME(AAIBPlayerState, bTalking);
	DOREPLIFETIME(AAIBPlayerState, LoreFound);
	DOREPLIFETIME(AAIBPlayerState, bBot);
	DOREPLIFETIME(AAIBPlayerState, bHost);
	DOREPLIFETIME(AAIBPlayerState, bReady);
	DOREPLIFETIME(AAIBPlayerState, Status);
	DOREPLIFETIME(AAIBPlayerState, EliminatedInStage);
	DOREPLIFETIME(AAIBPlayerState, Stage1Wins);
	DOREPLIFETIME(AAIBPlayerState, Lives);
	DOREPLIFETIME(AAIBPlayerState, Stage2Points);
	DOREPLIFETIME(AAIBPlayerState, bSubmitted);
	DOREPLIFETIME(AAIBPlayerState, bDisconnected);
	DOREPLIFETIME(AAIBPlayerState, ReconnectDeadline);
	DOREPLIFETIME(AAIBPlayerState, FinalChoice);
	DOREPLIFETIME(AAIBPlayerState, RaceTime);
	DOREPLIFETIME(AAIBPlayerState, bWantsSkipCinematic);
}

void AAIBPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AAIBPlayerState* Other = Cast<AAIBPlayerState>(PlayerState))
	{
		Other->Slot = Slot;
		Other->bBot = bBot;
		Other->bHost = bHost;
		Other->bReady = bReady;
		Other->Status = Status;
		Other->EliminatedInStage = EliminatedInStage;
		Other->Stage1Wins = Stage1Wins;
		Other->Lives = Lives;
		Other->Stage2Points = Stage2Points;
		Other->bSubmitted = bSubmitted;
		Other->FinalChoice = FinalChoice;
		Other->RaceTime = RaceTime;
		Other->HiddenSymbol = HiddenSymbol;
		Other->PendingNumber = PendingNumber;
		Other->PendingGuess = PendingGuess;
		Other->BotTemper = BotTemper;
	}
}

void AAIBPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	// Called when a disconnected player comes back: restore everything from their inactive state.
	if (AAIBPlayerState* Old = Cast<AAIBPlayerState>(PlayerState))
	{
		Slot = Old->Slot;
		bBot = Old->bBot;
		bReady = Old->bReady;
		Status = Old->Status;
		EliminatedInStage = Old->EliminatedInStage;
		Stage1Wins = Old->Stage1Wins;
		Lives = Old->Lives;
		Stage2Points = Old->Stage2Points;
		bSubmitted = Old->bSubmitted;
		FinalChoice = Old->FinalChoice;
		RaceTime = Old->RaceTime;
		HiddenSymbol = Old->HiddenSymbol;
		PendingNumber = Old->PendingNumber;
		PendingGuess = Old->PendingGuess;
		bDisconnected = false;
		ReconnectDeadline = 0.f;
	}
}

FLinearColor AAIBPlayerState::GetCommunityColor() const
{
	return Slot == INDEX_NONE ? FLinearColor(0.6f, 0.6f, 0.6f) : AIB::Community(Slot).Color;
}

FText AAIBPlayerState::GetCommunityName() const
{
	return Slot == INDEX_NONE ? FText::GetEmpty() : AIB::Community(Slot).Name;
}

void AAIBPlayerState::ResetForNewMatch()
{
	bReady = bBot;
	Status = EAIBPlayerStatus::Active;
	EliminatedInStage = 0;
	Stage1Wins = 0;
	Lives = 3;
	Stage2Points = 0;
	bSubmitted = false;
	FinalChoice = EAIBFinalChoice::None;
	RaceTime = -1.f;
	bWantsSkipCinematic = false;
	LoreFound = 0;
	HiddenSymbol = EAIBSymbol::None;
	PendingNumber = INDEX_NONE;
	PendingGuess = EAIBSymbol::None;
}
