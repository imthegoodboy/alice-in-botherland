// Alice in Botherland - main menu, prologue, lobby browser, hall of champions

#include "UI/SAIBMainMenu.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBHud.h"
#include "UI/SAIBLoadingScreen.h"
#include "Game/AIBMenuGameMode.h"
#include "Core/AIBGameInstance.h"
#include "Core/AIBAssets.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Online/AIBSessionSubsystem.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBMainMenu"

UAIBGameInstance* SAIBMainMenu::GI() const
{
	return Owner.IsValid() ? Owner->GetGameInstance<UAIBGameInstance>() : nullptr;
}

UAIBSessionSubsystem* SAIBMainMenu::Sessions() const
{
	return Owner.IsValid() && Owner->GetGameInstance() ? Owner->GetGameInstance()->GetSubsystem<UAIBSessionSubsystem>() : nullptr;
}

void SAIBMainMenu::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	PrologueLines = { TEXT("VO_Prologue_01"), TEXT("VO_Prologue_02"), TEXT("VO_Prologue_03"), TEXT("VO_Prologue_04"), TEXT("VO_Prologue_05"), TEXT("VO_Prologue_06") };
	if (UAIBGameInstance* Inst = GI())
	{
		MenuMessage = Inst->PendingMenuMessage;
		if (!MenuMessage.IsEmpty())
		{
			MenuMessageTime = FPlatformTime::Seconds();
			Inst->PendingMenuMessage = FText::GetEmpty();
		}
	}

	const FSlateBrush* Vignette = AIBStyle::Image(TEXT("Vignette.png"), FVector2D(1024.f, 1024.f));
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Visibility(Vignette ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			.Image(Vignette)
		]
		+ SOverlay::Slot()[PageBox(EPage::Home, MakeHome())]
		+ SOverlay::Slot()[PageBox(EPage::Join, MakeJoin())]
		+ SOverlay::Slot()
		[
			PageBox(EPage::Settings,
				SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SAIBSettingsPanel).GameInstance(GI()).OnClose_Lambda([this]() { Go(EPage::Home); })
				])
		]
		+ SOverlay::Slot()[PageBox(EPage::Prologue, MakePrologue())]
		+ SOverlay::Slot()[PageBox(EPage::HowTo, MakeHowTo())]
		+ SOverlay::Slot()[PageBox(EPage::Hall, MakeHall())]
		+ SOverlay::Slot()[PageBox(EPage::Credits, MakeCredits())]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 30.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return FPlatformTime::Seconds() - MenuMessageTime < 8.0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderImage(AIBStyle::PanelSolidBrush())
			.Padding(FMargin(20.f, 12.f))
			[
				SNew(STextBlock).Text_Lambda([this]() { return MenuMessage; }).Font(AIBStyle::BodyMedium(18)).ColorAndOpacity(AIBStyle::Gold())
			]
		]
	];

	// -QuickPlay (e.g. on a desktop shortcut) goes straight into a solo match against four AI champions, once per launch.
	static bool bQuickPlayUsed = false;
	if (!bQuickPlayUsed && FParse::Param(FCommandLine::Get(), TEXT("QuickPlay")) && Sessions())
	{
		bQuickPlayUsed = true;
		Sessions()->HostLobby(true, 4, true);
		return;
	}

	UAIBGameInstance* Inst = GI();
	if (Inst && !Inst->GetProfile()->bSeenPrologue)
	{
		StartPrologue();
	}
}

