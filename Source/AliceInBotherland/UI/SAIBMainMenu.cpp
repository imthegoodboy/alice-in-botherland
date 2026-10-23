// Alice in Botherland - main menu: champion profile, rooms, the story, hall of champions

#include "UI/SAIBMainMenu.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBHud.h"
#include "UI/SAIBLoadingScreen.h"
#include "UI/SAIBStoryCards.h"
#include "Game/AIBMenuGameMode.h"
#include "Core/AIBGameInstance.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Audio/AIBNarration.h"
#include "Online/AIBSessionSubsystem.h"
#include "Online/AIBRoomCode.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBMainMenu"

namespace
{
	const FText& MonthName(int32 Month)
	{
		static const FText Names[] =
		{
			LOCTEXT("Jan", "JANUARY"), LOCTEXT("Feb", "FEBRUARY"), LOCTEXT("Mar", "MARCH"), LOCTEXT("Apr", "APRIL"),
			LOCTEXT("May", "MAY"), LOCTEXT("Jun", "JUNE"), LOCTEXT("Jul", "JULY"), LOCTEXT("Aug", "AUGUST"),
			LOCTEXT("Sep", "SEPTEMBER"), LOCTEXT("Oct", "OCTOBER"), LOCTEXT("Nov", "NOVEMBER"), LOCTEXT("Dec", "DECEMBER")
		};
		return Names[FMath::Clamp(Month, 1, 12) - 1];
	}

	/** Keeps letters, digits, spaces, '-' and '_' (the name travels in a URL and shows over heads). */
	FString CleanName(const FString& In)
	{
		FString Out;
		for (const TCHAR C : In)
		{
			if (FChar::IsAlnum(C) || C == TEXT(' ') || C == TEXT('-') || C == TEXT('_'))
			{
				Out.AppendChar(C);
			}
		}
		return Out.TrimStartAndEnd().Left(16);
	}
}

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
		+ SOverlay::Slot()[PageBox(EPage::Profile, MakeProfile())]
		+ SOverlay::Slot()[PageBox(EPage::Join, MakeJoin())]
		+ SOverlay::Slot()
		[
			PageBox(EPage::Settings,
				SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SAIBSettingsPanel).GameInstance(GI()).OnClose_Lambda([this]() { Go(EPage::Home); })
				])
		]
		+ SOverlay::Slot()[PageBox(EPage::Story, MakeStory())]
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

	// -JoinCode=XXXX-XXX (e.g. on a shortcut a friend made) joins that room straight away, once per launch.
	static bool bJoinCodeUsed = false;
	FString CodeArg;
	if (!bJoinCodeUsed && FParse::Value(FCommandLine::Get(), TEXT("JoinCode="), CodeArg) && Sessions())
	{
		bJoinCodeUsed = true;
		// Show the join page, so a code that doesn't work leaves its message on screen.
		Go(EPage::Join);
		RoomCode = CodeArg.ToUpper();
		if (CodeBox.IsValid())
		{
			CodeBox->SetText(FText::FromString(RoomCode));
		}
		JoinWithCode();
		return;
	}

#if !UE_BUILD_SHIPPING
	// Development: -AIBMenuPage=home|join|story|settings|howto|hall opens that page (UI screenshots).
	FString PageArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("AIBMenuPage="), PageArg))
	{
		const TMap<FString, EPage> Pages = { { TEXT("home"), EPage::Home }, { TEXT("join"), EPage::Join }, { TEXT("settings"), EPage::Settings },
			{ TEXT("howto"), EPage::HowTo }, { TEXT("hall"), EPage::Hall }, { TEXT("profile"), EPage::Profile } };
		if (PageArg.ToLower() == TEXT("story"))
		{
			StartStory();
		}
		else if (const EPage* Page = Pages.Find(PageArg.ToLower()))
		{
			Go(*Page);
		}
		return;
	}
