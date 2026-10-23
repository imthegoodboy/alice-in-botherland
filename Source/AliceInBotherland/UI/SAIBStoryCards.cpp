// Alice in Botherland - narrated picture cards: the opening story (full screen) and each trial's rules (compact card)

#include "UI/SAIBStoryCards.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "AIBStoryCards"

namespace
{
	constexpr float ImageFadeTime = 1.1f;
	constexpr float LineFadeTime = 0.45f;
	/** Seconds a picture takes to finish its slow push-in. */
	constexpr float ZoomTime = 16.f;

	FText Roman(int32 Number)
	{
		static const TCHAR* Numerals[] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI") };
		return FText::FromString(Numerals[FMath::Clamp(Number - 1, 0, 5)]);
	}

	/** "NAR_S1_Rule3" -> 3; 0 for lines that are not numbered rules. */
	int32 RuleNumber(FName Line)
	{
		const FString Id = Line.ToString();
		const int32 At = Id.Find(TEXT("_Rule"));
		return At == INDEX_NONE ? 0 : FCString::Atoi(*Id.Mid(At + 5));
	}

	const FSlateBrush* NoBrush()
	{
		static const FSlateNoResource Brush;
		return &Brush;
	}
}

bool SAIBStoryCards::IsShowing(EAIBNarration Which, float Elapsed)
{
	return Which != EAIBNarration::None && Elapsed < AIBNarration::Length(Which);
}

void SAIBStoryCards::Construct(const FArguments& InArgs)
{
	Sequence = InArgs._Sequence;
	Elapsed = InArgs._Elapsed;
	bFullscreen = InArgs._Fullscreen;
	SAssignNew(Lines, SVerticalBox);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(NoBrush())
		.Padding(0.f)
		.ColorAndOpacity_Lambda([this]()
		{
			if (bFullscreen)
			{
				return FLinearColor::White;
			}
			// The rules card slides in with the first words and leaves once Alice has finished.
			const float E = Elapsed.Get();
			const float Alpha = FMath::Min(FMath::Clamp(E / 0.5f, 0.f, 1.f), FMath::Clamp((AIBNarration::Length(Sequence.Get()) - E) / 0.8f, 0.f, 1.f));
			return FLinearColor(1.f, 1.f, 1.f, Alpha);
		})
		[
			bFullscreen ? MakeFullscreen() : MakeCompact()
		]
	];
}