TSharedRef<SWidget> SAIBMainMenu::PageBox(EPage Page, TSharedRef<SWidget> Content)
{
	return SNew(SBox)
		.Visibility_Lambda([this, Page]() { return Current == Page ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		[
			Content
		];
}

void SAIBMainMenu::Go(EPage Page)
{
	if (Current == EPage::Prologue && Page != EPage::Prologue)
	{
		StopPrologue();
	}
	Current = Page;
	if (Page == EPage::Join && Sessions())
	{
		Sessions()->FindLobbies(bLanSearch);
		ShownResults = -1;
	}
	if (Page == EPage::Hall)
	{
		RebuildHall();
	}
}

FReply SAIBMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		if (Current != EPage::Home)
		{
			Go(EPage::Home);
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

// ---------------------------------------------------------------------------------------------------------------------
// Home
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakeHome()
{
	auto Button = [this](const FText& Text, const FText& Sub, EAIBButtonKind Kind, TFunction<void()> Action)
	{
		return SNew(SAIBButton)
			.Text(Text)
			.SubText(Sub)
			.Kind(Kind)
			.FontSize(24)
			.Height(Sub.IsEmpty() ? 50.f : 62.f)
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
	};

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(90.f, 40.f, 0.f, 40.f)
		[
			SNew(SBox).WidthOverride(580.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Title1", "ALICE IN")).Font(AIBStyle::Title(64)).ColorAndOpacity(AIBStyle::TextDim()).ShadowOffset(FVector2D(2.f, 2.f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, -18.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Title2", "BOTHERLAND")).Font(AIBStyle::Title(104)).ColorAndOpacity(AIBStyle::Text()).ShadowOffset(FVector2D(3.f, 3.f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, -10.f, 0.f, 6.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Subtitle", "THE  LAST  ISLAND")).Font(AIBStyle::StoryBold(26)).ColorAndOpacity(AIBStyle::Blood())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 0.f, 28.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Tagline", "Five communities. Three trials. One island. And a choice no champion is ready to make."))
					.Font(AIBStyle::Story(16)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("ChampionName", "CHAMPION NAME")).Font(AIBStyle::SubHeading(16)).ColorAndOpacity(AIBStyle::TextDim())
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SEditableTextBox)
						.Font(AIBStyle::Heading(20))
						.Text_Lambda([this]() { return FText::FromString(GI() ? GI()->GetSettings().PlayerName : FString()); })
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
						{
							if (UAIBGameInstance* Inst = GI())
							{
								FAIBSettings Settings = Inst->GetSettings();
								Settings.PlayerName = Text.ToString().Left(16).TrimStartAndEnd();
								if (!Settings.PlayerName.IsEmpty())
								{
									Inst->SetSettings(Settings);
								}
							}
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("PlaySolo", "PLAY  -  SOLO VS AI"), LOCTEXT("PlaySoloSub", "You and four AI champions"), EAIBButtonKind::Primary,
						[this]() { if (Sessions()) Sessions()->HostLobby(true, 4); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("Host", "HOST A LOBBY"), LOCTEXT("HostSub", "Invite up to four friends - empty seats can be AI"), EAIBButtonKind::Secondary,
						[this]() { if (Sessions()) Sessions()->HostLobby(false, 0); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("Join", "JOIN A LOBBY"), LOCTEXT("JoinSub", "Browse lobbies or connect by address"), EAIBButtonKind::Secondary,
						[this]() { Go(EPage::Join); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)[Button(LOCTEXT("Prologue", "PROLOGUE"), FText::GetEmpty(), EAIBButtonKind::Secondary, [this]() { StartPrologue(); })]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f, 0.f, 0.f)[Button(LOCTEXT("HowTo", "HOW TO PLAY"), FText::GetEmpty(), EAIBButtonKind::Secondary, [this]() { Go(EPage::HowTo); })]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)[Button(LOCTEXT("Hall", "HALL OF CHAMPIONS"), FText::GetEmpty(), EAIBButtonKind::Secondary, [this]() { Go(EPage::Hall); })]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f, 0.f, 0.f)[Button(LOCTEXT("Settings", "SETTINGS"), FText::GetEmpty(), EAIBButtonKind::Secondary, [this]() { Go(EPage::Settings); })]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)[Button(LOCTEXT("Credits", "CREDITS"), FText::GetEmpty(), EAIBButtonKind::Ghost, [this]() { Go(EPage::Credits); })]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f, 0.f, 0.f)
					[
						Button(LOCTEXT("Quit", "QUIT"), FText::GetEmpty(), EAIBButtonKind::Ghost, [this]()
						{
							if (Owner.IsValid())
							{
								UKismetSystemLibrary::QuitGame(Owner.Get(), Owner.Get(), EQuitPreference::Quit, false);
							}
						})
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 30.f, 22.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::Body(13))
			.ColorAndOpacity(AIBStyle::TextFaint())
			.Text_Lambda([this]()
			{
				const FString Platform = Sessions() ? Sessions()->GetPlatformName() : TEXT("?");
				return FText::Format(LOCTEXT("Footer", "v1.0.1  -  online: {0}  -  {1}"), FText::FromString(Platform),
					Sessions() && Sessions()->HasOnlinePlatform() ? LOCTEXT("SteamOn", "friend invites available") : LOCTEXT("SteamOff", "start Steam for friend invites; LAN / direct IP always works"));
			})
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Join
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakeJoin()
{
	SAssignNew(LobbyList, SVerticalBox);
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(820.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("JoinTitle", "JOIN A LOBBY")).Font(AIBStyle::Title(52)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
					[
						SNew(STextBlock).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Gold())
						.Text_Lambda([this]() { return Sessions() ? Sessions()->GetStatus() : FText::GetEmpty(); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
							.IsChecked_Lambda([this]() { return bLanSearch ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bLanSearch = State == ECheckBoxState::Checked; })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f)
						[
							SNew(STextBlock).Text(LOCTEXT("LanOnly", "Search my local network (LAN)")).Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::Text())
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SAIBButton).Text(LOCTEXT("Refresh", "REFRESH")).FontSize(18).Height(42.f)
							.IsEnabled_Lambda([this]() { return Sessions() && !Sessions()->IsSearching(); })
							.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->FindLobbies(bLanSearch); ShownResults = -1; return FReply::Handled(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f)
					[
						SNew(SBox).HeightOverride(300.f)
						[
							SNew(SScrollBox) + SScrollBox::Slot()[LobbyList.ToSharedRef()]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Direct", "OR CONNECT DIRECTLY"))]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(SEditableTextBox)
							.Font(AIBStyle::Body(18))
							.HintText(LOCTEXT("AddrHint", "Host address, e.g. 192.168.1.20 or 203.0.113.5:7777"))
							.OnTextChanged_Lambda([this](const FText& Text) { DirectAddress = Text.ToString(); })
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SAIBButton).Kind(EAIBButtonKind::Primary).Text(LOCTEXT("Connect", "CONNECT")).FontSize(18).Height(44.f)
							.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->JoinByAddress(DirectAddress); return FReply::Handled(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f).HAlign(HAlign_Right)
					[
						SNew(SAIBButton).Text(LOCTEXT("Back", "BACK")).MinWidth(160.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					],
					FMargin(32.f), true)
			]
		];
}

void SAIBMainMenu::RebuildLobbyList()
{
	if (!LobbyList.IsValid() || !Sessions())
	{
		return;
	}
	LobbyList->ClearChildren();
	const TArray<FAIBLobbyInfo>& Results = Sessions()->GetResults();
	if (Results.Num() == 0)
	{
		LobbyList->AddSlot().AutoHeight().Padding(0.f, 20.f)
		[
			SNew(STextBlock)
			.Text(Sessions()->IsSearching() ? LOCTEXT("SearchingList", "Searching...") : LOCTEXT("NoLobbies", "No lobbies found yet."))
			.Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::TextDim())
		];
		return;
	}
	for (const FAIBLobbyInfo& Info : Results)
	{
		const int32 Index = Info.SearchIndex;
		LobbyList->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(14.f, 8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("LobbyOf", "{0}'s lobby"), FText::FromString(Info.HostName))).Font(AIBStyle::Heading(20)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f)
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("Seats", "{0}/{1} champions  -  {2} ms"), Info.Players, Info.MaxPlayers, Info.Ping)).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SAIBButton).Kind(EAIBButtonKind::Primary).Text(LOCTEXT("JoinBtn", "JOIN")).FontSize(18).Height(40.f)
					.OnClicked_Lambda([this, Index]() { if (Sessions()) Sessions()->JoinLobby(Index); return FReply::Handled(); })
				]
			]
		];
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Prologue
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakePrologue()
{
	auto LineAlpha = [this]()
	{
		if (PrologueIndex == INDEX_NONE)
		{
			return 0.f;
		}
		const float T = float(FPlatformTime::Seconds() - LineStart);
		return FMath::Min(FMath::Clamp(T / 0.8f, 0.f, 1.f), FMath::Clamp((LineDuration - T) / 0.8f, 0.f, 1.f));
	};
	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(80.f, 0.f, 80.f, 140.f)
		[
			SNew(SBox).MaxDesiredWidth(1200.f)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Story(30))
				.Justification(ETextJustify::Center)
				.WrapTextAt(1100.f)
				.ShadowOffset(FVector2D(2.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f))
				.Text_Lambda([this]() { return PrologueLines.IsValidIndex(PrologueIndex) ? UAIBAudioSubsystem::VoiceLineText(PrologueLines[PrologueIndex]) : FText::GetEmpty(); })
				.ColorAndOpacity_Lambda([LineAlpha]() { return FSlateColor(FLinearColor(0.96f, 0.93f, 0.88f, LineAlpha())); })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 40.f, 40.f)
		[
			SNew(SAIBButton).Text(LOCTEXT("SkipPrologue", "SKIP")).Kind(EAIBButtonKind::Ghost).FontSize(18)
			.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 60.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(LOCTEXT("PrologueTitle", "THE LONG BURN")).Font(AIBStyle::StoryBold(22)).ColorAndOpacity(FLinearColor(0.85f, 0.1f, 0.12f, 0.8f))
		];
}

