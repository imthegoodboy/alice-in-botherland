// Alice in Botherland - narrated picture cards: the opening story (full screen) and each trial's rules (compact card)

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Audio/AIBNarration.h"

class SVerticalBox;

/**
 * Shows a narration sequence in step with a clock (seconds into the sequence). Captions appear line by line as Alice
 * speaks them. Full screen: the story with crossfading, slowly zooming pictures and chapter titles. Compact: a rules
 * card (picture, trial title, welcome line, then each rule as it is explained).
 */
class ALICEINBOTHERLAND_API SAIBStoryCards : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBStoryCards)
		: _Fullscreen(true)
	{}
		SLATE_ATTRIBUTE(EAIBNarration, Sequence)
		SLATE_ATTRIBUTE(float, Elapsed)
		SLATE_ARGUMENT(bool, Fullscreen)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** True while the sequence has something to show at this time (the HUD hides the card afterwards). */
	static bool IsShowing(EAIBNarration Which, float Elapsed);

private:
	TSharedRef<SWidget> MakeFullscreen();
	TSharedRef<SWidget> MakeCompact();
	TSharedRef<SWidget> MakePicture(bool bPrevious);
	void RebuildLines();

	const AIBNarration::FSequence& Seq() const;
	/** Beat whose picture and card are on screen (the first one during the lead-in); INDEX_NONE if empty. */
	int32 ShownBeat() const;
	/** First beat of the card / of the run of beats sharing a picture that Beat belongs to. */
	int32 CardStart(int32 Beat) const;
	int32 ImageStart(int32 Beat) const;
	int32 CardIndex(int32 Beat) const;
	int32 CardCount() const;
	/** When a beat's picture or card came up (the very first one at 0). */
	float BeatTime(int32 Beat) const;
	FName ImageFor(bool bPrevious) const;
	float ImageFade() const;
	float CardFade() const;
	TOptional<FSlateRenderTransform> KenBurns(bool bPrevious) const;

	TAttribute<EAIBNarration> Sequence;
	TAttribute<float> Elapsed;
	bool bFullscreen = true;

	TSharedPtr<SVerticalBox> Lines;
	EAIBNarration BuiltSequence = EAIBNarration::None;
	int32 BuiltBeat = INDEX_NONE - 1;
	int32 CurrentBeat = INDEX_NONE;
};
