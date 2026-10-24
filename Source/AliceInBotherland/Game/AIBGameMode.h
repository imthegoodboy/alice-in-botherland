// Alice in Botherland - server authority for the whole match (lobby -> trials -> ending)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Core/AIBTypes.h"
#include "Player/AIBPoses.h"
#include "AIBGameMode.generated.h"

class AAIBGameState;
class AAIBPlayerState;
class AAIBPlayerController;
class AAIBCharacter;
class AAIBVehicle;
class AAIBBotController;
class UAIBWorldSubsystem;

UCLASS()
class ALICEINBOTHERLAND_API AAIBGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AAIBGameMode();

	// AGameModeBase / AGameMode
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleMatchHasStarted() override;

	// ---------------- Requests from player controllers ----------------
	void RequestReady(AAIBPlayerController* PC, bool bReady);
	void RequestAddBot(AAIBPlayerController* PC);
	void RequestFillBots(AAIBPlayerController* PC);
	void RequestRemoveSeat(AAIBPlayerController* PC, int32 Slot);
	/** Host: start the trials (every seat filled and every champion ready). */
	void RequestStart(AAIBPlayerController* PC);
	/** Host: AI test mode keeps every empty seat filled with an AI champion. */
	void RequestSetAITestMode(AAIBPlayerController* PC, bool bOn);
	void DiscoverLore(AController* Controller, int32 Index);
	void SetTalking(AController* Controller, bool bTalking);
	void RequestSkipCinematic(AAIBPlayerController* PC);
	void RequestReturnToLobby(AAIBPlayerController* PC);
	void SubmitNumber(AController* Controller, int32 Number);
	void SetDraftNumber(AController* Controller, int32 Number);
	void SubmitGuess(AController* Controller, EAIBSymbol Guess);
	void SubmitChat(AController* Controller, const FString& Text, int32 ToSlot, EAIBSymbol Claim);
	void SubmitFinalSelect(AController* Controller, EAIBFinalChoice Choice);
	void SubmitFinalConfirm(AController* Controller, bool bConfirm);

	AAIBGameState* GetAIBGameState() const;