void SAIBMainMenu::StartPrologue()
{
	Current = EPage::Prologue;
	PrologueIndex = INDEX_NONE;
	PrologueStart = FPlatformTime::Seconds();
	// Total duration for the camera sweep.
	PrologueTotal = 0.f;
	for (const FName& Line : PrologueLines)
	{
		USoundBase* Sound = AIBAssets::Sound(AIBAssets::Paths::Audio + TEXT("VO/") + Line.ToString());
		PrologueTotal += FMath::Max(5.5f, (Sound ? Sound->GetDuration() : 6.f) + 1.2f);
	}
	if (UAIBGameInstance* Inst = GI())
	{
		Inst->GetProfile()->bSeenPrologue = true;
		Inst->SaveProfile();
	}
	if (Owner.IsValid() && Owner->GetGameInstance())
	{
		if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->PlayMusic(TEXT("MUS_Arrival"));
		}
	}
	LineStart = 0.0;
	LineDuration = 0.f;
}

void SAIBMainMenu::StopPrologue()
{
	PrologueIndex = INDEX_NONE;
	if (Owner.IsValid())
	{
		Owner->PrologueAlpha = -1.f;
		if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->PlayMusic(TEXT("MUS_Menu"));
			Audio->PlayVoice(NAME_None);
		}
	}
}

void SAIBMainMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (Current == EPage::Prologue)
	{
		const double Now = FPlatformTime::Seconds();
		if (PrologueIndex == INDEX_NONE || Now - LineStart > LineDuration)
		{
			++PrologueIndex;
			if (!PrologueLines.IsValidIndex(PrologueIndex))
			{
				Go(EPage::Home);
				return;
			}
			LineStart = Now;
			USoundBase* Sound = AIBAssets::Sound(AIBAssets::Paths::Audio + TEXT("VO/") + PrologueLines[PrologueIndex].ToString());
			LineDuration = FMath::Max(5.5f, (Sound ? Sound->GetDuration() : 6.f) + 1.2f);
			if (Owner.IsValid())
			{
				if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
				{
					Audio->PlayVoice(PrologueLines[PrologueIndex]);
				}
			}
		}
		if (Owner.IsValid())
		{
			Owner->PrologueAlpha = FMath::Clamp(float(Now - PrologueStart) / FMath::Max(1.f, PrologueTotal), 0.f, 1.f);
		}
	}

	if (Current == EPage::Join && Sessions())
	{
		const bool bSearching = Sessions()->IsSearching();
		if (ShownResults != Sessions()->GetResults().Num() || bWasSearching != bSearching)
		{
			ShownResults = Sessions()->GetResults().Num();
			bWasSearching = bSearching;
			RebuildLobbyList();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// How to play, Hall, Credits
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakeHowTo()
{
	const FText Controls = LOCTEXT("Controls",
		"ON FOOT  -  WASD move, Mouse look, Shift sprint, Space jump\n"
		"DRIVING  -  W/S throttle and brake, A/D steer, Space handbrake, R reset, hold E on the switch\n"
		"ALWAYS  -  Enter chat, Tab scoreboard, Esc menu.  Gamepads are supported too.");
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("HowToTitle", "HOW TO PLAY")).Font(AIBStyle::Title(52)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("HowToIntro", "Five champions land on the last island. After a short time to explore, the Guardian's three trials begin. Only two will reach the end - and then one terrible choice decides who wins and which community survives."))
						.Font(AIBStyle::Body(17)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()[SAIBHud::MakeRules(false)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 4.f)[AIBUI::Header(LOCTEXT("ControlsHeader", "CONTROLS"))]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Controls).Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("Back2", "BACK")).MinWidth(160.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					],
					FMargin(34.f), true)
			]
		];
}

