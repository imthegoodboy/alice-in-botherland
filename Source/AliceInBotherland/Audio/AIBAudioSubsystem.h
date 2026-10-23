// Alice in Botherland - music, voice, ambience and one-shot sounds by cue id

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Core/AIBTypes.h"
#include "AIBAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Cue ids map to assets: VO_* -> Audio/VO, MUS_* -> Audio/Music (looping, crossfaded), SFX_* -> Audio/SFX.
 * Voice lines show subtitles and duck the music.
 */
UCLASS()
class ALICEINBOTHERLAND_API UAIBAudioSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAIBAudioSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }

	void PlayCue(FName CueId);
	void PlayMusic(FName TrackId);
	void StopMusic(float FadeOut = 2.f);
	void PlayVoice(FName LineId);
	void PlaySfx(FName SfxId, float Volume = 1.f, float Pitch = 1.f);
	void SetAmbienceForZone(EAIBSceneZone Zone);

	void SetVolumes(float Master, float Music, float Sfx, float Voice);
	bool bSubtitles = true;

	/** Current subtitle (empty when none) for the HUD. */
	FText GetSubtitle() const;
	FText GetSubtitleSpeaker() const;

	static FText VoiceLineText(FName LineId);

private:
	USoundBase* LoadSound(const FString& Folder, FName Id, bool bLoop);
	UWorld* GetAudioWorld() const;
	UAudioComponent* MakeComponent(USoundBase* Sound, float Volume);
	void ApplyVolumes();
	void CheckWorld();

	UPROPERTY() TObjectPtr<UAudioComponent> MusicA;
	UPROPERTY() TObjectPtr<UAudioComponent> MusicB;
	UPROPERTY() TObjectPtr<UAudioComponent> Voice;
	UPROPERTY() TObjectPtr<UAudioComponent> Ambience;

	FName CurrentTrack;
	FName CurrentAmbience;
	bool bMusicAActive = true;
	float MasterVol = 0.9f;
	float MusicVol = 0.6f;
	float SfxVol = 0.85f;
	float VoiceVol = 1.f;
	float Duck = 1.f;

	FName SubtitleLine;
	float SubtitleEnd = 0.f;
	TWeakObjectPtr<UWorld> ComponentWorld;
};
