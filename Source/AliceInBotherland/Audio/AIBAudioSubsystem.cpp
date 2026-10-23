// Alice in Botherland - music, voice, ambience and one-shot sounds by cue id

#include "Audio/AIBAudioSubsystem.h"
#include "Audio/AIBNarration.h"
#include "Core/AIBAssets.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "AudioDevice.h"

#define LOCTEXT_NAMESPACE "AIBAudio"

void UAIBAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAIBAudioSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UWorld* UAIBAudioSubsystem::GetAudioWorld() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetWorld() : nullptr;
}

FText UAIBAudioSubsystem::VoiceLineText(FName LineId)
{
	static TMap<FName, FText> Lines;
	if (Lines.Num() == 0)
	{
		Lines.Add(TEXT("VO_Prologue_01"), LOCTEXT("P1", "They called it the Long Burn. The year the oceans turned, the sea began to eat everything it touched."));
		Lines.Add(TEXT("VO_Prologue_02"), LOCTEXT("P2", "Reefs dissolved. Harbors rusted overnight. The water that once fed the world became a slow green acid, and the coasts fell silent."));
		Lines.Add(TEXT("VO_Prologue_03"), LOCTEXT("P3", "Those who survived took to the waves. Floating towns of scrap and sail, drifting from one dying shore to the next. We called ourselves the Nomads."));
		Lines.Add(TEXT("VO_Prologue_04"), LOCTEXT("P4", "For thirty years we searched. Then, beyond the Bleached Reach, a beacon answered. An island the acid had never touched. Fresh water. Soil. Shelter. A future."));
		Lines.Add(TEXT("VO_Prologue_05"), LOCTEXT("P5", "But the island has a keeper. The Guardian. And it speaks only one law: the island will belong to one community. Only one."));
		Lines.Add(TEXT("VO_Prologue_06"), LOCTEXT("P6", "Five fleets answered the beacon. Each sent a single champion. Three trials. Five champions. One island. And at the end of it all, a choice."));
		Lines.Add(TEXT("VO_Arrival"), LOCTEXT("Arr", "Five communities have found their last hope. Only one community can claim this island. Your survival depends on your choices."));
		Lines.Add(TEXT("VO_Explore"), LOCTEXT("Exp", "Champions. Walk my island. See what you would fight for. The first trial begins soon."));
		Lines.Add(TEXT("VO_Stage1_Brief"), LOCTEXT("S1B", "The first trial. The Number Game. Choose a number from zero to one hundred. Your target is eighty percent of the average. Closest survives. Farthest is lost."));
		Lines.Add(TEXT("VO_Stage1_Round"), LOCTEXT("S1R", "Choose."));
		Lines.Add(TEXT("VO_Stage1_SuddenDeath"), LOCTEXT("S1S", "A tie at the edge. Sudden death."));
		Lines.Add(TEXT("VO_Stage1_Elim"), LOCTEXT("S1E", "A champion has fallen. Four remain."));
		Lines.Add(TEXT("VO_Stage2_Brief"), LOCTEXT("S2B", "The second trial. The Hidden Symbol. You carry a mark you cannot see. Your rivals can. Speak. Trust. Deceive. Then name your own mark. Only two will walk on."));
		Lines.Add(TEXT("VO_Stage2_Guess"), LOCTEXT("S2G", "Silence. Name your symbol."));
		Lines.Add(TEXT("VO_Stage2_Elim"), LOCTEXT("S2E", "Two champions remain. The race to the Guardian begins."));
		Lines.Add(TEXT("VO_Stage3_Brief"), LOCTEXT("S3B", "The final trial. Two machines. One road. Reach me first, and you will choose first."));
		Lines.Add(TEXT("VO_Countdown"), LOCTEXT("S3C", "Three. Two. One. Go!"));
		Lines.Add(TEXT("VO_Stage3_SuddenDeath"), LOCTEXT("S3S", "Neither of you reached me. Again. The road is shorter now."));
		Lines.Add(TEXT("VO_Stage3_Arrived"), LOCTEXT("S3A", "The switch is claimed. The first choice is earned."));
		Lines.Add(TEXT("VO_Final_Brief"), LOCTEXT("FB", "Hear my final law. Become the King, and the island is yours alone, but your people will not follow. Or save your people. Give your life, and your community is carried to safety. Choose."));
		Lines.Add(TEXT("VO_Final_Second"), LOCTEXT("FS", "A life is given. Now the choice passes to you."));
		Lines.Add(TEXT("VO_End_King"), LOCTEXT("EK", "A ruler is crowned. The island is claimed, but the fleet of their people turns back toward the burning sea. A victory, and a loss, that history will remember."));
		Lines.Add(TEXT("VO_End_SacrificeKing"), LOCTEXT("ESK", "One champion gave everything, and their people were carried to safety. The other took the crown. Tonight, one community sleeps on solid ground."));
		Lines.Add(TEXT("VO_End_Double"), LOCTEXT("ED", "Two champions refused the crown. Both communities were saved. The island remains unclaimed, waiting for a world that has learned to share it."));
		Lines.Add(TEXT("VO_End_SoloSacrifice"), LOCTEXT("ES", "The last champion gave their life for their people. The island remains unclaimed, and the Guardian sleeps once more."));
		Lines.Add(TEXT("VO_Lobby_Ready"), LOCTEXT("LR", "Five champions. The trials begin."));
	}
	if (const AIBNarration::FLine* Line = AIBNarration::FindLine(LineId))
	{
		return Line->Text;
	}
	const FText* Found = Lines.Find(LineId);
	return Found ? *Found : FText::GetEmpty();
}

