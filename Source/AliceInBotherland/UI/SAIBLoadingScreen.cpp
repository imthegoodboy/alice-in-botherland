// Alice in Botherland - loading screen and the shared settings panel

#include "UI/SAIBLoadingScreen.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBGameUI.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SScrollBox.h"
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

	struct FTab { TCHAR Icon; FText Name; };
	const FTab Tabs[] =
	{
		{ AIBIcon::Volume, LOCTEXT("TabAudio", "AUDIO") },
		{ AIBIcon::Voice, LOCTEXT("TabVoice", "VOICE & CAPTIONS") },
		{ AIBIcon::Gamepad, LOCTEXT("TabControls", "CONTROLS") },
		{ AIBIcon::Tune, LOCTEXT("TabGraphics", "GRAPHICS") }
	};
	TSharedRef<SVerticalBox> Rail = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 26.f)
		[
			AIBUI::PageTitle(LOCTEXT("Options", "OPTIONS"), LOCTEXT("Settings", "SETTINGS"), 60)
		];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
	{
		Rail->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SAIBReveal).Delay(0.05f * Index)
			[
				SNew(SAIBSlantButton)
				.Icon(Tabs[Index].Icon)
				.Text(Tabs[Index].Name)
				.FontSize(19)
				.Height(54.f)
				.IsSelected_Lambda([this, Index]() { return Tab == Index; })
				.OnClicked_Lambda([this, Index]() { Tab = Index; return FReply::Handled(); })
			]
		];
	}
	Rail->AddSlot().FillHeight(1.f)[SNullWidget::NullWidget];
	Rail->AddSlot().AutoHeight().Padding(0.f, 4.f)
	[
		SNew(SAIBSlantButton)
		.Kind(EAIBButtonKind::Ghost)
		.Icon(AIBIcon::Restart)
		.Text(LOCTEXT("Reset", "RESET TO DEFAULTS"))
		.FontSize(16)
		.Height(44.f)
		.OnClicked_Lambda([this]() { ResetDefaults(); return FReply::Handled(); })
	];
	Rail->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(SAIBSlantButton)
		.Kind(EAIBButtonKind::Primary)
		.Icon(AIBIcon::Check)
		.Text(LOCTEXT("Done", "SAVE & CLOSE"))
		.FontSize(22)
		.Height(62.f)
		.Shine(true)
		.OnClicked_Lambda([this]()
		{
			Commit(true);
			OnClose.ExecuteIfBound();
			return FReply::Handled();
		})
	];

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(1260.f)
		.HeightOverride(780.f)
		[
			AIBUI::Frame(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(310.f)[Rail]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(28.f, 0.f)
				[
					SNew(SBox).WidthOverride(1.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.08f))]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex_Lambda([this]() { return Tab; })
					+ SWidgetSwitcher::Slot()[MakeAudioTab()]
					+ SWidgetSwitcher::Slot()[MakeVoiceTab()]
					+ SWidgetSwitcher::Slot()[MakeControlsTab()]
					+ SWidgetSwitcher::Slot()[MakeGraphicsTab()]
				],
				FMargin(36.f, 34.f), FLinearColor(1.f, 0.74f, 0.32f, 0.9f), 0.9f)
		]
	];
}

void SAIBSettingsPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Hidden panels don't tick, so a gap means the panel was just reopened: pick up changes made elsewhere since
	// (e.g. the room's microphone button or the pause menu's quick settings).
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

