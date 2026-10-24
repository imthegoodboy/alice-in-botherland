// Alice in Botherland - in-match HUD: stage panels

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBGameUI.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBVehicle.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Online/AIBSessionSubsystem.h"
#include "Core/AIBGameInstance.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSlider.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "AIBHudPanels"

namespace
{
	FText StatusWord(const AAIBPlayerState* PS)
	{
		switch (PS->Status)
		{
		case EAIBPlayerStatus::Champion: return LOCTEXT("WChampion", "CHAMPION - CLAIMED THE ISLAND");
		case EAIBPlayerStatus::Sacrificed: return LOCTEXT("WSacrificed", "SACRIFICED - SAVED THEIR PEOPLE");
		case EAIBPlayerStatus::Forfeited: return LOCTEXT("WForfeit", "FORFEITED");
		case EAIBPlayerStatus::Eliminated: return FText::Format(LOCTEXT("WElim", "ELIMINATED IN STAGE {0}"), PS->EliminatedInStage);
		default: return LOCTEXT("WFinalist", "FINALIST");
		}
	}

	TSharedRef<SWidget> SuitIcon(EAIBSymbol Symbol, float Size, bool bGlow = true)
	{
		const FSlateBrush* Brush = AIBStyle::SuitBrush(Symbol, bGlow);
		if (Brush)
		{
			return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
				[
					SNew(SImage).Image(Brush).ColorAndOpacity(AIB::SymbolColor(Symbol))
				];
		}
		return SNew(STextBlock).Text(FText::FromString(AIB::SymbolGlyph(Symbol))).Font(AIBStyle::Heading(FMath::RoundToInt(Size * 0.8f))).ColorAndOpacity(AIB::SymbolColor(Symbol));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Lobby
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeLobbyPanel()
{
	auto IsLobby = [this]() { const EAIBPhase P = Phase(); return P == EAIBPhase::Lobby || P == EAIBPhase::LobbyCountdown || P == EAIBPhase::None; };
	auto IsHost = [this]() { return PC() && PC()->IsHost(); };
	auto SeatedCount = [this]() { const AAIBGameState* G = GS(); return G ? G->GetSeatedPlayers().Num() : 0; };
	auto NotReadyCount = [this]()
	{
		const AAIBGameState* G = GS();
		int32 Count = 0;
		if (G)
		{
			for (const AAIBPlayerState* PS : G->GetSeatedPlayers())
			{
				// The host readies up by pressing START.
				Count += (!PS->bReady && !PS->bHost) || PS->bDisconnected ? 1 : 0;
			}
		}
		return Count;
	};
	auto Code = [this]() -> FString
	{
		const AAIBGameState* G = GS();
		if (!G)
		{
			return FString();
		}
		return bShowInternetCode && !G->RoomCodeInternet.IsEmpty() ? G->RoomCodeInternet : G->RoomCode;
	};
	auto MicMode = [this]()
	{
		UAIBGameInstance* GI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr;
		return GI ? GI->GetSettings().MicMode : 1;
	};

	// Champion cards: one per community, across the bottom.
	TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		auto Seat = [this, Slot]() -> AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
		const FLinearColor Color = AIB::Community(Slot).Color;
		auto IsMe = [this, Seat]() { return Seat() && Seat() == MyPS(); };
		auto StatusText = [this, Seat]()
		{
			const AAIBPlayerState* PS = Seat();
			if (!PS) return LOCTEXT("SeatOpen", "WAITING");
			if (PS->bDisconnected) return FText::Format(LOCTEXT("SeatReconnect", "RECONNECTING {0}"), AIBStyle::Clock(PS->ReconnectDeadline - ServerNow()));
			if (PS->bBot) return LOCTEXT("SeatAI", "AI READY");
			if (PS->bHost && !PS->bReady) return LOCTEXT("SeatHost", "HOST");
			return PS->bReady ? LOCTEXT("SeatReady", "READY") : LOCTEXT("SeatNotReady", "NOT READY");
		};
		auto StatusColor = [Seat]()
		{
			const AAIBPlayerState* PS = Seat();
			if (!PS) return FSlateColor(AIBStyle::TextFaint());
			if (PS->bDisconnected) return FSlateColor(AIBStyle::Gold());
			if (PS->bHost && !PS->bReady) return FSlateColor(AIBStyle::Gold());
			return FSlateColor(PS->bReady || PS->bBot ? AIBStyle::Acid() : AIBStyle::Blood());
		};

		Cards->AddSlot().AutoWidth().Padding(6.f, 0.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(0.f, 50.f)).Delay(0.08f * Slot)
			[
				SNew(SBox)
				.WidthOverride(212.f)
				.HeightOverride(322.f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda([Seat]() { return FSlateColor(FLinearColor(0.012f, 0.013f, 0.018f, Seat() ? 0.82f : 0.55f)); })
					]
					// Community colour washing down from the top.
					+ SOverlay::Slot().VAlign(VAlign_Top)
					[
						SNew(SBox)
						.HeightOverride(190.f)
						.Visibility_Lambda([Seat]() { return Seat() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
						[
							SNew(SAIBGradient).Colors({ FLinearColor(Color.R, Color.G, Color.B, 0.5f), FLinearColor(Color.R, Color.G, Color.B, 0.f) })
						]
					]
					+ SOverlay::Slot().VAlign(VAlign_Top)
					[
						SNew(SBox).HeightOverride(4.f)
						[
							SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda([Seat, Color]() { return FSlateColor(FLinearColor(Color.R, Color.G, Color.B, Seat() ? 1.f : 0.3f)); })
						]
					]
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.Visibility(EVisibility::HitTestInvisible)
						.BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f))
						.BorderBackgroundColor_Lambda([IsMe, Seat, Color]()
						{
							return FSlateColor(IsMe() ? AIBStyle::Gold() : FLinearColor(Color.R, Color.G, Color.B, Seat() ? 0.45f : 0.15f));
						})
					]
					+ SOverlay::Slot()
					[
						SNew(SBox).Visibility_Lambda([IsMe]() { return IsMe() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
						[
							SNew(SAIBCorners).Color(AIBStyle::Gold()).Length(24.f).Thickness(3.f)
						]
					]
					// Crest.
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 34.f, 0.f, 0.f)
					[
						SNew(SBox).WidthOverride(92.f).HeightOverride(92.f)
						[
							SNew(SImage).Image(AIBStyle::CrestBrush(Slot)).ColorAndOpacity_Lambda([Seat, Color]() { return FSlateColor(Seat() ? Color : FLinearColor(Color.R, Color.G, Color.B, 0.22f)); })
						]
					]
					// YOU / AI / talking (top left), remove (top right).
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(12.f, 14.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBox).Visibility_Lambda([IsMe]() { return IsMe() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							[
								AIBUI::Pill(LOCTEXT("YouTag", "YOU"), AIBStyle::Gold(), 12)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
						[
							SNew(SBox).Visibility_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && (PS->bBot || PS->bHost) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							[
								AIBUI::Pill(TAttribute<FText>::CreateLambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bHost ? LOCTEXT("HostTag", "HOST") : LOCTEXT("AITag", "AI"); }), AIBStyle::Text(), 12)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
						[
							SNew(SBox).Visibility_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bTalking ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							[
								AIBUI::Icon(AIBIcon::Mic, 20, TAttribute<FSlateColor>::CreateLambda([]() { FLinearColor C = AIBStyle::Acid(); C.A = 0.6f + 0.4f * FMath::Sin(FPlatformTime::Seconds() * 9.0); return FSlateColor(C); }))
							]
						]
					]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 8.f, 8.f, 0.f)
					[
						SNew(SBox)
						.Visibility_Lambda([this, Seat]()
						{
							const AAIBPlayerState* PS = Seat();
							return PC() && PC()->IsHost() && PS && PS != MyPS() && Phase() == EAIBPhase::Lobby ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							SNew(SAIBIconButton).Icon(AIBIcon::Close).Size(30.f)
							.OnClicked_Lambda([this, Slot]() { if (PC()) PC()->ServerRemoveSeat(Slot); return FReply::Handled(); })
						]
					]
					// Name, community, status.
					+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(14.f, 0.f, 14.f, 16.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Font(AIBStyle::Spaced(AIBStyle::Heading(24), 40))
							.Justification(ETextJustify::Center)
							.Text_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS ? FText::FromString(PS->GetPlayerName().ToUpper()) : LOCTEXT("OpenSeat", "OPEN SEAT"); })
							.ColorAndOpacity_Lambda([Seat]() { return FSlateColor(Seat() ? AIBStyle::Text() : AIBStyle::TextFaint()); })
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock).Font(AIBStyle::SubHeading(15)).Text(AIB::Community(Slot).Name).ColorAndOpacity(FLinearColor(Color.R, Color.G, Color.B, 0.95f))
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 10.f)
						[
							SNew(STextBlock).Font(AIBStyle::Body(12)).Text(AIB::Community(Slot).ShipName).ColorAndOpacity(AIBStyle::TextDim())
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox)
							.Visibility_Lambda([this, Seat]() { return !Seat() && PC() && PC()->IsHost() && Phase() == EAIBPhase::Lobby && GS() && !GS()->bAITestMode ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
							[
								AIBUI::Pill(TAttribute<FText>::CreateLambda(StatusText), TAttribute<FSlateColor>::CreateLambda(StatusColor), 15)
							]
						]
						// Host: put an AI champion in an open seat.
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox)
							.Visibility_Lambda([this, Seat]() { return !Seat() && PC() && PC()->IsHost() && Phase() == EAIBPhase::Lobby && GS() && !GS()->bAITestMode ? EVisibility::Visible : EVisibility::Collapsed; })
							[
								SNew(SAIBSlantButton).Icon(AIBIcon::Robot).Text(LOCTEXT("AddAISeat", "ADD AI")).FontSize(16).Height(38.f).Centered(true)
								.OnClicked_Lambda([this]() { if (PC()) PC()->ServerAddBot(); return FReply::Handled(); })
							]
						]
					]
				]
			]
		];
	}

	// Room code (top left, under the title).
	TSharedRef<SWidget> RoomCodeBlock = AIBUI::Frame(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 22.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Font(AIBStyle::Spaced(AIBStyle::SubHeading(15), 260))
				.ColorAndOpacity(AIBStyle::TextDim())
				.Text_Lambda([this]() { return bShowInternetCode ? LOCTEXT("InternetCode", "INTERNET ROOM CODE") : LOCTEXT("RoomCode", "ROOM CODE"); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, -6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font_Lambda([Code]() { return Code().IsEmpty() ? AIBStyle::Spaced(AIBStyle::Title(30), 60) : AIBStyle::Spaced(AIBStyle::Title(70), 160); })
				.ColorAndOpacity(AIBStyle::Gold())
				.ShadowOffset(FVector2D(2.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f))
				.Text_Lambda([this, Code]()
				{
					const FString C = Code();
					if (!C.IsEmpty())
					{
						return FText::FromString(C);
					}
					if (bShowInternetCode)
					{
						return LOCTEXT("InternetPending", "LOOKING UP YOUR INTERNET ADDRESS...");
					}
					return PC() && PC()->GetWorld() && PC()->GetWorld()->GetNetMode() == NM_Standalone ? LOCTEXT("NoNetwork", "OFFLINE - CREATE THE ROOM AGAIN") : LOCTEXT("CodePending", "GETTING YOUR ROOM CODE...");
				})
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Font(AIBStyle::Body(14))
				.ColorAndOpacity(AIBStyle::TextDim())
				.Text_Lambda([this]()
				{
					return bShowInternetCode ? LOCTEXT("InternetHelp", "For friends elsewhere. Forward UDP port 7777 on your router to this PC.")
						: LOCTEXT("LanHelp", "Friends on your network: JOIN ROOM, then type this code.");
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SAIBSlantButton)
			.Kind(EAIBButtonKind::Gold)
			.Icon(AIBIcon::Copy)
			.FontSize(19)
			.Height(52.f)
			.MinWidth(150.f)
			.IsEnabled_Lambda([Code]() { return !Code().IsEmpty(); })
			.Text_Lambda([this]() { return FPlatformTime::Seconds() - CopiedTime < 2.0 ? LOCTEXT("Copied", "COPIED") : LOCTEXT("Copy", "COPY"); })
			.OnClicked_Lambda([this, Code]()
			{
				const FString C = Code();
				if (!C.IsEmpty())
				{
					FPlatformApplicationMisc::ClipboardCopy(*C);
					CopiedTime = FPlatformTime::Seconds();
				}
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SAIBIconButton)
			.Icon_Lambda([this]() { return bShowInternetCode ? AIBIcon::Lan : AIBIcon::Globe; })
			.Label_Lambda([this]() { return bShowInternetCode ? LOCTEXT("ShowLan", "LOCAL") : LOCTEXT("ShowInternet", "INTERNET"); })
			.IsActive_Lambda([this]() { return bShowInternetCode; })
			.Size(48.f)
			.OnClicked_Lambda([this]() { bShowInternetCode = !bShowInternetCode; return FReply::Handled(); })
		],
		FMargin(26.f, 14.f), FLinearColor(1.f, 0.74f, 0.32f, 0.9f), 0.78f);

	// Start / ready, AI test mode and status (bottom left).
	TSharedRef<SWidget> Actions = SNew(SBox)
		.WidthOverride(500.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 0.f, 12.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
				[
					AIBUI::Icon(AIBIcon::Hourglass, 26, AIBStyle::Gold())
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::BodyMedium(19))
					.AutoWrapText(true)
					.ColorAndOpacity(AIBStyle::Text())
					.ShadowOffset(FVector2D(1.f, 1.f))
					.Text_Lambda([this, IsHost, SeatedCount, NotReadyCount]()
					{
						const AAIBGameState* G = GS();
						if (Phase() == EAIBPhase::LobbyCountdown)
						{
							return LOCTEXT("StatusStarting", "The trials are about to begin.");
						}
						const int32 Missing = AIB::MaxPlayers - SeatedCount();
						if (Missing > 0)
						{
							return G && G->bAITestMode ? LOCTEXT("StatusFilling", "AI champions are taking the empty seats...")
								: FText::Format(LOCTEXT("StatusMissing", "Waiting for {0} more {0}|plural(one=champion,other=champions). Share the room code."), Missing);
						}
						const int32 NotReady = NotReadyCount();
						if (NotReady > 0)
						{
							return FText::Format(LOCTEXT("StatusNotReady", "{0} {0}|plural(one=champion is,other=champions are) not ready yet."), NotReady);
						}
						return IsHost() ? LOCTEXT("StatusHostGo", "Everyone is here. Start the trials!") : LOCTEXT("StatusWaitHost", "Everyone is here. Waiting for the host to start.");
					})
				]
			]
			// AI test mode, right above START.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::WhiteBrush())
				.BorderBackgroundColor(FLinearColor(0.012f, 0.013f, 0.018f, 0.72f))
				.Padding(FMargin(16.f, 10.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
					[
						AIBUI::Icon(AIBIcon::Robot, 30, TAttribute<FSlateColor>::CreateLambda([this]() { return FSlateColor(GS() && GS()->bAITestMode ? AIBStyle::Ember() : AIBStyle::TextDim()); }))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(LOCTEXT("AITest", "AI TEST MODE")).Font(AIBStyle::Spaced(AIBStyle::Heading(19), 80)).ColorAndOpacity(AIBStyle::Text())
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Font(AIBStyle::Body(13))
							.ColorAndOpacity(AIBStyle::TextDim())
							.Text_Lambda([IsHost]() { return IsHost() ? LOCTEXT("AITestSub", "Fill every empty seat with an AI champion") : LOCTEXT("AITestGuest", "Switched by the host"); })
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SAIBToggle)
						.IsEnabled_Lambda([this, IsHost]() { return IsHost() && Phase() == EAIBPhase::Lobby; })
						.IsOn_Lambda([this]() { return GS() && GS()->bAITestMode; })
						.OnToggled_Lambda([this](bool bOn) { if (PC()) PC()->ServerSetAITestMode(bOn); })
					]
				]
			]
			// Start (host) / ready (everyone else).
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([IsHost]() { return IsHost() ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SAIBSlantButton)
					.Kind(EAIBButtonKind::Primary)
					.Icon(AIBIcon::Play)
					.FontSize(40)
					.Height(104.f)
					.MinWidth(500.f)
					.Shine(true)
					.IsEnabled_Lambda([this, SeatedCount, NotReadyCount]() { return Phase() == EAIBPhase::Lobby && SeatedCount() >= AIB::MaxPlayers && NotReadyCount() == 0; })
					.Text_Lambda([this]() { return Phase() == EAIBPhase::LobbyCountdown ? LOCTEXT("Starting", "STARTING") : LOCTEXT("Start", "START"); })
					.SubText_Lambda([this, SeatedCount, NotReadyCount]()
					{
						if (Phase() != EAIBPhase::Lobby) return LOCTEXT("StartSoon", "The trials begin");
						if (SeatedCount() < AIB::MaxPlayers) return FText::Format(LOCTEXT("StartNeeds", "{0} of {1} champions here"), SeatedCount(), AIB::MaxPlayers);
						if (NotReadyCount() > 0) return LOCTEXT("StartNotReady", "Waiting for everyone to be ready");
						return LOCTEXT("StartGo", "Begin the trials");
					})
					.OnClicked_Lambda([this]() { if (PC()) PC()->ServerStartMatch(); return FReply::Handled(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([IsHost]() { return IsHost() ? EVisibility::Collapsed : EVisibility::Visible; })
				[
					SNew(SAIBSlantButton)
					.Kind(EAIBButtonKind::Primary)
					.Icon(AIBIcon::Check)
					.FontSize(40)
					.Height(104.f)
					.MinWidth(500.f)
					.Shine(true)
					.Text_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->bReady ? LOCTEXT("Unready", "NOT READY") : LOCTEXT("Ready", "READY"); })
					.SubText_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->bReady ? LOCTEXT("UnreadySub", "Click if you need more time") : LOCTEXT("ReadySub", "Tell the host you are ready"); })
					.IsEnabled_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->Slot != INDEX_NONE && Phase() == EAIBPhase::Lobby; })
					.OnClicked_Lambda([this]()
					{
						const AAIBPlayerState* Me = MyPS();
						if (PC() && Me) PC()->ServerSetReady(!Me->bReady);
						return FReply::Handled();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(30.f, 10.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Spaced(AIBStyle::SubHeading(14), 160))
				.ColorAndOpacity(AIBStyle::TextFaint())
				.Text(LOCTEXT("LobbyKeys", "ENTER  chat      V  talk      ESC  menu"))
			]
		];

	return SNew(SOverlay)
		.Visibility_Lambda([IsLobby]() { return IsLobby() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		// Shade the edges so the panels read over the fleet.
		+ SOverlay::Slot()
		[
			SNew(SAIBGradient).Vertical(false).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.75f), FLinearColor(0.f, 0.f, 0.f, 0.25f), FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.f) })
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(460.f)[SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.82f) })]
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(200.f)[SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.6f), FLinearColor(0.f, 0.f, 0.f, 0.f) })]
		]
		// Title and room code.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(84.f, 48.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
			[
				SNew(SAIBReveal).Offset(FVector2D(0.f, -30.f))
				[
					AIBUI::PageTitle(LOCTEXT("RoomKicker", "FIVE FLEETS. FIVE SEATS. ONE ISLAND."), LOCTEXT("Gathering", "THE GATHERING"), 76)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SNew(SAIBReveal).Delay(0.1f)[RoomCodeBlock]
			]
		]
		// Rules, microphone, settings, leave (top right).
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 54.f, 72.f, 0.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(0.f, -30.f)).Delay(0.1f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
				[
					SNew(SAIBIconButton).Icon(AIBIcon::Book).Label(LOCTEXT("NavRules", "RULES"))
					.OnClicked_Lambda([this]() { OpenRules(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
				[
					SNew(SAIBIconButton)
					.Icon_Lambda([MicMode]() { return MicMode() == 0 ? AIBIcon::MicOff : AIBIcon::Mic; })
					.Label_Lambda([MicMode]()
					{
						switch (MicMode())
						{
						case 0: return LOCTEXT("MicOff", "MIC OFF");
						case 2: return LOCTEXT("MicOpen", "MIC OPEN");
						default: return LOCTEXT("MicPtt", "HOLD V");
						}
					})
					.IsActive_Lambda([this]() { return PC() && PC()->IsSpeaking(); })
					.OnClicked_Lambda([this]()
					{
						// Cycle push-to-talk -> open mic -> off.
						if (UAIBGameInstance* GI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr)
						{
							FAIBSettings S = GI->GetSettings();
							S.MicMode = S.MicMode == 1 ? 2 : (S.MicMode == 2 ? 0 : 1);
							GI->SetSettings(S);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f)
				[
					SNew(SAIBIconButton).Icon(AIBIcon::Settings).Label(LOCTEXT("NavSettings", "SETTINGS"))
					.OnClicked_Lambda([this]() { OpenSettings(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
				[
					SNew(SAIBIconButton).Icon(AIBIcon::Logout).Label(LOCTEXT("LeaveLobby", "LEAVE"))
					.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
				]
			]
		]
		// Champions (bottom right).
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 66.f, 60.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 0.f, 0.f, 10.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(SBox).WidthOverride(26.f).HeightOverride(3.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Ember())]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("Champions", "CHAMPIONS")).Font(AIBStyle::Spaced(AIBStyle::SubHeading(17), 260)).ColorAndOpacity(AIBStyle::Gold())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Title(30))
					.Text_Lambda([SeatedCount]() { return FText::Format(LOCTEXT("SeatCount", "{0} / {1}"), SeatedCount(), AIB::MaxPlayers); })
					.ColorAndOpacity_Lambda([SeatedCount]() { return FSlateColor(SeatedCount() >= AIB::MaxPlayers ? AIBStyle::Acid() : AIBStyle::Text()); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[Cards]
		]
		// Start / ready (bottom left).
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(84.f, 0.f, 0.f, 60.f)
		[
			SNew(SAIBReveal).Offset(FVector2D(-60.f, 0.f)).Delay(0.3f)[Actions]
		]
		// Countdown.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 150.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return Phase() == EAIBPhase::LobbyCountdown ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("BeginIn", "THE TRIALS BEGIN IN")).Font(AIBStyle::Spaced(AIBStyle::Heading(26), 200)).ColorAndOpacity(AIBStyle::Gold()).ShadowOffset(FVector2D(1.f, 1.f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(150))
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D(3.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
				.Text_Lambda([this]() { const AAIBGameState* G = GS(); return FText::AsNumber(G ? FMath::CeilToInt(G->GetPhaseRemaining()) : 0); })
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Stage 1
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeNumberPanel()
{
	auto IsParticipant = [this]() { const AAIBGameState* G = GS(); const AAIBPlayerState* Me = MyPS(); return G && Me && G->Stage1Participants.Contains(Me->Slot) && Me->IsActiveChampion(); };
	auto CanPick = [this, IsParticipant]() { const AAIBPlayerState* Me = MyPS(); return Phase() == EAIBPhase::Stage1Selection && IsParticipant() && Me && !Me->bSubmitted; };
	auto Nudge = [this](int32 Delta)
	{
		return FOnClicked::CreateLambda([this, Delta]()
		{
			if (PC())
			{
				PC()->DraftNumber = FMath::Clamp(PC()->DraftNumber + Delta, 0, 100);
			}
			return FReply::Handled();
		});
	};

	TSharedRef<SWidget> Picker = SNew(SBox)
		.WidthOverride(560.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Heading(20))
					.ColorAndOpacity(AIBStyle::TextDim())
					.Text_Lambda([this, CanPick]()
					{
						const AAIBPlayerState* Me = MyPS();
						if (Phase() == EAIBPhase::Stage1RoundIntro) return LOCTEXT("GetReady", "GET READY - PICK A NUMBER FROM 0 TO 100");
						if (Me && Me->bSubmitted) return LOCTEXT("Locked", "NUMBER LOCKED IN");
						const AAIBGameState* G = GS();
						if (G && Me && G->bStage1SuddenDeath)
						{
							return G->Stage1AtRisk.Contains(Me->Slot) ? LOCTEXT("PickAtRisk", "SUDDEN DEATH - YOU ARE AT RISK") : LOCTEXT("PickSafe", "YOU ARE SAFE - BUT YOUR NUMBER MOVES THE TARGET");
						}
						return LOCTEXT("PickNumber", "YOUR SECRET NUMBER");
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Title(140))
					.ColorAndOpacity_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return FSlateColor(Me && Me->bSubmitted ? AIBStyle::Acid() : AIBStyle::Text()); })
					.Text_Lambda([this]() { return FText::AsNumber(PC() ? PC()->DraftNumber : 50); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(SSlider)
					.IsEnabled_Lambda(CanPick)
					.MinValue(0.f)
					.MaxValue(100.f)
					.StepSize(1.f)
					.MouseUsesStep(true)
					.SliderBarColor(FLinearColor(1.f, 1.f, 1.f, 0.2f))
					.SliderHandleColor(AIBStyle::Blood())
					.Value_Lambda([this]() { return PC() ? (float)PC()->DraftNumber : 50.f; })
					.OnValueChanged_Lambda([this](float Value) { if (PC()) PC()->DraftNumber = FMath::RoundToInt(Value); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					.IsEnabled_Lambda(CanPick)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f)[SNew(SAIBButton).Text(FText::FromString(TEXT("-10"))).FontSize(18).Height(44.f).OnClicked(Nudge(-10))]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f)[SNew(SAIBButton).Text(FText::FromString(TEXT("-1"))).FontSize(18).Height(44.f).OnClicked(Nudge(-1))]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f)[SNew(SAIBButton).Text(FText::FromString(TEXT("+1"))).FontSize(18).Height(44.f).OnClicked(Nudge(1))]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f)[SNew(SAIBButton).Text(FText::FromString(TEXT("+10"))).FontSize(18).Height(44.f).OnClicked(Nudge(10))]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
				[
					SNew(SAIBButton)
					.Kind(EAIBButtonKind::Primary)
					.FontSize(26)
					.Height(62.f)
					.IsEnabled_Lambda(CanPick)
					.Text_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->bSubmitted ? LOCTEXT("Waiting2", "WAITING FOR THE OTHERS...") : LOCTEXT("LockIn", "LOCK IN"); })
					.OnClicked_Lambda([this]()
					{
						if (PC())
						{
							PC()->ServerSubmitNumber(PC()->DraftNumber);
							AIBStyle::PlayUISound(TEXT("SFX_Submit"));
						}
						return FReply::Handled();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Body(14))
					.ColorAndOpacity(AIBStyle::TextDim())
					.Text(LOCTEXT("NumberHint", "Target = average of all numbers x 0.8. Farthest from the target is eliminated. No number in time = random."))
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
				],
				FMargin(26.f))
		];

	TSharedRef<SWidget> Safe = AIBUI::Panel(
		SNew(STextBlock)
		.Font(AIBStyle::Heading(24))
		.ColorAndOpacity(AIBStyle::TextDim())
		.Text_Lambda([this]()
		{
			const AAIBPlayerState* Me = MyPS();
			return Me && Me->IsActiveChampion() ? LOCTEXT("SafeRound", "WATCH THE SUDDEN DEATH") : LOCTEXT("Watching", "WATCHING THE NUMBER GAME");
		}),
		FMargin(24.f, 14.f));

	return SNew(SOverlay)
		.Visibility_Lambda([this]()
		{
			const EAIBPhase P = Phase();
			return P == EAIBPhase::Stage1RoundIntro || P == EAIBPhase::Stage1Selection ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 48.f, 60.f)
		[
			SNew(SBox).Visibility_Lambda([IsParticipant]() { return IsParticipant() ? EVisibility::Visible : EVisibility::Collapsed; })[Picker]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0.f, 260.f, 0.f, 0.f)
		[
			SNew(SBox).Visibility_Lambda([IsParticipant]() { return IsParticipant() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })[Safe]
		];
}

TSharedRef<SWidget> SAIBHud::MakeNumberResults()
{
	SAssignNew(NumberResultsBox, SVerticalBox);
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			if (!G) return EVisibility::Collapsed;
			const bool bShow = G->Phase == EAIBPhase::Stage1Results || (G->Phase == EAIBPhase::Stage1Elimination && G->Fallen.Num() == 0) || (G->Phase == EAIBPhase::Stage1Reveal && G->GetPhaseAlpha() > 0.7f);
			return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 48.f, 60.f))
		[
			SNew(SBox).WidthOverride(560.f)
			[
				AIBUI::Panel(NumberResultsBox.ToSharedRef(), FMargin(24.f))
			]
		];
}

void SAIBHud::RebuildNumberResults()
{
	const AAIBGameState* G = GS();
	if (!G || !NumberResultsBox.IsValid())
	{
		return;
	}
	NumberResultsBox->ClearChildren();
	const FAIBStage1Round& Round = G->Stage1Last;
	NumberResultsBox->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(STextBlock).Text(LOCTEXT("TargetLbl", "TARGET")).Font(AIBStyle::Heading(20)).ColorAndOpacity(AIBStyle::TextDim())
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%.1f"), Round.Target)))
			.Font(AIBStyle::Title(56)).ColorAndOpacity(AIBStyle::Gold())
		]
	];
	NumberResultsBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("AvgLine", "Average {0}  x  0.8"), FText::FromString(FString::Printf(TEXT("%.1f"), Round.Average))))
		.Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim())
	];
	TArray<FAIBNumberEntry> Sorted = Round.Entries;
	Sorted.Sort([](const FAIBNumberEntry& A, const FAIBNumberEntry& B) { return A.Distance < B.Distance; });
	for (const FAIBNumberEntry& Entry : Sorted)
	{
		const AAIBPlayerState* PS = G->GetPlayerBySlot(Entry.Slot);
		const FString Name = PS ? PS->GetPlayerName() : TEXT("?");
		FText RowTag;
		FLinearColor TagColor = AIBStyle::TextDim();
		if (Entry.bWinner)
		{
			RowTag = LOCTEXT("TagClosest", "CLOSEST");
			TagColor = AIBStyle::Gold();
		}
		else if (Entry.bFarthest)
		{
			RowTag = Round.EliminatedSlot == Entry.Slot ? LOCTEXT("TagElim", "ELIMINATED") : LOCTEXT("TagTie", "TIED - SUDDEN DEATH");
			TagColor = AIBStyle::Blood();
		}
		NumberResultsBox->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(12.f, 6.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(4.f).HeightOverride(26.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIB::Community(Entry.Slot).Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(Name.ToUpper())).Font(AIBStyle::SubHeading(18)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().FillWidth(0.15f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::AsNumber(Entry.Number)).Font(AIBStyle::Title(30)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().FillWidth(0.15f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.1f%s"), Entry.Distance, Entry.bAuto ? TEXT(" *") : TEXT("")))).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SHorizontalBox::Slot().FillWidth(0.25f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(STextBlock).Text(RowTag).Font(AIBStyle::Heading(15)).ColorAndOpacity(TagColor)
				]
			]
		];
	}
	NumberResultsBox->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock).Text(LOCTEXT("AutoNote", "* no number submitted - random number assigned")).Font(AIBStyle::Body(12)).ColorAndOpacity(AIBStyle::TextFaint())
	];
}

#undef LOCTEXT_NAMESPACE