FText UAIBAudioSubsystem::GetSubtitle() const
{
	const UWorld* World = GetAudioWorld();
	if (!bSubtitles || SubtitleLine.IsNone() || !World || World->GetRealTimeSeconds() > SubtitleEnd)
	{
		return FText::GetEmpty();
	}
	return VoiceLineText(SubtitleLine);
}

FText UAIBAudioSubsystem::GetSubtitleSpeaker() const
{
	const FString Id = SubtitleLine.ToString();
	if (AIBNarration::IsNarratorLine(SubtitleLine))
	{
		return LOCTEXT("Alice", "ALICE");
	}
	const bool bNarrator = Id.StartsWith(TEXT("VO_Prologue")) || Id.StartsWith(TEXT("VO_End")) || Id == TEXT("VO_Arrival");
	return bNarrator ? LOCTEXT("Narrator", "NARRATOR") : LOCTEXT("Guardian", "THE GUARDIAN");
}

USoundBase* UAIBAudioSubsystem::LoadSound(const FString& Folder, FName Id, bool bLoop)
{
	USoundBase* Sound = AIBAssets::Sound(AIBAssets::Paths::Audio + Folder + TEXT("/") + Id.ToString());
	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		if (bLoop)
		{
			Wave->bLooping = true;
		}
	}
	return Sound;
}

UAudioComponent* UAIBAudioSubsystem::MakeComponent(USoundBase* Sound, float Volume)
{
	UWorld* World = GetAudioWorld();
	if (!World || !Sound)
	{
		return nullptr;
	}
	UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, Sound, Volume, 1.f, 0.f, nullptr, false, false);
	if (Comp)
	{
		Comp->bIsUISound = true;
		Comp->bAutoDestroy = false;
	}
	return Comp;
}

void UAIBAudioSubsystem::PlayCue(FName CueId)
{
	const FString Id = CueId.ToString();
	if (Id.StartsWith(TEXT("VO_")) || Id.StartsWith(TEXT("NAR_")))
	{
		PlayVoice(CueId);
	}
	else if (Id.StartsWith(TEXT("MUS_")))
	{
		PlayMusic(CueId);
	}
	else
	{
		PlaySfx(CueId);
	}
}

void UAIBAudioSubsystem::CheckWorld()
{
	UWorld* World = GetAudioWorld();
	if (ComponentWorld.Get() != World)
	{
		// New map: the old components died with the old world.
		MusicA = MusicB = Voice = Ambience = nullptr;
		CurrentTrack = NAME_None;
		CurrentAmbience = NAME_None;
		ComponentWorld = World;
	}
}

void UAIBAudioSubsystem::PlayMusic(FName TrackId)
{
	CheckWorld();
	if (TrackId == CurrentTrack && ((bMusicAActive ? MusicA : MusicB) != nullptr))
	{
		return;
	}
	USoundBase* Sound = LoadSound(TEXT("Music"), TrackId, true);
	if (!Sound)
	{
		return;
	}
	UAudioComponent* Old = bMusicAActive ? MusicA : MusicB;
	if (Old)
	{
		Old->FadeOut(2.5f, 0.f);
	}
	bMusicAActive = !bMusicAActive;
	TObjectPtr<UAudioComponent>& New = bMusicAActive ? MusicA : MusicB;
	New = MakeComponent(Sound, MasterVol * MusicVol * Duck);
	if (New)
	{
		New->FadeIn(2.f, MasterVol * MusicVol * Duck);
	}
	CurrentTrack = TrackId;
}