#endif

	// First launch: who is this champion?
	UAIBGameInstance* Inst = GI();
	if (Inst && !Inst->GetProfile()->bProfileComplete)
	{
		Go(EPage::Profile);
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
	if (Current == EPage::Story && Page != EPage::Story)
	{
		StopStory();
	}
	Current = Page;
	if (Page == EPage::Join)
	{
		JoinError = FText::GetEmpty();
		if (Sessions())
		{
			Sessions()->FindLobbies(true);
			ShownResults = -1;
		}
		if (CodeBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(CodeBox, EFocusCause::SetDirectly);
		}
	}
	if (Page == EPage::Profile)
	{
		// Start the form from what is saved.
		if (const UAIBGameInstance* Inst = GI())
		{
			const UAIBSaveGame* P = const_cast<UAIBGameInstance*>(Inst)->GetProfile();
			DraftName = P->Settings.PlayerName;
			DraftDay = P->BirthDay > 0 ? P->BirthDay : 1;
			DraftMonth = P->BirthMonth > 0 ? P->BirthMonth : 1;
			DraftYear = P->BirthYear > 0 ? P->BirthYear : 2000;
			bProfileTouched = P->bProfileComplete;
			if (NameBox.IsValid())
			{
				NameBox->SetText(FText::FromString(DraftName));
			}
		}
		if (NameBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(NameBox, EFocusCause::SetDirectly);
		}
	}
	if (Page == EPage::Hall)
	{
		RebuildHall();
	}
}

