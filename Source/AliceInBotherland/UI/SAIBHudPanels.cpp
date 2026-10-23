// Alice in Botherland - in-match HUD: stage panels

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
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

	// Seats: one card per community.
	TSharedRef<SVerticalBox> Seats = SNew(SVerticalBox);
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		auto Seat = [this, Slot]() -> AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
		const FLinearColor Color = AIB::Community(Slot).Color;
		auto SeatColor = [Seat, Color]() { return FSlateColor(Seat() ? Color : Color * FLinearColor(1.f, 1.f, 1.f, 0.35f)); };
		Seats->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.BorderBackgroundColor_Lambda([Seat]() { return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Seat() ? 1.f : 0.55f)); })
			.Padding(FMargin(0.f, 0.f, 12.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(4.f).HeightOverride(58.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda(SeatColor)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f)
				[
					SNew(SBox).WidthOverride(34.f).HeightOverride(34.f)
					[
						SNew(SImage).Image(AIBStyle::CrestBrush(Slot)).ColorAndOpacity_Lambda(SeatColor)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(AIBStyle::Heading(21))
							.Text_Lambda([this, Seat]()
							{
								const AAIBPlayerState* PS = Seat();
								if (!PS)
								{
									return LOCTEXT("OpenSeat", "OPEN SEAT");
								}
								return FText::FromString(PS->GetPlayerName().ToUpper() + (PS == MyPS() ? TEXT("  (YOU)") : TEXT("")));
							})
							.ColorAndOpacity_Lambda([Seat]() { return FSlateColor(Seat() ? AIBStyle::Text() : AIBStyle::TextFaint()); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							SNew(SBorder)
							.Visibility_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && (PS->bHost || PS->bBot) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							.BorderImage(AIBStyle::OutlineBrush(FLinearColor(1.f, 1.f, 1.f, 0.35f), 4.f, 1.f))
							.Padding(FMargin(6.f, 0.f))
							[
								SNew(STextBlock)
								.Font(AIBStyle::SubHeading(12))
								.ColorAndOpacity(AIBStyle::TextDim())
								.Text_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bHost ? LOCTEXT("HostTag", "HOST") : LOCTEXT("AITag", "AI"); })
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							AIBUI::TalkDot(TAttribute<bool>::CreateLambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bTalking; }))
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Font(AIBStyle::Body(13))
						.ColorAndOpacity(Color * FLinearColor(1.f, 1.f, 1.f, 0.85f))
						.Text(FText::Format(LOCTEXT("SeatCommunity", "{0}  -  {1}"), AIB::Community(Slot).Name, AIB::Community(Slot).ShipName))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Heading(16))
					.Text_Lambda([this, Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						if (!PS) return LOCTEXT("Waiting", "WAITING...");
						if (PS->bDisconnected) return FText::Format(LOCTEXT("SeatReconnect", "RECONNECTING {0}"), AIBStyle::Clock(PS->ReconnectDeadline - ServerNow()));
						if (PS->bBot) return LOCTEXT("SeatAI", "READY");
						if (PS->bHost && !PS->bReady) return FText::GetEmpty();
						return PS->bReady ? LOCTEXT("SeatReady", "READY") : LOCTEXT("SeatNotReady", "NOT READY");
					})
					.ColorAndOpacity_Lambda([Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						if (!PS) return FSlateColor(AIBStyle::TextFaint());
						if (PS->bDisconnected) return FSlateColor(AIBStyle::Gold());
						return FSlateColor(PS->bReady || PS->bBot ? AIBStyle::Acid() : AIBStyle::Blood());
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox)
					.Visibility_Lambda([this, Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						return PC() && PC()->IsHost() && PS && PS != MyPS() && Phase() == EAIBPhase::Lobby ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						SNew(SAIBButton)
						.Text(LOCTEXT("Remove", "X"))
						.FontSize(16)
						.Height(32.f)
						.Kind(EAIBButtonKind::Ghost)
						.OnClicked_Lambda([this, Slot]() { if (PC()) PC()->ServerRemoveSeat(Slot); return FReply::Handled(); })
					]
				]
			]
		];
	}

	// Left: the room (code + champions).
	TSharedRef<SWidget> Room = SNew(SBox)
		.WidthOverride(600.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Gathering", "THE GATHERING")).Font(AIBStyle::Title(54)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(STextBlock).Text(LOCTEXT("GatheringSub", "Five fleets answered the beacon. Five seats. One island.")).Font(AIBStyle::Story(15)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
				]
				// Room code.
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::CardBrush())
					.Padding(FMargin(18.f, 12.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::SubHeading(15))
								.ColorAndOpacity(AIBStyle::TextDim())
								.Text_Lambda([this]() { return bShowInternetCode ? LOCTEXT("InternetCode", "INTERNET ROOM CODE") : LOCTEXT("RoomCode", "ROOM CODE"); })
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Title(54))
								.ColorAndOpacity(AIBStyle::Gold())
								.Text_Lambda([Code]() { const FString C = Code(); return C.IsEmpty() ? LOCTEXT("NoCode", "- - - -") : FText::FromString(C); })
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Body(13))
								.ColorAndOpacity(AIBStyle::TextDim())
								.AutoWrapText(true)
								.Text_Lambda([this]()
								{
									const AAIBGameState* G = GS();
									if (bShowInternetCode)
									{
										return G && G->RoomCodeInternet.IsEmpty() ? LOCTEXT("InternetPending", "Looking up your internet address...")
											: LOCTEXT("InternetHelp", "For friends elsewhere. Your router must forward UDP port 7777 to this PC.");
									}
									return LOCTEXT("LanHelp", "Friends on your network open JOIN ROOM and type this code.");
								})
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
							[
								SNew(SAIBButton)
								.Kind(EAIBButtonKind::Gold)
								.MinWidth(150.f)
								.Height(44.f)
								.FontSize(18)
								.Text_Lambda([this]() { return FPlatformTime::Seconds() - CopiedTime < 2.0 ? LOCTEXT("Copied", "COPIED!") : LOCTEXT("Copy", "COPY CODE"); })
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
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SAIBButton)
								.Kind(EAIBButtonKind::Ghost)
								.MinWidth(150.f)
								.Height(30.f)
								.FontSize(14)
								.Text_Lambda([this]() { return bShowInternetCode ? LOCTEXT("ShowLan", "SHOW LOCAL CODE") : LOCTEXT("ShowInternet", "INTERNET CODE"); })
								.OnClicked_Lambda([this]() { bShowInternetCode = !bShowInternetCode; return FReply::Handled(); })
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						AIBUI::Header(LOCTEXT("Champions", "CHAMPIONS"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(AIBStyle::Heading(18))
						.Text_Lambda([SeatedCount]() { return FText::Format(LOCTEXT("SeatCount", "{0} / {1}"), SeatedCount(), AIB::MaxPlayers); })
						.ColorAndOpacity_Lambda([SeatedCount]() { return FSlateColor(SeatedCount() >= AIB::MaxPlayers ? AIBStyle::Acid() : AIBStyle::Gold()); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight()[Seats]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this, IsHost]() { return IsHost() && Phase() == EAIBPhase::Lobby && GS() && !GS()->bAITestMode ? EVisibility::Visible : EVisibility::Collapsed; })
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("AddAI", "ADD AI CHAMPION")).FontSize(16).Height(40.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerAddBot(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("FillAI", "FILL EMPTY SEATS")).FontSize(16).Height(40.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerFillBots(); return FReply::Handled(); })
					]
				],
				FMargin(28.f), true)
		];

	// Right: what happens next.
	auto MicText = [this]()
	{
		UAIBGameInstance* GI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr;
		switch (GI ? GI->GetSettings().MicMode : 1)
		{
		case 0: return LOCTEXT("MicOff", "MIC: OFF");
		case 2: return LOCTEXT("MicOpen", "MIC: OPEN");
		default: return LOCTEXT("MicPtt", "MIC: HOLD V");
		}
	};
	TSharedRef<SWidget> Actions = SNew(SBox)
		.WidthOverride(460.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				// Status.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::BodyMedium(17))
					.AutoWrapText(true)
					.ColorAndOpacity(AIBStyle::Text())
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
						return IsHost() ? LOCTEXT("StatusHostGo", "Everyone is here. Start the trials when you are ready.") : LOCTEXT("StatusWaitHost", "Everyone is here. Waiting for the host to start.");
					})
				]
				// AI test mode, right above START (the host switches it, everyone sees it).
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::CardBrush())
					.Padding(FMargin(14.f, 8.f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(LOCTEXT("AITest", "AI TEST MODE")).Font(AIBStyle::Heading(18)).ColorAndOpacity(AIBStyle::Text())
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Body(13))
								.ColorAndOpacity(AIBStyle::TextDim())
								.Text_Lambda([IsHost]() { return IsHost() ? LOCTEXT("AITestSub", "Empty seats get AI champions") : LOCTEXT("AITestGuest", "The host decides"); })
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAIBButton)
							.MinWidth(86.f)
							.Height(38.f)
							.FontSize(17)
							.IsEnabled_Lambda([this, IsHost]() { return IsHost() && Phase() == EAIBPhase::Lobby; })
							.AccentColor_Lambda([this]() { return GS() && GS()->bAITestMode ? FLinearColor(0.16f, 0.45f, 0.1f, 1.f) : FLinearColor(0.06f, 0.06f, 0.07f, 0.85f); })
							.Text_Lambda([this]() { return GS() && GS()->bAITestMode ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF"); })
							.OnClicked_Lambda([this]() { if (PC() && GS()) PC()->ServerSetAITestMode(!GS()->bAITestMode); return FReply::Handled(); })
						]
					]
				]
				// Start (host) / ready (everyone else).
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBButton)
					.Kind(EAIBButtonKind::Primary)
					.FontSize(26)
					.Height(64.f)
					.Visibility_Lambda([IsHost]() { return IsHost() ? EVisibility::Visible : EVisibility::Collapsed; })
					.IsEnabled_Lambda([this, SeatedCount, NotReadyCount]() { return Phase() == EAIBPhase::Lobby && SeatedCount() >= AIB::MaxPlayers && NotReadyCount() == 0; })
					.Text_Lambda([this]() { return Phase() == EAIBPhase::LobbyCountdown ? LOCTEXT("Starting", "STARTING...") : LOCTEXT("Start", "START THE TRIALS"); })
					.SubText_Lambda([this, SeatedCount, NotReadyCount]()
					{
						if (Phase() != EAIBPhase::Lobby) return FText::GetEmpty();
						if (SeatedCount() < AIB::MaxPlayers) return FText::Format(LOCTEXT("StartNeeds", "Needs all {1} champions ({0} / {1})"), SeatedCount(), AIB::MaxPlayers);
						if (NotReadyCount() > 0) return LOCTEXT("StartNotReady", "Waiting for everyone to be ready");
						return FText::GetEmpty();
					})
					.OnClicked_Lambda([this]() { if (PC()) PC()->ServerStartMatch(); return FReply::Handled(); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAIBButton)
					.Kind(EAIBButtonKind::Primary)
					.FontSize(26)
					.Height(64.f)
					.Visibility_Lambda([IsHost]() { return IsHost() ? EVisibility::Collapsed : EVisibility::Visible; })
					.Text_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->bReady ? LOCTEXT("Unready", "NOT READY") : LOCTEXT("Ready", "READY"); })
					.IsEnabled_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->Slot != INDEX_NONE && Phase() == EAIBPhase::Lobby; })
					.OnClicked_Lambda([this]()
					{
						const AAIBPlayerState* Me = MyPS();
						if (PC() && Me) PC()->ServerSetReady(!Me->bReady);
						return FReply::Handled();
					})
				]
				// Settings, microphone, rules.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("LobbySettings", "SETTINGS")).FontSize(16).Height(42.f)
						.OnClicked_Lambda([this]() { OpenSettings(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f)
					[
						SNew(SAIBButton).Text_Lambda(MicText).FontSize(16).Height(42.f)
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
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4.f, 0.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("LobbyRules", "RULES")).FontSize(16).Height(42.f)
						.OnClicked_Lambda([this]() { OpenRules(); return FReply::Handled(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(AIBStyle::Body(13))
						.ColorAndOpacity(AIBStyle::TextFaint())
						.Text(LOCTEXT("LobbyKeys", "ENTER  chat      V  talk"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SAIBButton).Text(LOCTEXT("LeaveLobby", "LEAVE ROOM")).Kind(EAIBButtonKind::Ghost).FontSize(16).Height(36.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
					]
				],
				FMargin(24.f), true)
		];

	return SNew(SOverlay)
		.Visibility_Lambda([IsLobby]() { return IsLobby() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(48.f, 40.f, 0.f, 40.f)[Room]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 48.f, 48.f)[Actions]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 40.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return Phase() == EAIBPhase::LobbyCountdown ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("BeginIn", "THE TRIALS BEGIN IN")).Font(AIBStyle::Heading(24)).ColorAndOpacity(AIBStyle::Gold()).ShadowOffset(FVector2D(1.f, 1.f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(110))
				.ColorAndOpacity(AIBStyle::Text())
				.ShadowOffset(FVector2D(2.f, 2.f))
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
			return Me && Me->IsActiveChampion() ? LOCTEXT("SafeRound", "YOU ARE SAFE THIS ROUND - WATCH THE SUDDEN DEATH") : LOCTEXT("Watching", "WATCHING THE NUMBER GAME");
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
			const bool bShow = G->Phase == EAIBPhase::Stage1Results || G->Phase == EAIBPhase::Stage1Elimination || (G->Phase == EAIBPhase::Stage1Reveal && G->GetPhaseAlpha() > 0.7f);
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
