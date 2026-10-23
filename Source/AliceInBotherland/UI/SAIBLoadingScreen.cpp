// Alice in Botherland - loading screen and the shared settings panel

#include "UI/SAIBLoadingScreen.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "Core/AIBGameInstance.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "AIBLoading"

void SAIBLoadingScreen::Construct(const FArguments& InArgs)
{
	static const FText Tips[] =
	{
		LOCTEXT("Tip0", "THE NUMBER GAME: the target is 80% of everyone's average. If everyone thinks the same, think one step further."),
		LOCTEXT("Tip1", "THE HIDDEN SYMBOL: everyone can see your symbol except you. Not everyone will tell you the truth."),
		LOCTEXT("Tip2", "RACE TO THE GUARDIAN: winning the race only earns the first choice - not the island."),
		LOCTEXT("Tip3", "THE FINAL CHOICE: if you don't choose in time, the Guardian makes you King."),
		LOCTEXT("Tip4", "The acid sea burns. Stay on the island."),
		LOCTEXT("Tip5", "Symbols can repeat. Seeing three different symbols tells you nothing about your own.")
	};
	const FText Tip = Tips[FMath::RandRange(0, UE_ARRAY_COUNT(Tips) - 1)];
	const FSlateBrush* Background = AIBStyle::StoryImage(TEXT("L_Loading"));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(AIBStyle::WhiteBrush())
		.BorderBackgroundColor(AIBStyle::Ink())
		.Padding(0.f)
		[
			SNew(SOverlay)
			// The island at dusk behind everything, shaded towards the bottom where the text sits.
			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				.Visibility(Background ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
				[
					SNew(SImage).Image(Background)
				]
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBox).HeightOverride(520.f)
				[
					SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.6f), FLinearColor(0.f, 0.f, 0.f, 0.92f) })
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(90.f, 0.f, 90.f, 70.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "ALICE IN BOTHERLAND")).Font(AIBStyle::Title(72)).ColorAndOpacity(AIBStyle::Text())
					.ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.f, -8.f, 0.f, 18.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Sub", "THE LAST ISLAND")).Font(AIBStyle::StoryBold(22)).ColorAndOpacity(AIBStyle::Gold())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)
					[
						SNew(SThrobber)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).MaxDesiredWidth(900.f)
						[
							SNew(STextBlock).Text(Tip).Font(AIBStyle::BodyMedium(17)).ColorAndOpacity(AIBStyle::Text()).WrapTextAt(900.f)
						]
					]
				]
			]
		]
	];
}

// ---------------------------------------------------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------------------------------------------------

void SAIBSettingsPanel::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;
	OnClose = InArgs._OnClose;
	if (GameInstance.IsValid())
	{
		Working = GameInstance->GetSettings();
	}

	TSharedRef<SVerticalBox> Left = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Audio", "AUDIO"))]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Master", "Master volume"), &FAIBSettings::MasterVolume, 0.f, 1.f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Music", "Music"), &FAIBSettings::MusicVolume, 0.f, 1.f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sfx", "Effects"), &FAIBSettings::SfxVolume, 0.f, 1.f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Voice", "Narrator voice"), &FAIBSettings::VoiceVolume, 0.f, 1.f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("VoiceChat", "VOICE CHAT"))]
		+ SVerticalBox::Slot().AutoHeight()[MakeChoiceRow(LOCTEXT("Mic", "Microphone"), &FAIBSettings::MicMode, { LOCTEXT("MicOff", "OFF"), LOCTEXT("MicPtt", "HOLD V"), LOCTEXT("MicOpen", "ALWAYS ON") })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MicHelp", "Talk to the other champions. HOLD V sends your voice only while the key is held."))
			.Font(AIBStyle::Body(13)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Captions", "CAPTIONS"))]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Subtitles", "Show captions"), &FAIBSettings::bSubtitles)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("CaptionSize", "Caption size"), &FAIBSettings::SubtitleScale, 0.8f, 1.5f, EFormat::Percent)];

	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Controls", "CONTROLS & CAMERA"))]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sens", "Mouse sensitivity"), &FAIBSettings::MouseSensitivity, 0.2f, 3.f, EFormat::Value)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Invert", "Invert look up/down"), &FAIBSettings::bInvertY)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Fov", "Field of view"), &FAIBSettings::FieldOfView, 60.f, 110.f, EFormat::Degrees)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Graphics", "GRAPHICS"))]
		+ SVerticalBox::Slot().AutoHeight()[MakeChoiceRow(LOCTEXT("Quality", "Quality"), &FAIBSettings::Quality, { LOCTEXT("Low", "LOW"), LOCTEXT("Med", "MEDIUM"), LOCTEXT("High", "HIGH"), LOCTEXT("Epic", "EPIC") })]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("ResScale", "Render resolution"), &FAIBSettings::ResolutionScale, 50.f, 100.f, EFormat::Value)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Fullscreen", "Fullscreen"), &FAIBSettings::bFullscreen)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("VSync", "VSync"), &FAIBSettings::bVSync)];

	ChildSlot
	[
		SNew(SBox).WidthOverride(1060.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Settings", "SETTINGS")).Font(AIBStyle::Title(46)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 22.f, 0.f)[Left]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(1.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Line())]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(22.f, 0.f, 0.f, 0.f)[Right]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f).HAlign(HAlign_Right)
				[
					SNew(SAIBButton)
					.Text(LOCTEXT("Done", "DONE"))
					.Kind(EAIBButtonKind::Primary)
					.MinWidth(180.f)
					.OnClicked_Lambda([this]()
					{
						Commit(true);
						OnClose.ExecuteIfBound();
						return FReply::Handled();
					})
				],
				FMargin(32.f), true)
		]
	];
}

void SAIBSettingsPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Hidden panels don't tick, so a gap means the panel was just reopened: pick up changes made elsewhere since
	// (e.g. the lobby's microphone button).
	if (InCurrentTime - LastTick > 0.5 && GameInstance.IsValid())
	{
		Working = GameInstance->GetSettings();
	}
	LastTick = InCurrentTime;
}

void SAIBSettingsPanel::Commit(bool bSave)
{
	if (UAIBGameInstance* GI = GameInstance.Get())
	{
		if (bSave)
		{
			GI->SetSettings(Working);
		}
		else
		{
			GI->GetProfile()->Settings = Working;
			GI->ApplySettings();
		}
	}
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeSlider(const FText& Label, float FAIBSettings::* Field, float Min, float Max, EFormat Format)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center).Padding(0.f, 6.f)
		[
			SNew(STextBlock).Text(Label).Font(AIBStyle::BodyMedium(16)).ColorAndOpacity(AIBStyle::Text())
		]
		+ SHorizontalBox::Slot().FillWidth(0.46f).VAlign(VAlign_Center)
		[
			SNew(SSlider)
			.MinValue(Min)
			.MaxValue(Max)
			.Value_Lambda([this, Field]() { return Working.*Field; })
			.SliderBarColor(FLinearColor(1.f, 1.f, 1.f, 0.15f))
			.SliderHandleColor(AIBStyle::Blood())
			.OnValueChanged_Lambda([this, Field](float Value)
			{
				Working.*Field = Value;
				Commit(false);
			})
		]
		+ SHorizontalBox::Slot().FillWidth(0.12f).VAlign(VAlign_Center).HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Font(AIBStyle::SubHeading(16))
			.ColorAndOpacity(AIBStyle::TextDim())
			.Text_Lambda([this, Field, Format]()
			{
				const float Value = Working.*Field;
				switch (Format)
				{
				case EFormat::Percent: return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
				case EFormat::Degrees: return FText::FromString(FString::Printf(TEXT("%d\u00B0"), FMath::RoundToInt(Value)));
				default: return Value >= 10.f ? FText::FromString(FString::Printf(TEXT("%.0f%%"), Value)) : FText::FromString(FString::Printf(TEXT("%.2f"), Value));
				}
			})
		];
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeToggle(const FText& Label, bool FAIBSettings::* Field)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 6.f)
		[
			SNew(STextBlock).Text(Label).Font(AIBStyle::BodyMedium(16)).ColorAndOpacity(AIBStyle::Text())
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this, Field]() { return Working.*Field ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, Field](ECheckBoxState State)
			{
				Working.*Field = State == ECheckBoxState::Checked;
				AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
				Commit(false);
			})
		];
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeChoiceRow(const FText& Label, int32 FAIBSettings::* Field, TArray<FText> Names)
{
	// Label above, choices across the full width (so longer words like MEDIUM fit).
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Choice = 0; Choice < Names.Num(); ++Choice)
	{
		Row->AddSlot().FillWidth(1.f).Padding(Choice == 0 ? 0.f : 3.f, 0.f, Choice == Names.Num() - 1 ? 0.f : 3.f, 0.f)
		[
			SNew(SAIBButton)
			.Text(Names[Choice])
			.FontSize(14)
			.Height(36.f)
			.Kind(EAIBButtonKind::Secondary)
			.AccentColor_Lambda([this, Field, Choice]() { return Working.*Field == Choice ? AIBStyle::Blood() : FLinearColor::Transparent; })
			.OnClicked_Lambda([this, Field, Choice]()
			{
				Working.*Field = Choice;
				Commit(false);
				return FReply::Handled();
			})
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 4.f)
		[
			SNew(STextBlock).Text(Label).Font(AIBStyle::BodyMedium(16)).ColorAndOpacity(AIBStyle::Text())
		]
		+ SVerticalBox::Slot().AutoHeight()[Row];
}

#undef LOCTEXT_NAMESPACE