void SAIBStoryCards::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const EAIBNarration Which = Sequence.Get();
	CurrentBeat = AIBNarration::BeatAt(Which, Elapsed.Get());
	if (Which != BuiltSequence || CurrentBeat != BuiltBeat)
	{
		BuiltSequence = Which;
		BuiltBeat = CurrentBeat;
		RebuildLines();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Timing helpers
// ---------------------------------------------------------------------------------------------------------------------

const AIBNarration::FSequence& SAIBStoryCards::Seq() const
{
	return AIBNarration::Get(Sequence.Get());
}

int32 SAIBStoryCards::ShownBeat() const
{
	const AIBNarration::FSequence& S = Seq();
	if (S.Beats.Num() == 0)
	{
		return INDEX_NONE;
	}
	return FMath::Max(0, AIBNarration::BeatAt(Sequence.Get(), Elapsed.Get()));
}

int32 SAIBStoryCards::CardStart(int32 Beat) const
{
	const AIBNarration::FSequence& S = Seq();
	while (Beat > 0 && S.Beats.IsValidIndex(Beat) && S.Beats[Beat - 1].Card.EqualTo(S.Beats[Beat].Card))
	{
		--Beat;
	}
	return Beat;
}

int32 SAIBStoryCards::ImageStart(int32 Beat) const
{
	const AIBNarration::FSequence& S = Seq();
	while (Beat > 0 && S.Beats.IsValidIndex(Beat) && S.Beats[Beat - 1].Image == S.Beats[Beat].Image)
	{
		--Beat;
	}
	return Beat;
}

int32 SAIBStoryCards::CardIndex(int32 Beat) const
{
	int32 Index = 0;
	for (int32 Start = CardStart(Beat); Start > 0; Start = CardStart(Start - 1))
	{
		++Index;
	}
	return Index;
}

int32 SAIBStoryCards::CardCount() const
{
	const int32 Num = Seq().Beats.Num();
	return Num > 0 ? CardIndex(Num - 1) + 1 : 0;
}

float SAIBStoryCards::BeatTime(int32 Beat) const
{
	const AIBNarration::FSequence& S = Seq();
	return Beat > 0 && S.Starts.IsValidIndex(Beat) ? S.Starts[Beat] : 0.f;
}

FName SAIBStoryCards::ImageFor(bool bPrevious) const
{
	const AIBNarration::FSequence& S = Seq();
	const int32 Shown = ShownBeat();
	if (Shown == INDEX_NONE)
	{
		return NAME_None;
	}
	if (!bPrevious)
	{
		return S.Beats[Shown].Image;
	}
	const int32 Start = ImageStart(Shown);
	return Start > 0 ? S.Beats[Start - 1].Image : NAME_None;
}

float SAIBStoryCards::ImageFade() const
{
	const int32 Shown = ShownBeat();
	return Shown == INDEX_NONE ? 0.f : FMath::Clamp((Elapsed.Get() - BeatTime(ImageStart(Shown))) / ImageFadeTime, 0.f, 1.f);
}

float SAIBStoryCards::CardFade() const
{
	const int32 Shown = ShownBeat();
	return Shown == INDEX_NONE ? 0.f : FMath::Clamp((Elapsed.Get() - BeatTime(CardStart(Shown)) - 0.3f) / 0.8f, 0.f, 1.f);
}

TOptional<FSlateRenderTransform> SAIBStoryCards::KenBurns(bool bPrevious) const
{
	const int32 Shown = ShownBeat();
	if (Shown == INDEX_NONE)
	{
		return TOptional<FSlateRenderTransform>();
	}
	int32 Start = ImageStart(Shown);
	if (bPrevious)
	{
		if (Start == 0)
		{
			return TOptional<FSlateRenderTransform>();
		}
		Start = ImageStart(Start - 1);
	}
	// A slow push-in that drifts sideways, alternating direction from one picture to the next.
	const float T = FMath::Clamp((Elapsed.Get() - BeatTime(Start)) / ZoomTime, 0.f, 1.f);
	const float Pan = bFullscreen ? 44.f : 10.f;
	const float Dir = (Start % 2 == 0) ? 1.f : -1.f;
	return FSlateRenderTransform(1.06f + 0.07f * T, FVector2D(Dir * (T - 0.5f) * Pan, (0.5f - T) * Pan * 0.35f));
}

// ---------------------------------------------------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBStoryCards::MakePicture(bool bPrevious)
{
	return SNew(SScaleBox)
		.Stretch(EStretch::ScaleToFill)
		.Visibility_Lambda([this, bPrevious]()
		{
			// The outgoing picture only matters while the new one is still fading in.
			return !bPrevious || ImageFade() < 1.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.RenderTransformPivot(FVector2D(0.5f, 0.5f))
		.RenderTransform_Lambda([this, bPrevious]() { return KenBurns(bPrevious); })
		[
			SNew(SImage)
			.Image_Lambda([this, bPrevious]() { return AIBStyle::StoryImage(ImageFor(bPrevious)); })
			.ColorAndOpacity_Lambda([this, bPrevious]() { return FSlateColor(FLinearColor(1.f, 1.f, 1.f, bPrevious ? 1.f : ImageFade())); })
		];
}

TSharedRef<SWidget> SAIBStoryCards::MakeFullscreen()
{
	// Chapter progress: one short bar per card, lit up to the current one.
	TSharedRef<SHorizontalBox> Progress = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Progress->AddSlot().AutoWidth().Padding(5.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(56.f)
			.HeightOverride(3.f)
			.Visibility_Lambda([this, Index]() { return Index < CardCount() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::RoundBrush(1.5f))
				.BorderBackgroundColor_Lambda([this, Index]()
				{
					const int32 Shown = ShownBeat();
					const int32 Current = Shown == INDEX_NONE ? 0 : CardIndex(Shown);
					return FSlateColor(Index < Current ? AIBStyle::Gold() * FLinearColor(1.f, 1.f, 1.f, 0.6f) : Index == Current ? AIBStyle::Gold() : FLinearColor(1.f, 1.f, 1.f, 0.18f));
				})
			]
		];
	}

	auto TitleColor = [this](const FLinearColor& Base)
	{
		return TAttribute<FSlateColor>::CreateLambda([this, Base]()
		{
			FLinearColor C = Base;
			C.A *= CardFade();
			return FSlateColor(C);
		});
	};

	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Ink())
		]
		+ SOverlay::Slot()[MakePicture(true)]
		+ SOverlay::Slot()[MakePicture(false)]
		// Shade the top and bottom so the words read on any picture.
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(200.f)
			[
				SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.6f), FLinearColor(0.f, 0.f, 0.f, 0.f) })
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(560.f)
			[
				SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.55f), FLinearColor(0.f, 0.f, 0.f, 0.9f) })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 44.f, 0.f, 0.f)
		[
			Progress
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(80.f, 0.f, 80.f, 110.f)
		[
			SNew(SBox)
			.MaxDesiredWidth(1180.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::SubHeading(18))
					.ColorAndOpacity(TitleColor(AIBStyle::Gold()))
					.Text_Lambda([this]()
					{
						const int32 Shown = ShownBeat();
						return Shown == INDEX_NONE ? FText::GetEmpty() : FText::Format(LOCTEXT("Chapter", "CHAPTER  {0}"), Roman(CardIndex(Shown) + 1));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::StoryBold(50))
					.Justification(ETextJustify::Center)
					.ShadowOffset(FVector2D(2.f, 2.f))
					.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
					.ColorAndOpacity(TitleColor(AIBStyle::Text()))
					.Text_Lambda([this]()
					{
						const int32 Shown = ShownBeat();
						return Shown == INDEX_NONE ? FText::GetEmpty() : Seq().Beats[Shown].Card;
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 18.f)
				[
					SNew(SBox).WidthOverride(96.f).HeightOverride(2.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda([this]() { return FSlateColor(AIBStyle::Gold() * FLinearColor(1.f, 1.f, 1.f, 0.8f * CardFade())); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Lines.ToSharedRef()
				]
			]
		];
}

TSharedRef<SWidget> SAIBStoryCards::MakeCompact()
{
	const FLinearColor PanelColor(0.012f, 0.012f, 0.016f, 0.97f);
	return SNew(SBox)
		.WidthOverride(470.f)
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::PanelSolidBrush())
			.Padding(FMargin(1.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(264.f)
					.Clipping(EWidgetClipping::ClipToBounds)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[MakePicture(true)]
						+ SOverlay::Slot()[MakePicture(false)]
						+ SOverlay::Slot().VAlign(VAlign_Bottom)
						[
							SNew(SBox).HeightOverride(150.f)
							[
								SNew(SAIBGradient).Colors({ FLinearColor(PanelColor.R, PanelColor.G, PanelColor.B, 0.f), PanelColor })
							]
						]
						+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(22.f, 0.f, 22.f, 10.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::SubHeading(14))
								.ColorAndOpacity(AIBStyle::Gold())
								.Text(LOCTEXT("AliceExplains", "ALICE EXPLAINS"))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Heading(30))
								.ColorAndOpacity(AIBStyle::Text())
								.ShadowOffset(FVector2D(1.f, 1.f))
								.AutoWrapText(true)
								.Text_Lambda([this]()
								{
									const int32 Shown = ShownBeat();
									return Shown == INDEX_NONE ? FText::GetEmpty() : Seq().Beats[Shown].Card;
								})
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(22.f, 8.f, 22.f, 4.f)
				[
					Lines.ToSharedRef()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(22.f, 10.f, 22.f, 18.f)
				[
					AIBUI::Bar(
						TAttribute<float>::CreateLambda([this]() { return Elapsed.Get() / FMath::Max(1.f, AIBNarration::Length(Sequence.Get())); }),
						AIBStyle::Gold(), 3.f)
				]
			]
		];
}

void SAIBStoryCards::RebuildLines()
{
	Lines->ClearChildren();
	const AIBNarration::FSequence& S = Seq();
	if (!S.Beats.IsValidIndex(CurrentBeat))
	{
		return;
	}
	// Only the line Alice is speaking right now, so the words on screen always match her voice.
	const AIBNarration::FLine* Line = AIBNarration::FindLine(S.Beats[CurrentBeat].Line);
	if (!Line)
	{
		return;
	}
	const float Start = S.Starts[CurrentBeat];
	auto Appear = [this, Start]() { return FMath::Clamp((Elapsed.Get() - Start) / LineFadeTime, 0.f, 1.f); };
	auto Color = [Appear](const FLinearColor& Base)
	{
		return TAttribute<FSlateColor>::CreateLambda([Appear, Base]()
		{
			FLinearColor C = Base;
			C.A *= Appear();
			return FSlateColor(C);
		});
	};
	auto Rise = [Appear]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(0.f, (1.f - Appear()) * 10.f))); };

	if (bFullscreen)
	{
		Lines->AddSlot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(Line->Text)
			.Font(AIBStyle::BodyMedium(29))
			.Justification(ETextJustify::Center)
			.WrapTextAt(1120.f)
			.LineHeightPercentage(1.08f)
			.ShadowOffset(FVector2D(1.5f, 1.5f))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
			.ColorAndOpacity(Color(AIBStyle::Text()))
			.RenderTransform_Lambda(Rise)
		];
		return;
	}

	// Rules card: the rule's number beside the sentence (the welcome and closing tip get a dash).
	const int32 Rule = RuleNumber(S.Beats[CurrentBeat].Line);
	Lines->AddSlot().AutoHeight().Padding(0.f, 4.f)
	[
		SNew(SHorizontalBox)
		.RenderTransform_Lambda(Rise)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(26.f)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(Rule > 0 ? 30 : 24))
				.Text(Rule > 0 ? FText::AsNumber(Rule) : FText::FromString(TEXT("-")))
				.ColorAndOpacity(Color(AIBStyle::Gold()))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Line->Text)
			.Font(AIBStyle::BodyMedium(19))
			.AutoWrapText(true)
			.ColorAndOpacity(Color(AIBStyle::Text()))
		]
	];
}

#undef LOCTEXT_NAMESPACE
