// Alice in Botherland - the narrator (Alice): her lines and the narrated sequences built from them

#pragma once

#include "CoreMinimal.h"
#include "Core/AIBTypes.h"

/** A narrated sequence: the story intro, the island welcome, and each trial's welcome + rules. */
enum class EAIBNarration : uint8
{
	None,
	Story,
	Island,
	Stage1,
	Stage2,
	Stage3,
	Final,
	/** The truth about the island, after the ending; the closing line depends on how it ended. */
	EpilogueCrown,
	EpilogueGift,
	EpilogueSaved
};

/**
 * Sequences are timed off the replicated phase clock, so every player hears and reads the same line at the same
 * moment (and a reconnecting player picks up mid-sequence). Line texts and spoken lengths come from the generated
 * AIBNarrationLines.inl (Tools/gen_narration_table.py), so captions always match the recordings.
 */
namespace AIBNarration
{
	struct FLine
	{
		FName Id;
		float Duration = 0.f;
		FText Text;
	};

	/** One step of a sequence: the line, the picture shown with it, and the title of the card it belongs to. */
	struct FBeat
	{
		FName Line;
		FName Image;
		FText Card;
	};

	struct FSequence
	{
		TArray<FBeat> Beats;
		/** Offset of each beat from the start of the sequence (lead-in and gaps included). */
		TArray<float> Starts;
		float Length = 0.f;
	};

	/** Silence before the first line, between lines, and after the last one. */
	constexpr float LeadIn = 0.8f;
	constexpr float Gap = 0.45f;
	constexpr float Tail = 1.6f;

	const FLine* FindLine(FName Id);
	bool IsNarratorLine(FName Id);
	const FSequence& Get(EAIBNarration Which);
	/** Total length of a sequence in seconds (0 for None). */
	float Length(EAIBNarration Which);
	/** Beat being spoken (or just spoken) at Elapsed seconds into the sequence; INDEX_NONE before the first. */
	int32 BeatAt(EAIBNarration Which, float Elapsed);
	/** True if the line is one of the sequence's beats. */
	bool Contains(EAIBNarration Which, FName Line);
	/** The sequence narrated during a phase (None when the phase has no narration); the epilogue depends on the ending. */
	EAIBNarration ForPhase(EAIBPhase Phase, EAIBEnding Ending = EAIBEnding::None);
	/** The epilogue for an ending (None when nobody reached the Guardian's choice). */
	EAIBNarration EpilogueFor(EAIBEnding Ending);
	/** Texture asset path for an image id (e.g. "S3_Island"). */
	FString ImagePath(FName Image);
}