FReply SAIBMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const bool bFirstProfile = Current == EPage::Profile && GI() && !GI()->GetProfile()->bProfileComplete;
	if ((InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::SpaceBar) && Current != EPage::Home && !bFirstProfile)
	{
		Go(EPage::Home);
		return FReply::Handled();
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
			.FontSize(Sub.IsEmpty() ? 20 : 24)
			.Height(Sub.IsEmpty() ? 46.f : 62.f)
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
	};

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(90.f, 40.f, 0.f, 40.f)
		[
			SNew(SBox).WidthOverride(560.f)
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
					SNew(STextBlock).Text(LOCTEXT("Subtitle", "THE  LAST  ISLAND")).Font(AIBStyle::StoryBold(26)).ColorAndOpacity(AIBStyle::Gold())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 0.f, 20.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Tagline", "Five communities. Three trials. One island. And a choice no champion is ready to make."))
					.Font(AIBStyle::Story(16)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
				]
				// Who is playing.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::CardBrush())
					.Padding(FMargin(16.f, 8.f, 8.f, 8.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(LOCTEXT("PlayingAs", "CHAMPION")).Font(AIBStyle::SubHeading(13)).ColorAndOpacity(AIBStyle::TextDim())
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Heading(24))
								.ColorAndOpacity(AIBStyle::Text())
								.Text_Lambda([this]()
								{
									UAIBGameInstance* Inst = GI();
									if (!Inst)
									{
										return FText::GetEmpty();
									}
									const int32 Age = Inst->GetProfile()->GetAge();
									const FText Name = FText::FromString(Inst->GetSettings().PlayerName.ToUpper());
									return Age >= 0 ? FText::Format(LOCTEXT("NameAge", "{0}   -   AGE {1}"), Name, Age) : Name;
								})
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAIBButton).Text(LOCTEXT("EditProfile", "EDIT")).Kind(EAIBButtonKind::Ghost).FontSize(16).Height(36.f)
							.OnClicked_Lambda([this]() { Go(EPage::Profile); return FReply::Handled(); })
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("Create", "CREATE ROOM"), LOCTEXT("CreateSub", "Get a room code and bring four friends"), EAIBButtonKind::Primary,
						[this]() { if (Sessions()) Sessions()->HostLobby(false, 0); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("JoinRoom", "JOIN ROOM"), LOCTEXT("JoinRoomSub", "Type a friend's room code"), EAIBButtonKind::Secondary,
						[this]() { Go(EPage::Join); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[
					Button(LOCTEXT("PlaySolo", "PLAY VS AI"), LOCTEXT("PlaySoloSub", "You and four AI champions"), EAIBButtonKind::Secondary,
						[this]() { if (Sessions()) Sessions()->HostLobby(true, 4); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)[Button(LOCTEXT("Story", "THE STORY"), FText::GetEmpty(), EAIBButtonKind::Secondary, [this]() { StartStory(); })]
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
			.Text(LOCTEXT("Footer", "v1.1.0   -   rooms work on your local network; internet play needs UDP port 7777 forwarded on the host's router"))
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakeStepper(const FText& Label, TAttribute<FText> Value, TFunction<void(int32)> Step, float Width)
{
	auto Arrow = [Step](const TCHAR* Glyph, int32 Delta)
	{
		return SNew(SAIBButton)
			.Text(FText::FromString(Glyph))
			.Kind(EAIBButtonKind::Secondary)
			.FontSize(22)
			.Height(52.f)
			.OnClicked_Lambda([Step, Delta]() { Step(Delta); return FReply::Handled(); });
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock).Text(Label).Font(AIBStyle::SubHeading(14)).ColorAndOpacity(AIBStyle::TextDim())
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Arrow(TEXT("<"), -1)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(Width).HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(Value).Font(AIBStyle::Heading(26)).ColorAndOpacity(AIBStyle::Text())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[Arrow(TEXT(">"), 1)]
		];
}

FText SAIBMainMenu::ProfileProblem() const
{
	const FString Name = CleanName(DraftName);
	if (Name.Len() < 2)
	{
		return LOCTEXT("NameShort", "Your champion name needs at least 2 letters.");
	}
	const FDateTime Now = FDateTime::Now();
	if (DraftYear > Now.GetYear() || (DraftYear == Now.GetYear() && (DraftMonth > Now.GetMonth() || (DraftMonth == Now.GetMonth() && DraftDay > Now.GetDay()))))
	{
		return LOCTEXT("DobFuture", "That date of birth is in the future.");
	}
	return FText::GetEmpty();
}

void SAIBMainMenu::SaveProfile()
{
	UAIBGameInstance* Inst = GI();
	if (!Inst || !ProfileProblem().IsEmpty())
	{
		bProfileTouched = true;
		AIBStyle::PlayUISound(TEXT("SFX_Wrong"));
		return;
	}
	UAIBSaveGame* P = Inst->GetProfile();
	P->BirthDay = DraftDay;
	P->BirthMonth = DraftMonth;
	P->BirthYear = DraftYear;
	P->bProfileComplete = true;
	FAIBSettings Settings = Inst->GetSettings();
	Settings.PlayerName = CleanName(DraftName);
	Inst->SetSettings(Settings);
	Inst->SaveProfile();
	Go(EPage::Home);
}

TSharedRef<SWidget> SAIBMainMenu::MakeProfile()
{
	auto ClampDay = [this]() { DraftDay = FMath::Clamp(DraftDay, 1, FDateTime::DaysInMonth(DraftYear, DraftMonth)); };
	const int32 ThisYear = FDateTime::Now().GetYear();
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(760.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("ProfileTitle", "WHO ARE YOU, CHAMPION?")).Font(AIBStyle::Title(54)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ProfileSub", "Your community has chosen you. Tell the island your name."))
						.Font(AIBStyle::Story(16)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock).Text(LOCTEXT("NameLabel", "CHAMPION NAME")).Font(AIBStyle::SubHeading(14)).ColorAndOpacity(AIBStyle::TextDim())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(NameBox, SEditableTextBox)
						.Font(AIBStyle::Heading(28))
						.HintText(LOCTEXT("NameHint", "Type your name"))
						.Text(FText::FromString(DraftName))
						.OnTextChanged_Lambda([this](const FText& Text) { DraftName = Text.ToString().Left(16); })
						.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit) { DraftName = Text.ToString().Left(16); if (Commit == ETextCommit::OnEnter) SaveProfile(); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("NameRules", "2 to 16 letters or numbers. The other champions see this name.")).Font(AIBStyle::Body(13)).ColorAndOpacity(AIBStyle::TextFaint())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 8.f)
					[
						AIBUI::Header(LOCTEXT("DobHeader", "DATE OF BIRTH"))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 18.f, 0.f)
						[
							MakeStepper(LOCTEXT("Day", "DAY"), TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(DraftDay); }),
								[this](int32 Delta)
								{
									const int32 Days = FDateTime::DaysInMonth(DraftYear, DraftMonth);
									DraftDay = (DraftDay - 1 + Delta + Days) % Days + 1;
									bProfileTouched = true;
								}, 60.f)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 18.f, 0.f)
						[
							MakeStepper(LOCTEXT("Month", "MONTH"), TAttribute<FText>::CreateLambda([this]() { return MonthName(DraftMonth); }),
								[this, ClampDay](int32 Delta)
								{
									DraftMonth = (DraftMonth - 1 + Delta + 12) % 12 + 1;
									ClampDay();
									bProfileTouched = true;
								}, 170.f)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							MakeStepper(LOCTEXT("Year", "YEAR"), TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::FromInt(DraftYear)); }),
								[this, ClampDay, ThisYear](int32 Delta)
								{
									DraftYear = FMath::Clamp(DraftYear + Delta, 1920, ThisYear);
									ClampDay();
									bProfileTouched = true;
								}, 90.f)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(AIBStyle::BodyMedium(16))
						.Text_Lambda([this]()
						{
							const FText Problem = ProfileProblem();
							if (bProfileTouched && !Problem.IsEmpty())
							{
								return Problem;
							}
							const FDateTime Now = FDateTime::Now();
							int32 Age = Now.GetYear() - DraftYear;
							if (Now.GetMonth() < DraftMonth || (Now.GetMonth() == DraftMonth && Now.GetDay() < DraftDay))
							{
								--Age;
							}
							return FText::Format(LOCTEXT("AgeLine", "Age {0}. Your date of birth stays on this PC."), FMath::Max(0, Age));
						})
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(bProfileTouched && !ProfileProblem().IsEmpty() ? AIBStyle::Blood() : AIBStyle::TextDim()); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SAIBButton)
							.Text(LOCTEXT("ProfileBack", "BACK"))
							.Kind(EAIBButtonKind::Ghost)
							.Visibility_Lambda([this]() { return GI() && GI()->GetProfile()->bProfileComplete ? EVisibility::Visible : EVisibility::Hidden; })
							.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SAIBButton)
							.Text(LOCTEXT("ProfileContinue", "CONTINUE"))
							.Kind(EAIBButtonKind::Primary)
							.MinWidth(220.f)
							.FontSize(24)
							.Height(58.f)
							.OnClicked_Lambda([this]() { SaveProfile(); return FReply::Handled(); })
						]
					],
					FMargin(36.f), true)
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Join
// ---------------------------------------------------------------------------------------------------------------------

