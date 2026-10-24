// Alice in Botherland - game instance: profile, settings, travel errors, loading screen

#include "Core/AIBGameInstance.h"
#include "Audio/AIBAudioSubsystem.h"
#include "UI/SAIBLoadingScreen.h"
#include "AliceInBotherland.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "MoviePlayer.h"
#include "Engine/Engine.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/PlatformProcess.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "AIBGameInstance"

const TCHAR* UAIBGameInstance::IslandMap = TEXT("/Game/AliceInBotherland/Maps/L_Island");

namespace
{
	const TCHAR* ProfileSlot = TEXT("AIB_Profile");

	/** Bump to re-run the automatic graphics detection on existing profiles. */
	constexpr int32 GraphicsDefaultsVersion = 2;
}

void UAIBGameInstance::Init()
{
	Super::Init();

	GetProfile();
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UAIBGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UAIBGameInstance::HandleTravelFailure);
	}
	if (!IsDedicatedServerInstance())
	{
		PreLoadHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UAIBGameInstance::BeginLoadingScreen);
		PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UAIBGameInstance::EndLoadingScreen);
	}
}

void UAIBGameInstance::OnStart()
{
	Super::OnStart();
	UE_LOG(LogAIB, Log, TEXT("Alice in Botherland: The Last Island v1.3.0 build 1"));

	// Settings are applied here rather than in Init: scalability changes are ignored until the engine is initialized.
	// First launch on this PC (or new defaults): let the engine pick sensible graphics settings for the hardware.
	if (!GIsEditor && (!Profile->bBenchmarked || Profile->GraphicsDefaultsVersion < GraphicsDefaultsVersion))
	{
		if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
		{
			UserSettings->RunHardwareBenchmark();
			UserSettings->ApplyHardwareBenchmarkResults();
			// The benchmark measures speed, not video memory: Epic overflows 4 GB GPUs (stutter and flicker), so
			// auto-detect tops out at High. Players with big GPUs can still pick Epic in Settings.
			const int32 Detected = UserSettings->GetOverallScalabilityLevel();
			Profile->Settings.Quality = FMath::Clamp(Detected < 0 ? 2 : Detected, 0, 2);
			UserSettings->SetOverallScalabilityLevel(Profile->Settings.Quality);
			UE_LOG(LogAIB, Log, TEXT("Graphics auto-detect: level %d (benchmark said %d)"), Profile->Settings.Quality, Detected);
		}
		Profile->bBenchmarked = true;
		Profile->GraphicsDefaultsVersion = GraphicsDefaultsVersion;
		SaveProfile();
	}
	ApplySettings();
}

void UAIBGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
	Super::Shutdown();
}

UAIBSaveGame* UAIBGameInstance::GetProfile()
{
	if (!Profile)
	{
		Profile = Cast<UAIBSaveGame>(UGameplayStatics::LoadGameFromSlot(ProfileSlot, 0));
		if (!Profile)
		{
			Profile = Cast<UAIBSaveGame>(UGameplayStatics::CreateSaveGameObject(UAIBSaveGame::StaticClass()));
		}
		if (Profile->Settings.PlayerName.IsEmpty())
		{
			FString Name = FPlatformProcess::UserName();
			Name = Name.Left(16);
			Profile->Settings.PlayerName = Name.IsEmpty() ? FString::Printf(TEXT("Nomad%03d"), FMath::RandRange(100, 999)) : Name;
		}
	}
	return Profile;
}

const FAIBSettings& UAIBGameInstance::GetSettings()
{
	return GetProfile()->Settings;
}

void UAIBGameInstance::SetSettings(const FAIBSettings& NewSettings)
{
	GetProfile()->Settings = NewSettings;
	ApplySettings();
	SaveProfile();
}

FString UAIBGameInstance::GetPlayerName()
{
	FString Name = GetSettings().PlayerName;
	// Characters that would break URL options.
	Name.ReplaceInline(TEXT("?"), TEXT(""));
	Name.ReplaceInline(TEXT("="), TEXT(""));
	Name.ReplaceInline(TEXT("#"), TEXT(""));
	Name.ReplaceInline(TEXT(" "), TEXT("_"));
	return Name.IsEmpty() ? TEXT("Nomad") : Name.Left(16);
}

