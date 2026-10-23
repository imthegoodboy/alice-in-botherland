// Alice in Botherland - loading screen and the shared settings panel

#include "UI/SAIBLoadingScreen.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "Core/AIBGameInstance.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
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
	const FSlateBrush* Emblem = AIBStyle::Image(TEXT("Logo_Emblem.png"), FVector2D(220.f, 220.f));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(AIBStyle::WhiteBrush())
		.BorderBackgroundColor(AIBStyle::Ink())
		.Padding(0.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SBox).WidthOverride(220.f).HeightOverride(220.f)
					.Visibility(Emblem ? EVisibility::Visible : EVisibility::Collapsed)
					[
						SNew(SImage).Image(Emblem).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.85f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 18.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "ALICE IN BOTHERLAND")).Font(AIBStyle::Title(64)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Sub", "THE LAST ISLAND")).Font(AIBStyle::StoryBold(20)).ColorAndOpacity(AIBStyle::Blood())
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(80.f, 0.f, 80.f, 70.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SThrobber)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 0.f)
				[
					SNew(SBox).MaxDesiredWidth(900.f)
					[
						SNew(STextBlock).Text(Tip).Font(AIBStyle::Body(17)).ColorAndOpacity(AIBStyle::TextDim()).WrapTextAt(900.f).Justification(ETextJustify::Center)
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

	ChildSlot
	[
		SNew(SBox).WidthOverride(620.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Settings", "SETTINGS")).Font(AIBStyle::Title(44)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)[AIBUI::Header(LOCTEXT("Audio", "AUDIO"))]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Master", "Master volume"), &FAIBSettings::MasterVolume, 0.f, 1.f, true)]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Music", "Music"), &FAIBSettings::MusicVolume, 0.f, 1.f, true)]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sfx", "Effects"), &FAIBSettings::SfxVolume, 0.f, 1.f, true)]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Voice", "Voice"), &FAIBSettings::VoiceVolume, 0.f, 1.f, true)]
				+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Subtitles", "Subtitles"), &FAIBSettings::bSubtitles)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 10.f)[AIBUI::Header(LOCTEXT("Controls", "CONTROLS"))]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sens", "Mouse sensitivity"), &FAIBSettings::MouseSensitivity, 0.2f, 3.f, false)]
				+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Invert", "Invert look up/down"), &FAIBSettings::bInvertY)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 10.f)[AIBUI::Header(LOCTEXT("Graphics", "GRAPHICS"))]
				+ SVerticalBox::Slot().AutoHeight()[MakeQualityRow()]
				+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("ResScale", "Render resolution"), &FAIBSettings::ResolutionScale, 50.f, 100.f, false)]
				+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Fullscreen", "Fullscreen"), &FAIBSettings::bFullscreen)]
				+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("VSync", "VSync"), &FAIBSettings::bVSync)]
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

TSharedRef<SWidget> SAIBSettingsPanel::MakeSlider(const FText& Label, float FAIBSettings::* Field, float Min, float Max, bool bPercent)
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
			.Text_Lambda([this, Field, bPercent]()
			{
				const float Value = Working.*Field;
				if (bPercent)
				{
					return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
				}
				return Value >= 10.f ? FText::FromString(FString::Printf(TEXT("%.0f%%"), Value)) : FText::FromString(FString::Printf(TEXT("%.2f"), Value));
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

TSharedRef<SWidget> SAIBSettingsPanel::MakeQualityRow()
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center).Padding(0.f, 6.f)
		[
			SNew(STextBlock).Text(LOCTEXT("Quality", "Quality")).Font(AIBStyle::BodyMedium(16)).ColorAndOpacity(AIBStyle::Text())
		];
	const FText Names[] = { LOCTEXT("Low", "LOW"), LOCTEXT("Med", "MEDIUM"), LOCTEXT("High", "HIGH"), LOCTEXT("Epic", "EPIC") };
	for (int32 Level = 0; Level < 4; ++Level)
	{
		Row->AddSlot().FillWidth(0.175f).Padding(3.f, 0.f)
		[
			SNew(SAIBButton)
			.Text(Names[Level])
			.FontSize(14)
			.Height(36.f)
			.Kind(EAIBButtonKind::Secondary)
			.AccentColor_Lambda([this, Level]() { return Working.Quality == Level ? AIBStyle::Blood() : FLinearColor::Transparent; })
			.OnClicked_Lambda([this, Level]()
			{
				Working.Quality = Level;
				Commit(false);
				return FReply::Handled();
			})
		];
	}
	return Row;
}

#undef LOCTEXT_NAMESPACE