void SAIBMainMenu::JoinWithCode()
{
	FString Address;
	if (!AIBRoomCode::ToAddress(RoomCode, Address))
	{
		JoinError = LOCTEXT("BadCode", "That doesn't look like a room code. It has 7 letters and numbers, like 6C2M-9QZ.");
		AIBStyle::PlayUISound(TEXT("SFX_Wrong"));
		return;
	}
	JoinError = FText::GetEmpty();
	if (Sessions())
	{
		Sessions()->JoinByAddress(Address);
	}
}

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
						SNew(STextBlock).Text(LOCTEXT("JoinTitle", "JOIN A ROOM")).Font(AIBStyle::Title(54)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("JoinSub", "Ask the host for their room code (it is shown in their room), then type it here."))
						.Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
						[
							SAssignNew(CodeBox, SEditableTextBox)
							.Font(AIBStyle::Title(46))
							.HintText(LOCTEXT("CodeHint", "ROOM CODE"))
							.OnTextChanged_Lambda([this](const FText& Text) { RoomCode = Text.ToString().ToUpper(); JoinError = FText::GetEmpty(); })
							.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit)
							{
								RoomCode = Text.ToString().ToUpper();
								if (Commit == ETextCommit::OnEnter)
								{
									JoinWithCode();
								}
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAIBButton).Kind(EAIBButtonKind::Primary).Text(LOCTEXT("JoinCode", "JOIN")).FontSize(26).MinWidth(160.f).Height(64.f)
							.IsEnabled_Lambda([this]() { return !RoomCode.TrimStartAndEnd().IsEmpty(); })
							.OnClicked_Lambda([this]() { JoinWithCode(); return FReply::Handled(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(AIBStyle::BodyMedium(15))
						.AutoWrapText(true)
						.Text_Lambda([this]() { return !JoinError.IsEmpty() ? JoinError : (Sessions() ? Sessions()->GetStatus() : FText::GetEmpty()); })
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(!JoinError.IsEmpty() ? AIBStyle::Blood() : AIBStyle::Gold()); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[AIBUI::Header(LOCTEXT("NearbyRooms", "ROOMS ON YOUR NETWORK"))]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SAIBButton).Text(LOCTEXT("Refresh", "REFRESH")).Kind(EAIBButtonKind::Ghost).FontSize(15).Height(32.f)
							.IsEnabled_Lambda([this]() { return Sessions() && !Sessions()->IsSearching(); })
							.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->FindLobbies(true); ShownResults = -1; return FReply::Handled(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).MaxDesiredHeight(220.f)
						[
							SNew(SScrollBox) + SScrollBox::Slot()[LobbyList.ToSharedRef()]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("Direct", "OR CONNECT BY IP ADDRESS"))]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(SEditableTextBox)
							.Font(AIBStyle::Body(17))
							.HintText(LOCTEXT("AddrHint", "e.g. 192.168.1.20 or 203.0.113.5:7777"))
							.OnTextChanged_Lambda([this](const FText& Text) { DirectAddress = Text.ToString(); })
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SAIBButton).Text(LOCTEXT("Connect", "CONNECT")).FontSize(17).Height(42.f)
							.IsEnabled_Lambda([this]() { return !DirectAddress.TrimStartAndEnd().IsEmpty(); })
							.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->JoinByAddress(DirectAddress); return FReply::Handled(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f).HAlign(HAlign_Right)
					[
						SNew(SAIBButton).Text(LOCTEXT("Back", "BACK")).MinWidth(160.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					],
					FMargin(34.f), true)
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
		LobbyList->AddSlot().AutoHeight().Padding(0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(Sessions()->IsSearching() ? LOCTEXT("SearchingList", "Looking for rooms nearby...") : LOCTEXT("NoLobbies", "No rooms found on this network. Use the room code instead."))
			.Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim())
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
					SNew(STextBlock).Text(FText::Format(LOCTEXT("LobbyOf", "{0}'s room"), FText::FromString(Info.HostName))).Font(AIBStyle::Heading(20)).ColorAndOpacity(AIBStyle::Text())
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
// The story (replay of the opening, narrated locally)
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::MakeStory()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAIBStoryCards)
			.Fullscreen(true)
			.Sequence(EAIBNarration::Story)
			.Elapsed_Lambda([this]() { return float(FPlatformTime::Seconds() - StoryStart); })
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 40.f, 34.f)
		[
			SNew(SAIBButton).Text(LOCTEXT("SkipStory", "SKIP  [SPACE]")).Kind(EAIBButtonKind::Ghost).FontSize(17)
			.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
		];
}

