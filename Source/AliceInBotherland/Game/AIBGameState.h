// Alice in Botherland - replicated match state

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/AIBTypes.h"
#include "AIBGameState.generated.h"

class AAIBPlayerState;

UCLASS()
class ALICEINBOTHERLAND_API AAIBGameState : public AGameState
{
	GENERATED_BODY()

public:
	AAIBGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	// ---------------- Phase clock ----------------

	UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "AIB")
	EAIBPhase Phase = EAIBPhase::None;

	/** Server world time the phase started / ends. Compare with GetServerWorldTimeSeconds(). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float PhaseStartTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float PhaseEndTime = 0.f;

	/** Incremented on every phase change so clients can detect repeats of the same phase (e.g. Stage1Selection round 2). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 PhaseSerial = 0;

	/** When >= 0 the screen fades to black until FadeOutEndTime (a scene change is coming). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float FadeOutStartTime = -1.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float FadeOutEndTime = -1.f;

	/** True while a match is running (any phase after the lobby). Late joiners become spectators. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bMatchInProgress = false;

	/** Room code for friends on the same network (host's LAN address) and for internet play (public address). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FString RoomCode;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FString RoomCodeInternet;

	/** Host's test/showcase mode: empty seats are kept filled with AI champions. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bAITestMode = false;

	/** Seed shared with clients for deterministic cosmetic randomness (ship sway etc). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 MatchSeed = 0;

	// ---------------- Announcements ----------------

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FText BannerTitle;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FText BannerSubtitle;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float BannerTime = -100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FLinearColor BannerColor = FLinearColor::White;

	// ---------------- Stage 1 : Number Game ----------------

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Stage1Round = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bStage1SuddenDeath = false;

	/** Slots allowed to submit in the current round (every active champion). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<int32> Stage1Participants;

	/** Sudden death: everyone still picks (and moves the target), but only these tied champions can fall. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<int32> Stage1AtRisk;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FAIBStage1Round Stage1Last;

	// ---------------- Stage 2 : Hidden Symbol ----------------

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 Stage2Round = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bStage2SuddenDeath = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<int32> Stage2Participants;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<FAIBSymbolResult> Stage2Last;

	/** Who told whom what this round, true or false - filled in only when the symbols are revealed. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<FAIBClaimRecord> Stage2Ledger;

	// ---------------- Stage 3 : Race ----------------

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<int32> Finalists;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	bool bRaceSuddenDeath = false;

	/** Per finalist (same order as Finalists): progress along the road to the Guardian, 0..1. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<float> SwitchProgress;

	/** Per finalist: remaining distance to the guardian in metres. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<float> RaceDistance;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 RaceWinnerSlot = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	FVector GuardianLocation = FVector::ZeroVector;

	// ---------------- Final ----------------

	/** Slots in the order they choose. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<int32> ChoiceOrder;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 ChooserIndex = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	EAIBFinalChoice PendingOption = EAIBFinalChoice::None;

	/** Server time the confirmation window closes, 0 when not confirming. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float ConfirmEndTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<FAIBFinalRecord> Choices;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	EAIBEnding Ending = EAIBEnding::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	int32 ChampionSlot = INDEX_NONE;

	// ---------------- Match story ----------------

	/** Server time the match began (timeline times are relative to it). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float MatchStartTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<FAIBMatchEvent> Timeline;

	// ---------------- The Guardian's judgement (elimination scenes) ----------------

	/** Champions being judged in the current elimination phase, in speaking order. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	TArray<FAIBFallen> Fallen;

	/** Seconds into the elimination phase when the beam strikes. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "AIB")
	float FallStrikeAt = 0.f;

	// ---------------- Helpers ----------------

	AAIBPlayerState* GetPlayerBySlot(int32 Slot) const;
	TArray<AAIBPlayerState*> GetSeatedPlayers() const;
	TArray<AAIBPlayerState*> GetActivePlayers() const;
	int32 CountActive() const;
	float GetServerTime() const { return GetServerWorldTimeSeconds(); }
	float GetPhaseRemaining() const;
	float GetPhaseElapsed() const;
	float GetPhaseAlpha() const;
	int32 GetCurrentChooserSlot() const;
	bool IsFinalist(int32 Slot) const { return Finalists.Contains(Slot); }

	/** Broadcast a voice/sfx/music cue to everyone. Ids map to sounds in UAIBAudioSubsystem. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastCue(FName CueId);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPhaseChanged, EAIBPhase /*Old*/, EAIBPhase /*New*/);
	FOnPhaseChanged OnPhaseChanged;

	/** Server: set the phase and broadcast locally too (listen server doesn't get OnRep). */
	void ServerSetPhase(EAIBPhase NewPhase, float Duration);

protected:
	UFUNCTION()
	void OnRep_Phase(EAIBPhase OldPhase);

	EAIBPhase LastBroadcastPhase = EAIBPhase::None;
};