TSharedRef<SWidget> SAIBMainMenu::MakeHall()
{
	SAssignNew(HallList, SVerticalBox);
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(880.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("HallTitle", "HALL OF CHAMPIONS")).Font(AIBStyle::Title(52)).ColorAndOpacity(AIBStyle::Gold())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
					[
						SNew(STextBlock)
						.Font(AIBStyle::Heading(19))
						.ColorAndOpacity(AIBStyle::Text())
						.Text_Lambda([this]()
						{
							const UAIBSaveGame* P = GI() ? GI()->GetProfile() : nullptr;
							return P ? FText::Format(LOCTEXT("Stats", "MATCHES {0}   -   CROWNS {1}   -   SACRIFICES {2}   -   FINALS REACHED {3}"), P->MatchesPlayed, P->Crowns, P->Sacrifices, P->Finals) : FText::GetEmpty();
						})
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(420.f)
						[
							SNew(SScrollBox) + SScrollBox::Slot()[HallList.ToSharedRef()]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("Back3", "BACK")).MinWidth(160.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					],
					FMargin(34.f), true)
			]
		];
}

void SAIBMainMenu::RebuildHall()
{
	if (!HallList.IsValid() || !GI())
	{
		return;
	}
	HallList->ClearChildren();
	const UAIBSaveGame* P = GI()->GetProfile();
	if (P->Matches.Num() == 0)
	{
		HallList->AddSlot().AutoHeight().Padding(0.f, 20.f)
		[
			SNew(STextBlock).Text(LOCTEXT("NoMatches", "No trials recorded yet. The island is waiting.")).Font(AIBStyle::Story(18)).ColorAndOpacity(AIBStyle::TextDim())
		];
		return;
	}
	for (const FAIBMatchRecord& Match : P->Matches)
	{
		FString You;
		for (const FAIBMatchPlayerRecord& Player : Match.Players)
		{
			if (Player.bLocal)
			{
				switch (Player.Status)
				{
				case EAIBPlayerStatus::Champion: You = TEXT("You were crowned"); break;
				case EAIBPlayerStatus::Sacrificed: You = TEXT("You saved your people"); break;
				case EAIBPlayerStatus::Active: You = TEXT("You reached the final"); break;
				case EAIBPlayerStatus::Forfeited: You = TEXT("You forfeited"); break;
				default: You = FString::Printf(TEXT("You fell in stage %d"), Player.EliminatedInStage); break;
				}
			}
		}
		const FText Champion = Match.ChampionName.IsEmpty() ? LOCTEXT("Unclaimed", "Island unclaimed")
			: FText::Format(LOCTEXT("ChampionOf", "{0} of {1}"), FText::FromString(Match.ChampionName), AIB::Community(Match.ChampionCommunity).Name);
		HallList->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(14.f, 8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.22f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::AsDateTime(Match.Date, EDateTimeStyle::Short, EDateTimeStyle::Short)).Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(AIB::EndingTitle(Match.Ending)).Font(AIBStyle::Heading(18)).ColorAndOpacity(AIBStyle::Gold())
				]
				+ SHorizontalBox::Slot().FillWidth(0.28f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Champion).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(STextBlock).Text(FText::FromString(You)).Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::Acid())
				]
			]
		];
	}
}

TSharedRef<SWidget> SAIBMainMenu::MakeCredits()
{
	FString Credits;
	FFileHelper::LoadFileToString(Credits, *(AIBStyle::UIRoot() / TEXT("Credits.txt")));
	if (Credits.IsEmpty())
	{
		Credits = TEXT("Alice in Botherland - The Last Island");
	}
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("CreditsTitle", "CREDITS")).Font(AIBStyle::Title(52)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(480.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SNew(STextBlock).Text(FText::FromString(Credits)).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("Back4", "BACK")).MinWidth(160.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					],
					FMargin(34.f), true)
			]
		];
}

#undef LOCTEXT_NAMESPACE
