// Alice in Botherland - per champion replicated state

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/AIBTypes.h"
#include "AIBPlayerState.generated.h"

UCLASS()
class ALICEINBOTHERLAND_API AAIBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AAIBPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;

	/** Community / seat index 0-4. INDEX_NONE while the player has no seat (lobby full = spectator). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Slot = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bBot = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bHost = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	EAIBPlayerStatus Status = EAIBPlayerStatus::Active;

	/** 1 or 2 when knocked out of a stage, 3 for the race, 0 otherwise. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 EliminatedInStage = 0;

	/** Number Game: secondary score (rounds won as the closest player). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Stage1Wins = 0;

	/** Number Game: lives shown as a secondary score, start at 3. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Lives = 3;

	/** Hidden Symbol: qualification points. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Stage2Points = 0;

	/** True once the player locked in an answer for the current input phase. The value itself stays on the server. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bSubmitted = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bDisconnected = false;

	/** Server time when a disconnected player forfeits. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float ReconnectDeadline = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	EAIBFinalChoice FinalChoice = EAIBFinalChoice::None;

	/** Seconds from race start to switch activation, -1 when not finished. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float RaceTime = -1.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bWantsSkipCinematic = false;

	/** Voice chat: the player's microphone is sending right now (drives the speaking icons). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bTalking = false;

	/** Island marker stones this champion has read (bit per stone). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 LoreFound = 0;

	// ---- Server only (never replicated) ----

	/** Hidden symbol for the current Stage 2 round. Intentionally NOT replicated so clients can't read their own. */
	EAIBSymbol HiddenSymbol = EAIBSymbol::None;
	int32 PendingNumber = INDEX_NONE;
	EAIBSymbol PendingGuess = EAIBSymbol::None;
	/** Bot personality 0..1 (used for number strategy, honesty and final choice). */
	float BotTemper = 0.5f;

	bool IsActiveChampion() const { return Slot != INDEX_NONE && Status == EAIBPlayerStatus::Active; }
	FLinearColor GetCommunityColor() const;
	FText GetCommunityName() const;
	void ResetForNewMatch();
};