void UAIBGameInstance::ApplySettings()
{
	const FAIBSettings& S = GetSettings();
	if (UAIBAudioSubsystem* Audio = GetSubsystem<UAIBAudioSubsystem>())
	{
		Audio->SetVolumes(S.MasterVolume, S.MusicVolume, S.SfxVolume, S.VoiceVolume);
		Audio->bSubtitles = S.bSubtitles;
	}
	if (GIsEditor)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->DisplayGamma = 2.2f * FMath::Clamp(S.Brightness, 0.7f, 1.3f);
		const float Caps[] = { 30.f, 60.f, 120.f, 0.f };
		GEngine->SetMaxFPS(Caps[FMath::Clamp(S.FrameRateIndex, 0, 3)]);
	}
	if (IConsoleVariable* MotionBlur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		MotionBlur->Set(S.bMotionBlur ? 4 : 0, ECVF_SetByGameSetting);
	}
	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		if (S.Quality >= 0)
		{
			UserSettings->SetOverallScalabilityLevel(FMath::Clamp(S.Quality, 0, 3));
		}
		UserSettings->SetFullscreenMode(S.bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		UserSettings->SetVSyncEnabled(S.bVSync);
		UserSettings->SetResolutionScaleNormalized(FMath::Clamp(S.ResolutionScale / 100.f, 0.5f, 1.f));
		UserSettings->ApplySettings(false);
	}
	// Nanite geometry pool: engine default (512 MB) at Epic, 320 MB below it (the island streams fine in that).
	if (IConsoleVariable* NanitePool = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.Streaming.StreamingPoolSize")))
	{
		NanitePool->Set(S.Quality >= 3 ? 512 : 320, ECVF_SetByGameSetting);
	}
}

void UAIBGameInstance::SaveProfile()
{
	if (Profile)
	{
		UGameplayStatics::SaveGameToSlot(Profile, ProfileSlot, 0);
	}
}

void UAIBGameInstance::RecordMatch(const FAIBMatchRecord& Record)
{
	UAIBSaveGame* P = GetProfile();
	P->Matches.Insert(Record, 0);
	if (P->Matches.Num() > 50)
	{
		P->Matches.SetNum(50);
	}
	P->MatchesPlayed++;
	for (const FAIBMatchPlayerRecord& Player : Record.Players)
	{
		if (!Player.bLocal)
		{
			continue;
		}
		if (Player.Status == EAIBPlayerStatus::Champion)
		{
			P->Crowns++;
		}
		else if (Player.Status == EAIBPlayerStatus::Sacrificed)
		{
			P->Sacrifices++;
		}
		if (Player.EliminatedInStage == 0 && Player.Status != EAIBPlayerStatus::Forfeited)
		{
			P->Finals++;
		}
	}
	SaveProfile();
}

void UAIBGameInstance::OpenMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(IslandMap), true, TEXT("game=Menu"));
}

void UAIBGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogAIB, Warning, TEXT("Network failure: %s"), *ErrorString);
	PendingMenuMessage = FText::Format(LOCTEXT("NetFail", "Connection lost: {0}"), FText::FromString(ErrorString));
}

void UAIBGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogAIB, Warning, TEXT("Travel failure: %s"), *ErrorString);
	PendingMenuMessage = FText::Format(LOCTEXT("TravelFail", "Could not join: {0}"), FText::FromString(ErrorString));
}

void UAIBGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer() || !IsMoviePlayerEnabled())
	{
		return;
	}
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.bWaitForManualStop = false;
	Attributes.MinimumLoadingScreenDisplayTime = 1.5f;
	Attributes.WidgetLoadingScreen = SNew(SAIBLoadingScreen);
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UAIBGameInstance::EndLoadingScreen(UWorld* LoadedWorld)
{
}

#undef LOCTEXT_NAMESPACE