void SAIBMainMenu::StartStory()
{
	Current = EPage::Story;
	StoryStart = FPlatformTime::Seconds();
	StoryBeat = INDEX_NONE;
	if (UAIBGameInstance* Inst = GI())
	{
		Inst->GetProfile()->bSeenPrologue = true;
		Inst->SaveProfile();
	}
	if (Owner.IsValid() && Owner->GetGameInstance())
	{
		if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			TArray<FName> Lines;
			for (const AIBNarration::FBeat& Beat : AIBNarration::Get(EAIBNarration::Story).Beats)
			{
				Lines.Add(Beat.Line);
				AIBStyle::StoryImage(Beat.Image);
			}
			Audio->PreloadVoices(Lines);
			Audio->PlayMusic(TEXT("MUS_Arrival"));
		}
	}
	// Loading may have taken a moment: start the clock now.
	StoryStart = FPlatformTime::Seconds();
}

void SAIBMainMenu::StopStory()
{
	StoryBeat = INDEX_NONE;
	if (Owner.IsValid() && Owner->GetGameInstance())
	{
		if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->StopNarration();
			Audio->PlayMusic(TEXT("MUS_Menu"));
		}
	}
}

void SAIBMainMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (Current == EPage::Story)
	{
		const float Elapsed = float(FPlatformTime::Seconds() - StoryStart);
		if (Elapsed > AIBNarration::Length(EAIBNarration::Story))
		{
			Go(EPage::Home);
			return;
		}
		const int32 Beat = AIBNarration::BeatAt(EAIBNarration::Story, Elapsed);
		if (Beat != StoryBeat && Beat != INDEX_NONE)
		{
			StoryBeat = Beat;
			if (Owner.IsValid() && Owner->GetGameInstance())
			{
				if (UAIBAudioSubsystem* Audio = Owner->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
				{
					const float Offset = Elapsed - AIBNarration::Get(EAIBNarration::Story).Starts[Beat];
					Audio->PlayVoice(AIBNarration::Get(EAIBNarration::Story).Beats[Beat].Line, Offset > 0.15f ? Offset : 0.f);
				}
			}
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
		"ON FOOT  -  WASD move, Mouse look, Shift sprint, Space jump, E read a marker stone\n"
		"DRIVING  -  W/S throttle and brake, A/D steer, Space handbrake, R reset, hold E on the switch\n"
		"ALWAYS  -  hold V to talk (voice chat), Enter text chat, Tab scoreboard, Esc menu.  Gamepads are supported too.");
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
						.Text(LOCTEXT("HowToIntro", "Five champions land on the last island. After some time to explore (look for the marker stones), the Guardian's three trials begin. Only two will reach the end - and then one terrible choice decides who wins and which community survives."))
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