void SAIBSettingsPanel::ResetDefaults()
{
	const FString Name = Working.PlayerName;
	Working = FAIBSettings();
	Working.PlayerName = Name;
	Commit(false);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakePage(const FText& Title, const FText& Description, TSharedRef<SVerticalBox> Rows)
{
	return SNew(SScrollBox)
		.ScrollBarThickness(FVector2D(6.f, 6.f))
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Title).Font(AIBStyle::Spaced(AIBStyle::Title(42), 30)).ColorAndOpacity(AIBStyle::Text())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
			[
				SNew(STextBlock).Text(Description).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 14.f, 10.f)[Rows]
		];
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeRow(const FText& Label, const FText& Help, TSharedRef<SWidget> Control)
{
	TSharedRef<SVerticalBox> Words = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(Label).Font(AIBStyle::BodyMedium(19)).ColorAndOpacity(AIBStyle::Text())
		];
	if (!Help.IsEmpty())
	{
		Words->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(Help).Font(AIBStyle::Body(13)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
		];
	}
	return SNew(SBox)
		.Padding(FMargin(0.f, 3.f))
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::WhiteBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.035f))
			.Padding(FMargin(22.f, 12.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 20.f, 0.f)[Words]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(400.f).HAlign(HAlign_Right)[Control]
				]
			]
		];
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeSlider(const FText& Label, const FText& Help, float FAIBSettings::* Field, float Min, float Max, float Step, EFormat Format)
{
	return MakeRow(Label, Help,
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SAIBSlider)
			.Width(300.f)
			.MinValue(Min)
			.MaxValue(Max)
			.StepSize(Step)
			.Value_Lambda([this, Field]() { return Working.*Field; })
			.OnValueChanged_Lambda([this, Field](float Value)
			{
				Working.*Field = Value;
				Commit(false);
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(84.f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(28))
				.ColorAndOpacity(AIBStyle::Gold())
				.Text_Lambda([this, Field, Format]()
				{
					const float Value = Working.*Field;
					switch (Format)
					{
					case EFormat::Percent: return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
					case EFormat::Degrees: return FText::FromString(FString::Printf(TEXT("%d°"), FMath::RoundToInt(Value)));
					case EFormat::Whole: return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value)));
					default: return FText::FromString(FString::Printf(TEXT("%.2f"), Value));
					}
				})
			]
		]);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeToggle(const FText& Label, const FText& Help, bool FAIBSettings::* Field)
{
	return MakeRow(Label, Help,
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::Spaced(AIBStyle::Heading(16), 120))
			.Text_Lambda([this, Field]() { return Working.*Field ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF"); })
			.ColorAndOpacity_Lambda([this, Field]() { return FSlateColor(Working.*Field ? AIBStyle::Ember() : AIBStyle::TextFaint()); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SAIBToggle)
			.IsOn_Lambda([this, Field]() { return Working.*Field; })
			.OnToggled_Lambda([this, Field](bool bOn)
			{
				Working.*Field = bOn;
				Commit(false);
			})
		]);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeChoice(const FText& Label, const FText& Help, int32 FAIBSettings::* Field, TArray<FText> Names)
{
	TSharedRef<SHorizontalBox> Choices = SNew(SHorizontalBox);
	for (int32 Choice = 0; Choice < Names.Num(); ++Choice)
	{
		Choices->AddSlot().FillWidth(1.f).Padding(3.f, 0.f)
		[
			SNew(SAIBSlantButton)
			.Text(Names[Choice])
			.FontSize(15)
			.Height(40.f)
			.Centered(true)
			.IsSelected_Lambda([this, Field, Choice]() { return Working.*Field == Choice; })
			.OnClicked_Lambda([this, Field, Choice]()
			{
				Working.*Field = Choice;
				Commit(false);
				return FReply::Handled();
			})
		];
	}
	return MakeRow(Label, Help, SNew(SBox).WidthOverride(400.f)[Choices]);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeAudioTab()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Master", "Master volume"), FText::GetEmpty(), &FAIBSettings::MasterVolume, 0.f, 1.f, 0.01f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Music", "Music"), LOCTEXT("MusicHelp", "The score under the story, the trials and the endings."), &FAIBSettings::MusicVolume, 0.f, 1.f, 0.01f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sfx", "Effects"), LOCTEXT("SfxHelp", "Waves, engines, menus and the island around you."), &FAIBSettings::SfxVolume, 0.f, 1.f, 0.01f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Voice", "Narrator voice"), LOCTEXT("VoiceHelp", "Alice, who tells the story and explains every trial."), &FAIBSettings::VoiceVolume, 0.f, 1.f, 0.01f, EFormat::Percent)];
	return MakePage(LOCTEXT("AudioTitle", "AUDIO"), LOCTEXT("AudioDesc", "Balance the music, the island and the narrator."), Rows);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeVoiceTab()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MakeChoice(LOCTEXT("Mic", "Microphone"), LOCTEXT("MicHelp", "HOLD V sends your voice only while the key is held (gamepad: D-pad up)."), &FAIBSettings::MicMode, { LOCTEXT("MicOff", "OFF"), LOCTEXT("MicPtt", "HOLD V"), LOCTEXT("MicOpen", "ALWAYS ON") })]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Subtitles", "Captions"), LOCTEXT("SubtitlesHelp", "Show what is being said at the bottom of the screen."), &FAIBSettings::bSubtitles)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("CaptionSize", "Caption size"), LOCTEXT("CaptionSizeHelp", "Make captions bigger if they are hard to read."), &FAIBSettings::SubtitleScale, 0.8f, 1.5f, 0.05f, EFormat::Percent)];
	return MakePage(LOCTEXT("VoiceTitle", "VOICE & CAPTIONS"), LOCTEXT("VoiceDesc", "Talk to the other champions and read along with Alice."), Rows);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeControlsTab()
{
	struct FBind { const TCHAR* Action; const TCHAR* Key; };
	const FBind Binds[] =
	{
		{ TEXT("Move"), TEXT("W A S D") }, { TEXT("Look"), TEXT("MOUSE") }, { TEXT("Sprint"), TEXT("SHIFT") }, { TEXT("Jump / skip"), TEXT("SPACE") },
		{ TEXT("Interact"), TEXT("E") }, { TEXT("Voice chat"), TEXT("V") }, { TEXT("Text chat"), TEXT("ENTER") }, { TEXT("Scores"), TEXT("TAB") },
		{ TEXT("Menu"), TEXT("ESC") }, { TEXT("Handbrake"), TEXT("SPACE") }, { TEXT("Reset car"), TEXT("R") }, { TEXT("Spectate"), TEXT("MOUSE L / R") }
	};
	TSharedRef<SVerticalBox> Keys = SNew(SVerticalBox);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Binds); Index += 2)
	{
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox);
		for (int32 Col = 0; Col < 2 && Index + Col < UE_ARRAY_COUNT(Binds); ++Col)
		{
			const FBind& Bind = Binds[Index + Col];
			Line->AddSlot().FillWidth(1.f).Padding(0.f, 3.f, 18.f, 3.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(Bind.Action)).Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::OutlineBrush(FLinearColor(1.f, 1.f, 1.f, 0.3f), 4.f, 1.f))
					.Padding(FMargin(10.f, 2.f))
					[
						SNew(STextBlock).Text(FText::FromString(Bind.Key)).Font(AIBStyle::Spaced(AIBStyle::Heading(15), 80)).ColorAndOpacity(AIBStyle::Text())
					]
				]
			];
		}
		Keys->AddSlot().AutoHeight()[Line];
	}

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Sens", "Mouse sensitivity"), LOCTEXT("SensHelp", "How fast the camera turns."), &FAIBSettings::MouseSensitivity, 0.2f, 3.f, 0.05f, EFormat::Value)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Invert", "Invert look"), LOCTEXT("InvertHelp", "Moving the mouse up looks down."), &FAIBSettings::bInvertY)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Fov", "Field of view"), LOCTEXT("FovHelp", "Wider shows more of the island around your champion."), &FAIBSettings::FieldOfView, 60.f, 110.f, 1.f, EFormat::Degrees)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 8.f)[AIBUI::Header(LOCTEXT("KeysHeader", "KEYS"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.035f)).Padding(FMargin(22.f, 12.f))[Keys]
		];
	return MakePage(LOCTEXT("ControlsTitle", "CONTROLS"), LOCTEXT("ControlsDesc", "Camera, mouse and the keys for every action. Gamepads work too."), Rows);
}

