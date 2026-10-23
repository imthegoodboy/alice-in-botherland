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
	/** Caption text size multiplier (0.8 - 1.5). */
	UPROPERTY() float SubtitleScale = 1.f;
	/** Third-person camera field of view in degrees. */
	UPROPERTY() float FieldOfView = 80.f;
	/** Voice chat: 0 = microphone off, 1 = push to talk (V), 2 = open microphone. */
	UPROPERTY() int32 MicMode = 1;
	/** 0 Low .. 3 Epic, -1 = auto (benchmark). */
	UPROPERTY() int32 Quality = -1;
	UPROPERTY() bool bFullscreen = true;
	UPROPERTY() bool bVSync = true;
	UPROPERTY() float ResolutionScale = 100.f;
	/** Picture brightness (display gamma multiplier, 0.7 - 1.3). */
	UPROPERTY() float Brightness = 1.f;
	/** Frame rate cap: 0 = 30, 1 = 60, 2 = 120, 3 = unlimited. */
	UPROPERTY() int32 FrameRateIndex = 3;
	UPROPERTY() bool bMotionBlur = true;
	UPROPERTY() bool bShowFPS = false;
};

UCLASS()
class ALICEINBOTHERLAND_API UAIBSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() FAIBSettings Settings;
	UPROPERTY() bool bSeenPrologue = false;
	/** First-launch profile (champion name + date of birth) has been filled in. */
	UPROPERTY() bool bProfileComplete = false;
	UPROPERTY() int32 BirthYear = 0;
	UPROPERTY() int32 BirthMonth = 0;
	UPROPERTY() int32 BirthDay = 0;
	/** Age in whole years from the stored date of birth (-1 if unknown). */
	int32 GetAge() const
	{
		if (BirthYear <= 0 || BirthMonth <= 0 || BirthDay <= 0)
		{
			return -1;
		}
		const FDateTime Now = FDateTime::Now();
		int32 Age = Now.GetYear() - BirthYear;
		if (Now.GetMonth() < BirthMonth || (Now.GetMonth() == BirthMonth && Now.GetDay() < BirthDay))
		{
			--Age;
		}
		return Age;
	}
	UPROPERTY() bool bBenchmarked = false;
	/** Version of the automatic graphics defaults this profile was set up with (re-detected when it changes). */
	UPROPERTY() int32 GraphicsDefaultsVersion = 0;
	UPROPERTY() TArray<FAIBMatchRecord> Matches;
	UPROPERTY() int32 MatchesPlayed = 0;
	UPROPERTY() int32 Crowns = 0;
	UPROPERTY() int32 Sacrifices = 0;
	UPROPERTY() int32 Finals = 0;
};
