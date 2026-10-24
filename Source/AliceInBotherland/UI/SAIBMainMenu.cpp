// Alice in Botherland - main menu: champion profile, modes, rooms, the story, hall of champions

#include "UI/SAIBMainMenu.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBGameUI.h"
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
#include "Widgets/Input/SEditableText.h"
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

	constexpr int32 FirstYear = 1920;

	/** A glass input field with an accent underline (name, room code, address). */
	TSharedRef<SWidget> InputFrame(TSharedRef<SWidget> Field, const FMargin& Padding = FMargin(22.f, 10.f))
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)).Padding(Padding)[Field]
			]
			+ SOverlay::Slot()
			[
				SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f)).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.14f))
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBox).Visibility(EVisibility::HitTestInvisible).HeightOverride(3.f)
				[
					SNew(SAIBGradient).Vertical(false).Colors({ AIBStyle::Ember(), AIBStyle::Gold(), FLinearColor(1.f, 0.74f, 0.32f, 0.f) })
				]
			];
	}

	TSharedRef<SWidget> FieldLabel(const TAttribute<FText>& Text)
	{
		return SNew(STextBlock).Text(Text).Font(AIBStyle::Spaced(AIBStyle::SubHeading(15), 220)).ColorAndOpacity(AIBStyle::TextDim());
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

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[PageBox(EPage::Home, MakeHome())]
		+ SOverlay::Slot()[PageBox(EPage::Profile, MakeProfile())]
		+ SOverlay::Slot()[PageBox(EPage::Join, MakeJoin())]
		+ SOverlay::Slot()
		[
			PageBox(EPage::Settings,
				SNew(SOverlay)
				+ SOverlay::Slot()[AIBUI::ArtBackdrop(TEXT("I_Welcome"), 0.75f)]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SAIBReveal).Offset(FVector2D(0.f, 30.f))
					[
						SNew(SAIBSettingsPanel).GameInstance(GI()).OnClose_Lambda([this]() { Go(EPage::Home); })
					]
				])
		]
		+ SOverlay::Slot()[PageBox(EPage::Story, MakeStory())]
		+ SOverlay::Slot()[PageBox(EPage::HowTo, MakeHowTo())]
		+ SOverlay::Slot()[PageBox(EPage::Hall, MakeHall())]
		+ SOverlay::Slot()[PageBox(EPage::Credits, MakeCredits())]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 8.f, 10.f, 0.f)[SNew(SAIBFpsCounter)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 30.f, 0.f, 0.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return FPlatformTime::Seconds() - MenuMessageTime < 8.0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				AIBUI::Frame(SNew(STextBlock).Text_Lambda([this]() { return MenuMessage; }).Font(AIBStyle::BodyMedium(18)).ColorAndOpacity(AIBStyle::Gold()), FMargin(22.f, 12.f))
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
	// Development: -AIBMenuPage=home|join|story|settings|howto|hall|credits|profile opens that page (UI screenshots).
	FString PageArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("AIBMenuPage="), PageArg))
	{
		const TMap<FString, EPage> Pages = { { TEXT("home"), EPage::Home }, { TEXT("join"), EPage::Join }, { TEXT("settings"), EPage::Settings },
			{ TEXT("howto"), EPage::HowTo }, { TEXT("hall"), EPage::Hall }, { TEXT("credits"), EPage::Credits }, { TEXT("profile"), EPage::Profile } };
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
		bJoining = false;
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

void SAIBMainMenu::StartMode(int32 Which)
{
	switch (Which)
	{
	case ModeSolo:
		if (Sessions())
		{
			Sessions()->HostLobby(true, 4);
		}
		break;
	case ModeCreate:
		if (Sessions())
		{
			Sessions()->HostLobby(false, 0);
		}
		break;
	default:
		Go(EPage::Join);
		break;
	}
}

FReply SAIBMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Current == EPage::Home)
	{
		if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			StartMode(Mode);
			return FReply::Handled();
		}
		if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
		{
			Mode = (Mode + ModeCount - 1) % ModeCount;
			AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
			return FReply::Handled();
		}
		if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
		{
			Mode = (Mode + 1) % ModeCount;
			AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
	const bool bFirstProfile = Current == EPage::Profile && GI() && !GI()->GetProfile()->bProfileComplete;
	if ((Key == EKeys::Escape || (Key == EKeys::SpaceBar && Current == EPage::Story) || Key == EKeys::Gamepad_FaceButton_Right) && !bFirstProfile)
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
	struct FModeInfo { TCHAR Icon; FText Title; FText Sub; };
	static const FModeInfo Modes[ModeCount] =
	{
		{ AIBIcon::Robot, LOCTEXT("ModeSolo", "PLAY VS AI"), LOCTEXT("ModeSoloSub", "You and four AI champions") },
		{ AIBIcon::Groups, LOCTEXT("ModeCreate", "CREATE ROOM"), LOCTEXT("ModeCreateSub", "Get a room code and bring four friends") },
		{ AIBIcon::Login, LOCTEXT("ModeJoin", "JOIN ROOM"), LOCTEXT("ModeJoinSub", "Enter a friend's room code") }
	};

	TSharedRef<SVerticalBox> ModeList = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(SAIBReveal).Delay(0.1f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(SBox).WidthOverride(26.f).HeightOverride(3.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Ember())]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("SelectMode", "SELECT MODE")).Font(AIBStyle::Spaced(AIBStyle::SubHeading(17), 260)).ColorAndOpacity(AIBStyle::Gold())
				]
			]
		];
	for (int32 Index = 0; Index < ModeCount; ++Index)
	{
		ModeList->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SAIBReveal).Delay(0.15f + 0.07f * Index)
			[
				SNew(SAIBSlantButton)
				.Icon(Modes[Index].Icon)
				.Text(Modes[Index].Title)
				.SubText(Modes[Index].Sub)
				.FontSize(28)
				.Height(84.f)
				.MinWidth(540.f)
				.IsSelected_Lambda([this, Index]() { return Mode == Index; })
				.OnClicked_Lambda([this, Index]()
				{
					// First click picks the mode, a second click on it starts.
					if (Mode == Index)
					{
						StartMode(Index);
					}
					Mode = Index;
					return FReply::Handled();
				})
			]
		];
	}

	auto NavButton = [this](TCHAR Glyph, const FText& Label, EPage Page)
	{
		return SNew(SAIBIconButton).Icon(Glyph).Label(Label).OnClicked_Lambda([this, Page]() { Go(Page); return FReply::Handled(); });
	};

	return SNew(SOverlay)
		// Shade the left, top and bottom so everything reads over the live fleet.
		+ SOverlay::Slot()
		[
			SNew(SAIBGradient).Vertical(false).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.86f), FLinearColor(0.f, 0.f, 0.f, 0.5f), FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.f) })
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(190.f)[SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.6f), FLinearColor(0.f, 0.f, 0.f, 0.f) })]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(340.f)[SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.78f) })]
		]
		// Logo.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(84.f, 52.f, 0.f, 0.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(0.f, -30.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Title1", "ALICE IN")).Font(AIBStyle::Spaced(AIBStyle::Title(50), 320)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, -26.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Title2", "BOTHERLAND")).Font(AIBStyle::Spaced(AIBStyle::Title(132), 10)).ColorAndOpacity(FLinearColor(0.98f, 0.96f, 0.92f))
					.ShadowOffset(FVector2D(3.f, 4.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, -14.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
					[
						SNew(SBox).WidthOverride(64.f).HeightOverride(3.f)[SNew(SAIBGradient).Vertical(false).Colors({ AIBStyle::Ember(), AIBStyle::Gold() })]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("Subtitle", "THE LAST ISLAND")).Font(AIBStyle::Spaced(AIBStyle::StoryBold(22), 300)).ColorAndOpacity(AIBStyle::Gold())
					]
				]
			]
		]
		// Champion and navigation (top right).
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 54.f, 72.f, 0.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(0.f, -30.f)).Delay(0.1f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 26.f, 0.f)
				[
					SNew(SAIBSlantButton)
					.Icon(AIBIcon::Person)
					.Text_Lambda([this]() { return FText::FromString(GI() ? GI()->GetSettings().PlayerName.ToUpper() : FString()); })
					.SubText_Lambda([this]()
					{
						const int32 Age = GI() ? GI()->GetProfile()->GetAge() : -1;
						return Age >= 0 ? FText::Format(LOCTEXT("ChipAge", "AGE {0}  -  EDIT PROFILE"), Age) : LOCTEXT("ChipEdit", "EDIT PROFILE");
					})
					.FontSize(21)
					.Height(56.f)
					.MinWidth(250.f)
					.OnClicked_Lambda([this]() { Go(EPage::Profile); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)[NavButton(AIBIcon::Book, LOCTEXT("NavRules", "RULES"), EPage::HowTo)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)[NavButton(AIBIcon::Trophy, LOCTEXT("NavHall", "HALL"), EPage::Hall)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)[NavButton(AIBIcon::Settings, LOCTEXT("NavSettings", "SETTINGS"), EPage::Settings)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)[NavButton(AIBIcon::Info, LOCTEXT("NavCredits", "CREDITS"), EPage::Credits)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
				[
					SNew(SAIBIconButton)
					.Icon(AIBIcon::Power)
					.Label(LOCTEXT("NavQuit", "QUIT"))
					.OnClicked_Lambda([this]()
					{
						if (Owner.IsValid())
						{
							UKismetSystemLibrary::QuitGame(Owner.Get(), Owner.Get(), EQuitPreference::Quit, false);
						}
						return FReply::Handled();
					})
				]
			]
		]
		// Modes (left).
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(84.f, 150.f, 0.f, 0.f)
		[
			ModeList
		]
		// START (bottom left).
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(84.f, 0.f, 0.f, 64.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(-60.f, 0.f)).Delay(0.35f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBSlantButton)
					.Kind(EAIBButtonKind::Primary)
					.Icon(AIBIcon::Play)
					.Text(LOCTEXT("Start", "START"))
					.SubText_Lambda([this]() { return Modes[FMath::Clamp(Mode, 0, ModeCount - 1)].Title; })
					.FontSize(44)
					.Height(108.f)
					.MinWidth(540.f)
					.Shine(true)
					.OnClicked_Lambda([this]() { StartMode(Mode); return FReply::Handled(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("StartHint", "ENTER  start      W / S  change mode"))
					.Font(AIBStyle::Spaced(AIBStyle::SubHeading(14), 160))
					.ColorAndOpacity(AIBStyle::TextFaint())
				]
			]
		]
		// The story (bottom right).
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 72.f, 64.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(60.f, 0.f)).Delay(0.45f)
			[
				SNew(SAIBFeatureCard)
				.Image(TEXT("S3_Island"))
				.Kicker(LOCTEXT("StoryKicker", "HOW IT BEGAN"))
				.Title(LOCTEXT("StoryTitle", "THE STORY"))
				.Body(LOCTEXT("StoryBody", "Earth fell. Five communities took to the sea. Then they found the island."))
				.Icon(AIBIcon::Play)
				.OnClicked_Lambda([this]() { StartStory(); return FReply::Handled(); })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 18.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::Spaced(AIBStyle::SubHeading(12), 140))
			.ColorAndOpacity(AIBStyle::TextFaint())
			.Text(LOCTEXT("Footer", "V1.2  -  ROOMS WORK ON YOUR LOCAL NETWORK; FOR INTERNET PLAY THE HOST FORWARDS UDP PORT 7777"))
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------------------------------------------------

