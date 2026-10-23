// Alice in Botherland - shared gameplay types

#pragma once

#include "CoreMinimal.h"
#include "AIBTypes.generated.h"

/** Every server-driven moment of a match. The lobby is part of the match so no map travel is needed. */
UENUM(BlueprintType)
enum class EAIBPhase : uint8
{
	None,
	Lobby,
	LobbyCountdown,
	StoryIntro,
	ArrivalCinematic,
	IslandExplore,
	Stage1Briefing,
	Stage1RoundIntro,
	Stage1Selection,
	Stage1Reveal,
	Stage1Results,
	Stage1Elimination,
	Stage2Briefing,
	Stage2Communication,
	Stage2Guess,
	Stage2Results,
	Stage2Elimination,
	Stage3Briefing,
	Stage3Countdown,
	Stage3Race,
	Stage3Finish,
	FinalBriefing,
	FinalChoice,
	FinalResolve,
	EndingCinematic,
	Results
};

UENUM(BlueprintType)
enum class EAIBPlayerStatus : uint8
{
	Active,
	Eliminated,
	Sacrificed,
	Champion,
	Forfeited
};

UENUM(BlueprintType)
enum class EAIBSymbol : uint8
{
	None,
	Heart,
	Diamond,
	Club,
	Spade
};

UENUM(BlueprintType)
enum class EAIBFinalChoice : uint8
{
	None,
	King,
	Sacrifice
};

UENUM(BlueprintType)
enum class EAIBEnding : uint8
{
	None,
	/** First chooser became the King. */
	KingFirst,
	/** First chooser sacrificed, second became the King. */
	SacrificeThenKing,
	/** Both finalists sacrificed. */
	DoubleSacrifice,
	/** Only one finalist left (forfeit) and they sacrificed. */
	SoloSacrifice,
	/** Everyone else forfeited before the final; the survivor still chooses, this is used when they claim the island. */
	SoloKing
};

/** Coarse location the camera / time of day should use for a phase. */
UENUM()
enum class EAIBSceneZone : uint8
{
	Lobby,
	Sea,
	Island,
	NumberRoom,
	Hotel,
	Race,
	Guardian,
	Results
};

/** One player's number in a Number Game round, revealed after the round resolves. */
USTRUCT(BlueprintType)
struct FAIBNumberEntry
{
	GENERATED_BODY()

	UPROPERTY() int32 Slot = INDEX_NONE;
	UPROPERTY() int32 Number = 0;
	UPROPERTY() float Distance = 0.f;
	UPROPERTY() bool bAuto = false;
	UPROPERTY() bool bWinner = false;
	UPROPERTY() bool bFarthest = false;
};

USTRUCT(BlueprintType)
struct FAIBStage1Round
{
	GENERATED_BODY()

	UPROPERTY() int32 Round = 0;
	UPROPERTY() bool bSuddenDeath = false;
	UPROPERTY() float Average = 0.f;
	UPROPERTY() float Target = 0.f;
	UPROPERTY() TArray<FAIBNumberEntry> Entries;
	/** Slot eliminated by this round, or INDEX_NONE when a tie forces sudden death. */
	UPROPERTY() int32 EliminatedSlot = INDEX_NONE;
	UPROPERTY() TArray<int32> TiedSlots;
};

USTRUCT(BlueprintType)
struct FAIBSymbolResult
{
	GENERATED_BODY()

	UPROPERTY() int32 Slot = INDEX_NONE;
	UPROPERTY() EAIBSymbol Actual = EAIBSymbol::None;
	UPROPERTY() EAIBSymbol Guess = EAIBSymbol::None;
	UPROPERTY() bool bCorrect = false;
};

/** Private view sent to one client: the symbol another player carries. Never contains the receiver's own symbol. */
USTRUCT(BlueprintType)
struct FAIBSymbolView
{
	GENERATED_BODY()

	UPROPERTY() int32 Slot = INDEX_NONE;
	UPROPERTY() EAIBSymbol Symbol = EAIBSymbol::None;
};

USTRUCT(BlueprintType)
struct FAIBChatMessage
{
	GENERATED_BODY()

	UPROPERTY() int32 FromSlot = INDEX_NONE;
	/** Optional addressee for "your symbol is ..." claims. */
	UPROPERTY() int32 ToSlot = INDEX_NONE;
	UPROPERTY() FString FromName;
	UPROPERTY() FString Text;
	UPROPERTY() EAIBSymbol ClaimSymbol = EAIBSymbol::None;
	UPROPERTY() bool bSystem = false;
	UPROPERTY() bool bSpectator = false;
	UPROPERTY() float ServerTime = 0.f;
};

USTRUCT(BlueprintType)
struct FAIBFinalRecord
{
	GENERATED_BODY()