void UAIBAudioSubsystem::StopMusic(float FadeOut)
{
	for (UAudioComponent* Comp : { MusicA.Get(), MusicB.Get() })
	{
		if (Comp)
		{
			Comp->FadeOut(FadeOut, 0.f);
		}
	}
	CurrentTrack = NAME_None;
}

void UAIBAudioSubsystem::PlayVoice(FName LineId)
{
	CheckWorld();
	UWorld* World = GetAudioWorld();
	if (!World)
	{
		return;
	}
	USoundBase* Sound = LoadSound(TEXT("VO"), LineId, false);
	if (Voice)
	{
		Voice->Stop();
	}
	const float Duration = Sound ? Sound->GetDuration() : 4.f;
	SubtitleLine = LineId;
	SubtitleEnd = World->GetRealTimeSeconds() + FMath::Max(2.5f, Duration + 0.6f);
	if (Sound)
	{
		Voice = MakeComponent(Sound, MasterVol * VoiceVol);
		if (Voice)
		{
			Voice->Play();
		}
	}
}

void UAIBAudioSubsystem::StopNarration()
{
	if (AIBNarration::IsNarratorLine(SubtitleLine))
	{
		if (IsValid(Voice))
		{
			Voice->FadeOut(0.4f, 0.f);
		}
		SubtitleLine = NAME_None;
	}
}

FName UAIBAudioSubsystem::GetSubtitleLine() const
{
	const UWorld* World = GetAudioWorld();
	return World && World->GetRealTimeSeconds() <= SubtitleEnd ? SubtitleLine : NAME_None;
}

void UAIBAudioSubsystem::PlaySfx(FName SfxId, float Volume, float Pitch)
{
	UWorld* World = GetAudioWorld();
	USoundBase* Sound = LoadSound(TEXT("SFX"), SfxId, false);
	if (World && Sound)
	{
		UGameplayStatics::PlaySound2D(World, Sound, MasterVol * SfxVol * Volume, Pitch, 0.f, nullptr, nullptr, true);
	}
}

void UAIBAudioSubsystem::SetAmbienceForZone(EAIBSceneZone Zone)
{
	FName Id;
	switch (Zone)
	{
	case EAIBSceneZone::Lobby:
	case EAIBSceneZone::Sea:
	case EAIBSceneZone::Results:
		Id = TEXT("SFX_Amb_Ocean");
		break;
	case EAIBSceneZone::NumberRoom:
		Id = TEXT("SFX_Drone_Dark_Loop");
		break;
	case EAIBSceneZone::Hotel:
		Id = TEXT("SFX_Amb_Hotel");
		break;
	default:
		Id = TEXT("SFX_Amb_Wind");
		break;
	}
	CheckWorld();
	if (Id == CurrentAmbience && Ambience)
	{
		return;
	}
	if (Ambience)
	{
		Ambience->FadeOut(1.5f, 0.f);
	}
	USoundBase* Sound = LoadSound(TEXT("SFX"), Id, true);
	Ambience = Sound ? MakeComponent(Sound, MasterVol * SfxVol * 0.55f) : nullptr;
	if (Ambience)
	{
		Ambience->FadeIn(2.f, MasterVol * SfxVol * 0.55f);
	}
	CurrentAmbience = Id;
}

void UAIBAudioSubsystem::SetVolumes(float Master, float Music, float Sfx, float InVoice)
{
	MasterVol = Master;
	MusicVol = Music;
	SfxVol = Sfx;
	VoiceVol = InVoice;
	ApplyVolumes();
}

void UAIBAudioSubsystem::ApplyVolumes()
{
	UAudioComponent* ActiveMusic = bMusicAActive ? MusicA : MusicB;
	if (ActiveMusic)
	{
		ActiveMusic->SetVolumeMultiplier(MasterVol * MusicVol * Duck);
	}
	if (Ambience)
	{
		Ambience->SetVolumeMultiplier(MasterVol * SfxVol * 0.55f);
	}
	if (Voice)
	{
		Voice->SetVolumeMultiplier(MasterVol * VoiceVol);
	}
}

void UAIBAudioSubsystem::Tick(float DeltaTime)
{
	// Duck the music under voice lines.
	CheckWorld();
	const bool bVoicePlaying = IsValid(Voice) && Voice->IsPlaying();
	// Music sits well under the voice, then swells back slowly once the line ends.
	const float Target = bVoicePlaying ? 0.25f : 1.f;
	const float NewDuck = FMath::FInterpTo(Duck, Target, DeltaTime, bVoicePlaying ? 4.f : 1.2f);
	if (!FMath::IsNearlyEqual(NewDuck, Duck, 0.002f))
	{
		Duck = NewDuck;
		ApplyVolumes();
	}
}

#undef LOCTEXT_NAMESPACE
