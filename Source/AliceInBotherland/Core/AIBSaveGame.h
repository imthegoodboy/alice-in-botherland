// Alice in Botherland - local profile: settings + Hall of Champions

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/AIBTypes.h"
#include "AIBSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FAIBSettings
{
	GENERATED_BODY()

	UPROPERTY() FString PlayerName;
	UPROPERTY() float MasterVolume = 0.9f;
	UPROPERTY() float MusicVolume = 0.6f;
	UPROPERTY() float SfxVolume = 0.85f;
	UPROPERTY() float VoiceVolume = 1.0f;
	UPROPERTY() float MouseSensitivity = 1.0f;
	UPROPERTY() bool bInvertY = false;
	UPROPERTY() bool bSubtitles = true;
	/** 0 Low .. 3 Epic, -1 = auto (benchmark). */
	UPROPERTY() int32 Quality = -1;
	UPROPERTY() bool bFullscreen = true;
	UPROPERTY() bool bVSync = true;
	UPROPERTY() float ResolutionScale = 100.f;
};

UCLASS()
class ALICEINBOTHERLAND_API UAIBSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() FAIBSettings Settings;
	UPROPERTY() bool bSeenPrologue = false;
	UPROPERTY() bool bBenchmarked = false;
	/** Version of the automatic graphics defaults this profile was set up with (re-detected when it changes). */
	UPROPERTY() int32 GraphicsDefaultsVersion = 0;
	UPROPERTY() TArray<FAIBMatchRecord> Matches;
	UPROPERTY() int32 MatchesPlayed = 0;
	UPROPERTY() int32 Crowns = 0;
	UPROPERTY() int32 Sacrifices = 0;
	UPROPERTY() int32 Finals = 0;
};