	UPROPERTY() int32 Slot = INDEX_NONE;
	UPROPERTY() EAIBFinalChoice Choice = EAIBFinalChoice::None;
	UPROPERTY() bool bAuto = false;
};

/** Saved per match in the local Hall of Champions. */
USTRUCT(BlueprintType)
struct FAIBMatchPlayerRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Name;
	UPROPERTY() int32 Community = 0;
	UPROPERTY() EAIBPlayerStatus Status = EAIBPlayerStatus::Active;
	UPROPERTY() int32 EliminatedInStage = 0;
	UPROPERTY() bool bBot = false;
	UPROPERTY() bool bLocal = false;
};

USTRUCT(BlueprintType)
struct FAIBMatchRecord
{
	GENERATED_BODY()

	UPROPERTY() FDateTime Date;
	UPROPERTY() EAIBEnding Ending = EAIBEnding::None;
	UPROPERTY() FString ChampionName;
	UPROPERTY() int32 ChampionCommunity = INDEX_NONE;
	UPROPERTY() TArray<FAIBMatchPlayerRecord> Players;
};

/** Static description of one of the five communities. */
struct FAIBCommunityInfo
{
	FText Name;
	FText Motto;
	FText Description;
	FText ShipName;
	FLinearColor Color;
};

namespace AIB
{
	constexpr int32 MaxPlayers = 5;
	constexpr int32 Stage1Advance = 4;
	constexpr int32 Stage2Advance = 2;
	constexpr int32 Stage2Rounds = 3;

	// Timers from the design document (seconds).
	/** After the host presses START (everyone seated and ready). */
	constexpr float LobbyCountdown = 5.f;
	constexpr float LobbyReconnect = 30.f;
	constexpr float StageReconnect = 20.f;
	/** The fleet's full voyage takes ArrivalVoyage seconds; the arrival shows only its last part (the landing in the cove). */
	constexpr float ArrivalVoyage = 20.f;
	constexpr float ArrivalSkipTo = 12.f;
	constexpr float ArrivalCinematic = ArrivalVoyage - ArrivalSkipTo;
	constexpr float IslandExplore = 120.f;
	constexpr float Stage1Briefing = 10.f;
	constexpr float Stage1RoundIntro = 5.f;
	constexpr float Stage1Selection = 10.f;
	constexpr float Stage1Reveal = 3.f;
	constexpr float Stage1Results = 5.f;
	constexpr float StageElimination = 6.f;
	constexpr float Stage2Briefing = 8.f;
	constexpr float Stage2Communication = 30.f;
	constexpr float Stage2Guess = 10.f;
	constexpr float Stage2Results = 5.f;
	constexpr float Stage3Briefing = 10.f;
	constexpr float Stage3Countdown = 3.f;
	constexpr float Stage3Race = 90.f;
	constexpr float Stage3SuddenDeathRace = 45.f;
	constexpr float SwitchHold = 3.f;
	constexpr float Stage3Finish = 5.f;
	constexpr float FinalBriefing = 10.f;
	constexpr float FinalSelect = 15.f;
	constexpr float FinalConfirm = 5.f;
	constexpr float FinalResolve = 5.f;
	constexpr float EndingCinematic = 18.f;
	constexpr float FadeDuration = 1.0f;

	/** All authored light intensities are multiplied by this (candela calibration against the ~10 lux sun). */
	constexpr float LightScale = 0.08f;
	constexpr float RectLightScale = 0.35f;

	ALICEINBOTHERLAND_API const FAIBCommunityInfo& Community(int32 Slot);
	ALICEINBOTHERLAND_API FText SymbolName(EAIBSymbol Symbol);
	ALICEINBOTHERLAND_API FString SymbolGlyph(EAIBSymbol Symbol);
	ALICEINBOTHERLAND_API FLinearColor SymbolColor(EAIBSymbol Symbol);
	ALICEINBOTHERLAND_API EAIBSymbol ParseSymbol(const FString& Text);
	ALICEINBOTHERLAND_API FText PhaseTitle(EAIBPhase Phase);
	ALICEINBOTHERLAND_API EAIBSceneZone ZoneForPhase(EAIBPhase Phase);
	ALICEINBOTHERLAND_API bool IsInputPhase(EAIBPhase Phase);
	ALICEINBOTHERLAND_API FText EndingTitle(EAIBEnding Ending);
	ALICEINBOTHERLAND_API FString RandomBotName(FRandomStream& Stream, const TArray<FString>& Taken);

	/** Content paths used across the game. Everything game-owned lives under /Game/AliceInBotherland. */
	namespace Paths
	{
		inline const TCHAR* Root = TEXT("/Game/AliceInBotherland");
	}
}