protected:
	// ---------------- Phase machine ----------------
	void EnterPhase(EAIBPhase Phase, float DurationOverride = -1.f);
	void FadeToPhase(EAIBPhase Phase);
	void OnPhaseTimerExpired();
	float DefaultDuration(EAIBPhase Phase) const;

	// ---------------- Lobby & seating ----------------
	int32 FindFreeSlot() const;
	bool SeatPlayer(AAIBPlayerState* PS);
	AAIBBotController* SpawnBot(int32 Slot, const FString& Name, bool bGhost);
	void RemoveBot(AAIBBotController* Bot);
	bool LobbyIsReady() const;
	void TickLobby();
	void StartCountdown();
	/** Room codes from the host's LAN and public addresses. */
	void UpdateRoomCodes();
	float NextRoomCodeTry = 0.f;
	bool bFetchingPublicAddress = false;
	void BeginMatch();
	void ResetToLobby();
	void HandleGhosts();
	bool TryReclaimSeat(AAIBPlayerController* PC);

	// ---------------- Pawns ----------------
	AController* GetControllerForSlot(int32 Slot) const;
	AAIBCharacter* GetCharacterForSlot(int32 Slot) const;
	AAIBCharacter* EnsureCharacter(int32 Slot, const FTransform& Transform);
	void TeleportSlot(int32 Slot, const FTransform& Transform);
	void SpawnChampionsAtLanding();
	void DestroyAllPawns();
	void TickAcid();

	// ---------------- Trials (AIBGameMode_Trials.cpp) ----------------
	void Stage1_Enter();
	void Stage1_BeginRound();
	void Stage1_Resolve();
	void Stage1_AfterResults();
	void Stage2_Enter();
	void Stage2_BeginRound();
	/** Sends each viewer the symbols they may see (only to OnlyFor when given, e.g. after a reconnect). */
	void Stage2_SendSymbolViews(AAIBPlayerController* OnlyFor = nullptr);
	void Stage2_Resolve();
	void Stage2_AfterResults();

	// ---------------- Finale (AIBGameMode_Finale.cpp) ----------------
	void Stage3_Enter();
	void Stage3_PlaceVehicles(bool bSuddenDeath);
	void Stage3_StartRace();
	void Stage3_Tick(float DeltaSeconds);
	void Stage3_TimeUp();
	void Stage3_Claim(int32 FinalistIndex);
	void Stage3_Cleanup();
	void Final_Enter();
	void Final_BeginChoice();
	void Final_Tick();
	/** Selection timer ran out: the pending option, else Become the King (or pass on if the chooser is gone). */
	void Final_TimeUp();
	/** The chooser left: the choice passes to the next finalist, or the final ends. */
	void Final_PassOn();
	void Final_Commit(EAIBFinalChoice Choice, bool bAuto);
	void Final_AfterResolve();
	void Ending_Enter();
	void Results_Enter();
	FAIBMatchRecord BuildMatchRecord() const;

	// ---------------- Bots (AIBGameMode_Bots.cpp) ----------------
	void TickBots(float DeltaSeconds);
	int32 BotPickNumber(const AAIBPlayerState* Bot) const;
	void BotChat(AAIBPlayerState* Bot);
	EAIBSymbol BotGuess(const AAIBPlayerState* Bot) const;
	void ScheduleBotActions(float WindowStart, float WindowEnd);
	void UpdateBotTrust();
	void SetBotWander(const TArray<FVector>& Points, const FBox& Bounds);

	// ---------------- Helpers ----------------
	/** Out of the trials. bJudged: the body stays standing for the judgement scene instead of dropping at once. */
	void Eliminate(AAIBPlayerState* PS, int32 Stage, bool bForfeit, bool bJudged = false);
	void Announce(const FText& Title, const FText& Subtitle, const FLinearColor& Color = FLinearColor::White);
	void Cue(FName CueId);
	void BroadcastChat(const FAIBChatMessage& Message);
	void SystemChat(const FText& Text);
	TArray<AAIBPlayerState*> ActivePlayers() const;
	UAIBWorldSubsystem* WorldSys() const;
	float Now() const;
	void SetAllMovementLocked(bool bLocked);
	/** Adds a moment to the match story. */
	void LogEvent(EAIBMatchEventKind Kind, int32 Slot, int32 Value = 0);
	/** Story poses for the moment a phase shows (grief before the judgement, the chooser weighing the choice). */
	void PlayPhasePoses(EAIBPhase Phase);
	void PoseSlot(int32 Slot, EAIBPose Pose);
	void StopAllPoses();
	/**
	 * The Guardian's judgement of the champions eliminated this trial (PendingFallen): each says their last words, the
	 * red beam strikes, they fall, the Guardian speaks. Fills the game state and schedules it; returns the phase length.
	 */
	float BeginJudgement(int32 Stage, bool bTrialOver);
	/** Runs Action after Seconds of (scaled) phase time; cancelled by ClearJudgement. */
	void Later(float Seconds, TFunction<void()> Action);
	void ClearJudgement();
	/** Eliminated this trial and waiting for the judgement scene. */
	TArray<int32> PendingFallen;
	/** Last words already spoken this match (each is said once). */
	TArray<int32> UsedLastWords;
	TArray<FTimerHandle> JudgementTimers;
	/** Champions still competing can't hear the eliminated (who see every hidden symbol). */
	void UpdateVoiceMutes();
	/** Name mentioned as a whole word in a chat line ("Ann" is not in "Joanna"). */
	static bool MentionsName(const FString& Text, const FString& Name);
	/** Turns a departing human's seat into a ghost (reclaimable when bReclaimable) and returns its state. */
	AAIBPlayerState* MakeGhost(AController* Exiting, AAIBPlayerState* PS, bool bReclaimable);
	float NextVoiceMuteUpdate = 0.f;
	/** Gameplay mutes we placed (listener hears nothing from speaker). */
	struct FAIBVoiceMute
	{
		TWeakObjectPtr<APlayerController> Listener;
		FUniqueNetIdRepl Speaker;
		bool operator==(const FAIBVoiceMute& Other) const { return Listener == Other.Listener && Speaker == Other.Speaker; }
	};
	TArray<FAIBVoiceMute> VoiceMutes;

	// Phase bookkeeping
	bool bPhaseTimed = false;
	EAIBPhase PendingPhase = EAIBPhase::None;
	float PendingPhaseTime = 0.f;

	// Lobby options
	int32 AutoBots = 0;
	bool bAutoStart = false;
	/** Test option: ready everyone who is seated (AutoStart also fills empty seats with AI). */
	bool bAutoReady = false;
	float LobbyCountdownCancelCooldown = 0.f;

	// Stage 2 bookkeeping
	TArray<int32> Stage2Qualified;
	/** Names in the current Stage 2 tie-break (for the round banner). */
	FString Stage2TieNames;
	/** Consecutive tied Stage 1 sudden-death rounds (two players can both pick 0 forever). */
	int32 Stage1TiedRounds = 0;
	TArray<int32> Stage2Contenders;
	int32 Stage2SpotsLeft = 0;
	/** Chat claims this round: Receiver slot -> (claimer slot, claimed symbol). */
	TMultiMap<int32, TPair<int32, EAIBSymbol>> Stage2Claims;
	/** Bot trust in other players: (bot slot * 8 + other slot) -> trust 0..1. */
	TMap<int32, float> BotTrust;

	// Bot scheduling: slot -> server time to act
	TMap<int32, float> BotActTime;
	TMap<int32, int32> BotChatBudget;

	// Stage 3
	UPROPERTY()
	TArray<TObjectPtr<AAIBVehicle>> Vehicles;
	/** Road distance from each finalist's start to the Guardian (for the progress bars). */
	TArray<float> RaceStartDistance;
	float RaceStartTime = 0.f;
	float NextVehicleLog = 0.f;

	/** Frame-rate sampling for the log (real time). */
	double PerfWindowStart = 0.0;
	int32 PerfFrames = 0;

	// Final
	/** Latest the current choice may run to: the selection timer plus one confirmation window. */
	float FinalChoiceHardEnd = 0.f;
	float BotFinalActTime = 0.f;
	bool bBotSelected = false;

	FRandomStream Random;
};