TSharedRef<SWidget> SAIBSettingsPanel::MakeGraphicsTab()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MakeChoice(LOCTEXT("Quality", "Quality"), LOCTEXT("QualityHelp", "Lower it if the game stutters."), &FAIBSettings::Quality, { LOCTEXT("Low", "LOW"), LOCTEXT("Med", "MEDIUM"), LOCTEXT("High", "HIGH"), LOCTEXT("Epic", "EPIC") })]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("ResScale", "Render resolution"), LOCTEXT("ResScaleHelp", "Below 100% is faster but softer."), &FAIBSettings::ResolutionScale, 50.f, 100.f, 1.f, EFormat::Whole)]
		+ SVerticalBox::Slot().AutoHeight()[MakeSlider(LOCTEXT("Brightness", "Brightness"), LOCTEXT("BrightnessHelp", "Raise it if the night scenes look too dark."), &FAIBSettings::Brightness, 0.7f, 1.3f, 0.01f, EFormat::Percent)]
		+ SVerticalBox::Slot().AutoHeight()[MakeChoice(LOCTEXT("FpsCap", "Frame rate limit"), LOCTEXT("FpsCapHelp", "A limit keeps a laptop cooler and quieter."), &FAIBSettings::FrameRateIndex, { LOCTEXT("Fps30", "30"), LOCTEXT("Fps60", "60"), LOCTEXT("Fps120", "120"), LOCTEXT("FpsMax", "MAX") })]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("MotionBlur", "Motion blur"), FText::GetEmpty(), &FAIBSettings::bMotionBlur)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("ShowFps", "Show FPS"), LOCTEXT("ShowFpsHelp", "Frame counter in the top corner."), &FAIBSettings::bShowFPS)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("Fullscreen", "Fullscreen"), FText::GetEmpty(), &FAIBSettings::bFullscreen)]
		+ SVerticalBox::Slot().AutoHeight()[MakeToggle(LOCTEXT("VSync", "VSync"), LOCTEXT("VSyncHelp", "Stops tearing; can add a little input delay."), &FAIBSettings::bVSync)];
	return MakePage(LOCTEXT("GraphicsTitle", "GRAPHICS"), LOCTEXT("GraphicsDesc", "Picture quality, brightness and performance."), Rows);
}

#undef LOCTEXT_NAMESPACE