int32 SAIBMainMenu::DraftAge() const
{
	const FDateTime Now = FDateTime::Now();
	int32 Age = Now.GetYear() - DraftYear;
	if (Now.GetMonth() < DraftMonth || (Now.GetMonth() == DraftMonth && Now.GetDay() < DraftDay))
	{
		--Age;
	}
	return FMath::Max(0, Age);
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
	const int32 ThisYear = FDateTime::Now().GetYear();
	auto ClampDay = [this]() { DraftDay = FMath::Clamp(DraftDay, 1, FDateTime::DaysInMonth(DraftYear, DraftMonth)); };

	TSharedRef<SWidget> Wheels = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[FieldLabel(LOCTEXT("Day", "DAY"))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SAIBWheel)
				.Width(110.f)
				.NumItems_Lambda([this]() { return FDateTime::DaysInMonth(DraftYear, DraftMonth); })
				.Selected_Lambda([this]() { return DraftDay - 1; })
				.ItemText([](int32 Index) { return FText::AsNumber(Index + 1); })
				.OnSelect_Lambda([this](int32 Index) { DraftDay = Index + 1; bProfileTouched = true; })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[FieldLabel(LOCTEXT("Month", "MONTH"))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SAIBWheel)
				.Width(240.f)
				.NumItems(12)
				.Selected_Lambda([this]() { return DraftMonth - 1; })
				.ItemText([](int32 Index) { return MonthName(Index + 1); })
				.OnSelect_Lambda([this, ClampDay](int32 Index) { DraftMonth = Index + 1; ClampDay(); bProfileTouched = true; })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[FieldLabel(LOCTEXT("Year", "YEAR"))]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SAIBWheel)
				.Width(140.f)
				.Wrap(false)
				.NumItems(ThisYear - FirstYear + 1)
				.Selected_Lambda([this]() { return DraftYear - FirstYear; })
				.ItemText([](int32 Index) { return FText::FromString(FString::FromInt(FirstYear + Index)); })
				.OnSelect_Lambda([this, ClampDay](int32 Index) { DraftYear = FirstYear + Index; ClampDay(); bProfileTouched = true; })
			]
		]
		// Age badge.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(34.f, 30.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(AIBStyle::Title(110)).ColorAndOpacity(AIBStyle::Gold())
				.Text_Lambda([this]() { return FText::AsNumber(DraftAge()); })
				.ShadowOffset(FVector2D(2.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, -18.f, 0.f, 0.f)
			[
				FieldLabel(LOCTEXT("YearsOld", "YEARS OLD"))
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()[AIBUI::ArtBackdrop(TEXT("L_Loading"), 0.93f)]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(110.f, 70.f, 0.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(860.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBReveal)[AIBUI::PageTitle(LOCTEXT("ProfileKicker", "NEW CHAMPION"), LOCTEXT("ProfileTitle", "WHO ARE YOU, CHAMPION?"), 68)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
				[
					SNew(SAIBReveal).Delay(0.05f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ProfileSub", "Your community has chosen you to stand before the Guardian. Tell the island your name."))
						.Font(AIBStyle::Story(17)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(SAIBReveal).Delay(0.1f)
					[
						SNew(SBox).WidthOverride(620.f).HAlign(HAlign_Left)
						[
							SNew(SBox).WidthOverride(620.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.f)[FieldLabel(LOCTEXT("NameLabel", "CHAMPION NAME"))]
								+ SHorizontalBox::Slot().AutoWidth()[FieldLabel(TAttribute<FText>::CreateLambda([this]() { return FText::Format(LOCTEXT("NameCount", "{0} / 16"), DraftName.Len()); }))]
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
				[
					SNew(SAIBReveal).Delay(0.12f)
					[
						SNew(SBox)
						.WidthOverride(620.f)
						[
							InputFrame(
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
								[
									AIBUI::Icon(AIBIcon::Person, 32, AIBStyle::Gold())
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[
									SAssignNew(NameBox, SEditableText)
									.Font(AIBStyle::Spaced(AIBStyle::Heading(34), 40))
									.ColorAndOpacity(FLinearColor(0.98f, 0.96f, 0.92f))
									.HintText(LOCTEXT("NameHint", "TYPE YOUR NAME"))
									.Text(FText::FromString(DraftName))
									.OnTextChanged_Lambda([this](const FText& Text) { DraftName = Text.ToString().Left(16); })
									.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit)
									{
										DraftName = Text.ToString().Left(16);
										if (Commit == ETextCommit::OnEnter)
										{
											SaveProfile();
										}
									})
								])
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 18.f)
				[
					SNew(STextBlock).Text(LOCTEXT("NameRules", "2 to 16 letters or numbers. The other champions see this name.")).Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextFaint())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(SAIBReveal).Delay(0.18f)[FieldLabel(LOCTEXT("DobHeader", "DATE OF BIRTH  -  SCROLL OR DRAG TO PICK"))]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBReveal).Delay(0.22f)[Wheels]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::BodyMedium(16))
					.Text_Lambda([this]()
					{
						const FText Problem = ProfileProblem();
						return bProfileTouched && !Problem.IsEmpty() ? Problem : LOCTEXT("DobPrivate", "Your date of birth stays on this PC.");
					})
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(bProfileTouched && !ProfileProblem().IsEmpty() ? AIBStyle::Blood() : AIBStyle::TextDim()); })
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(110.f, 0.f, 0.f, 64.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(-60.f, 0.f)).Delay(0.3f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SAIBSlantButton)
					.Kind(EAIBButtonKind::Primary)
					.Icon(AIBIcon::Check)
					.Text(LOCTEXT("ProfileContinue", "CONTINUE"))
					.SubText(LOCTEXT("ProfileContinueSub", "Save your champion"))
					.FontSize(38)
					.Height(100.f)
					.MinWidth(480.f)
					.Shine(true)
					.OnClicked_Lambda([this]() { SaveProfile(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return GI() && GI()->GetProfile()->bProfileComplete ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SAIBSlantButton)
						.Kind(EAIBButtonKind::Ghost)
						.Icon(AIBIcon::Back)
						.Text(LOCTEXT("ProfileBack", "BACK"))
						.FontSize(20)
						.Height(52.f)
						.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); })
					]
				]
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Shared page layout
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBMainMenu::BackButton()
{
	return SNew(SAIBSlantButton)
		.Kind(EAIBButtonKind::Ghost)
		.Icon(AIBIcon::Back)
		.Text(LOCTEXT("BackBtn", "BACK"))
		.SubText(LOCTEXT("BackKey", "ESC"))
		.FontSize(20)
		.Height(56.f)
		.OnClicked_Lambda([this]() { Go(EPage::Home); return FReply::Handled(); });
}

TSharedRef<SWidget> SAIBMainMenu::SubPage(FName Art, const FText& Kicker, const FText& Title, TSharedRef<SWidget> Body, float Width)
{
	return SNew(SOverlay)
		+ SOverlay::Slot()[AIBUI::ArtBackdrop(Art, 0.9f)]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(84.f, 48.f, 0.f, 0.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(0.f, -20.f))[BackButton()]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(110.f, 130.f, 0.f, 60.f)
		[
			SNew(SBox)
			.WidthOverride(Width)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
				[
					SNew(SAIBReveal)[AIBUI::PageTitle(Kicker, Title, 76)]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBReveal).Delay(0.1f).Offset(FVector2D(0.f, 30.f))
					[
						AIBUI::Frame(Body, FMargin(30.f, 26.f), FLinearColor(1.f, 0.74f, 0.32f, 0.9f), 0.82f)
					]
				]
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
		bJoining = true;
		Sessions()->JoinByAddress(Address);
	}
}

TSharedRef<SWidget> SAIBMainMenu::MakeJoin()
{
	SAssignNew(LobbyList, SVerticalBox);
	TSharedRef<SWidget> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			FieldLabel(LOCTEXT("CodeLabel", "ROOM CODE  -  ASK THE HOST, IT IS SHOWN IN THEIR ROOM"))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
			[
				InputFrame(
					SAssignNew(CodeBox, SEditableText)
					.Font(AIBStyle::Spaced(AIBStyle::Title(58), 240))
					.ColorAndOpacity(AIBStyle::Gold())
					.HintText(LOCTEXT("CodeHint", "XXXX-XXX"))
					.OnTextChanged_Lambda([this](const FText& Text) { RoomCode = Text.ToString().ToUpper(); JoinError = FText::GetEmpty(); })
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit)
					{
						RoomCode = Text.ToString().ToUpper();
						if (Commit == ETextCommit::OnEnter)
						{
							JoinWithCode();
						}
					}),
					FMargin(24.f, 6.f))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SAIBSlantButton)
				.Kind(EAIBButtonKind::Primary)
				.Icon(AIBIcon::Login)
				.Text(LOCTEXT("JoinCode", "JOIN"))
				.FontSize(32)
				.Height(84.f)
				.MinWidth(220.f)
				.Shine(true)
				.IsEnabled_Lambda([this]() { return !RoomCode.TrimStartAndEnd().IsEmpty(); })
				.OnClicked_Lambda([this]() { JoinWithCode(); return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::BodyMedium(16))
			.AutoWrapText(true)
			.Text_Lambda([this]() { return !JoinError.IsEmpty() ? JoinError : (bJoining ? LOCTEXT("Joining", "Connecting to the room...") : FText::GetEmpty()); })
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(!JoinError.IsEmpty() ? AIBStyle::Blood() : AIBStyle::Gold()); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[AIBUI::Header(LOCTEXT("NearbyRooms", "ROOMS ON YOUR NETWORK"))]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SAIBSlantButton).Kind(EAIBButtonKind::Ghost).Icon(AIBIcon::Refresh).Text(LOCTEXT("Refresh", "REFRESH")).FontSize(15).Height(36.f)
				.IsEnabled_Lambda([this]() { return Sessions() && !Sessions()->IsSearching(); })
				.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->FindLobbies(true); ShownResults = -1; return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).MaxDesiredHeight(200.f)
			[
				SNew(SScrollBox).ScrollBarThickness(FVector2D(5.f, 5.f)) + SScrollBox::Slot()[LobbyList.ToSharedRef()]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 8.f)[AIBUI::Header(LOCTEXT("Direct", "OR CONNECT BY IP ADDRESS"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
			[
				InputFrame(
					SNew(SEditableText)
					.Font(AIBStyle::Body(19))
					.ColorAndOpacity(AIBStyle::Text())
					.HintText(LOCTEXT("AddrHint", "e.g. 192.168.1.20 or 203.0.113.5:7777"))
					.OnTextChanged_Lambda([this](const FText& Text) { DirectAddress = Text.ToString(); }),
					FMargin(18.f, 10.f))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SAIBSlantButton).Icon(AIBIcon::Lan).Text(LOCTEXT("Connect", "CONNECT")).FontSize(18).Height(48.f)
				.IsEnabled_Lambda([this]() { return !DirectAddress.TrimStartAndEnd().IsEmpty(); })
				.OnClicked_Lambda([this]() { if (Sessions()) Sessions()->JoinByAddress(DirectAddress); return FReply::Handled(); })
			]
		];
	return SubPage(TEXT("S2_Search"), LOCTEXT("JoinKicker", "MULTIPLAYER"), LOCTEXT("JoinTitle", "JOIN A ROOM"), Body, 860.f);
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
			SNew(SAIBSlantButton)
			.Icon(AIBIcon::Groups)
			.Text(FText::Format(LOCTEXT("LobbyOf", "{0}'S ROOM"), FText::FromString(Info.HostName.ToUpper())))
			.SubText(FText::Format(LOCTEXT("Seats", "{0} / {1} champions  -  {2} ms  -  click to join"), Info.Players, Info.MaxPlayers, Info.Ping))
			.FontSize(20)
			.Height(60.f)
			.OnClicked_Lambda([this, Index]() { if (Sessions()) Sessions()->JoinLobby(Index); return FReply::Handled(); })
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
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 48.f, 40.f)
		[
			SNew(SAIBSlantButton).Kind(EAIBButtonKind::Ghost).Icon(AIBIcon::Chevron).Text(LOCTEXT("SkipStory", "SKIP")).SubText(LOCTEXT("SkipKey", "SPACE")).FontSize(20).Height(52.f)
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
		"DRIVING  -  W/S throttle and brake, A/D steer, Space handbrake, R reset, drive into the Guardian's ring to claim it\n"
		"ALWAYS  -  hold V to talk, M mic on/off, Enter text chat, Tab scoreboard, Esc menu (mute players there).  Gamepads are supported too.");
	TSharedRef<SWidget> Body = SNew(SBox)
		.MaxDesiredHeight(600.f)
		[
			SNew(SScrollBox)
			.ScrollBarThickness(FVector2D(5.f, 5.f))
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 12.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("HowToIntro", "Five champions land on the last island. After some time to explore (look for the marker stones), the Guardian's three trials begin. Only two will reach the end - and then one terrible choice decides who wins and which community survives."))
					.Font(AIBStyle::Body(17)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 12.f, 0.f)[SAIBHud::MakeRules(false)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 12.f, 6.f)[AIBUI::Header(LOCTEXT("ControlsHeader", "CONTROLS"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(STextBlock).Text(Controls).Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
				]
			]
		];
	return SubPage(TEXT("T1_Numbers"), LOCTEXT("HowToKicker", "THE GUARDIAN'S LAW"), LOCTEXT("HowToTitle", "HOW TO PLAY"), Body, 960.f);
}

TSharedRef<SWidget> SAIBMainMenu::MakeHall()
{
	SAssignNew(HallList, SVerticalBox);
	TSharedRef<SWidget> Body = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::Spaced(AIBStyle::Heading(20), 80))
			.ColorAndOpacity(AIBStyle::Text())
			.Text_Lambda([this]()
			{
				const UAIBSaveGame* P = GI() ? GI()->GetProfile() : nullptr;
				return P ? FText::Format(LOCTEXT("Stats", "MATCHES {0}   -   CROWNS {1}   -   SACRIFICES {2}   -   FINALS {3}"), P->MatchesPlayed, P->Crowns, P->Sacrifices, P->Finals) : FText::GetEmpty();
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).MaxDesiredHeight(460.f)
			[
				SNew(SScrollBox).ScrollBarThickness(FVector2D(5.f, 5.f)) + SScrollBox::Slot()[HallList.ToSharedRef()]
			]
		];
	return SubPage(TEXT("S4_Guardian"), LOCTEXT("HallKicker", "YOUR LEGEND"), LOCTEXT("HallTitle", "HALL OF CHAMPIONS"), Body, 960.f);
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
				default: You = FString::Printf(TEXT("You fell in trial %d"), Player.EliminatedInStage); break;
				}
			}
		}
		const FText Champion = Match.ChampionName.IsEmpty() ? LOCTEXT("Unclaimed", "Island unclaimed")
			: FText::Format(LOCTEXT("ChampionOf", "{0} of {1}"), FText::FromString(Match.ChampionName), AIB::Community(Match.ChampionCommunity).Name);
		HallList->AddSlot().AutoHeight().Padding(0.f, 3.f, 10.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::WhiteBrush())
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.04f))
			.Padding(FMargin(18.f, 10.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)
				[
					AIBUI::Icon(AIBIcon::Trophy, 26, Match.ChampionName.IsEmpty() ? AIBStyle::TextFaint() : AIBStyle::Gold())
				]
				+ SHorizontalBox::Slot().FillWidth(0.22f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::AsDateTime(Match.Date, EDateTimeStyle::Short, EDateTimeStyle::Short)).Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(AIB::EndingTitle(Match.Ending)).Font(AIBStyle::Spaced(AIBStyle::Heading(19), 60)).ColorAndOpacity(AIBStyle::Gold())
				]
				+ SHorizontalBox::Slot().FillWidth(0.28f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Champion).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(STextBlock).Text(FText::FromString(You)).Font(AIBStyle::BodyMedium(14)).ColorAndOpacity(AIBStyle::Acid())
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
	TSharedRef<SWidget> Body = SNew(SBox)
		.MaxDesiredHeight(560.f)
		[
			SNew(SScrollBox)
			.ScrollBarThickness(FVector2D(5.f, 5.f))
			+ SScrollBox::Slot().Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(STextBlock).Text(FText::FromString(Credits)).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
			]
		];
	return SubPage(TEXT("S2_Fleets"), LOCTEXT("CreditsKicker", "MADE BY"), LOCTEXT("CreditsTitle", "CREDITS"), Body, 960.f);
}

#undef LOCTEXT_NAMESPACE
