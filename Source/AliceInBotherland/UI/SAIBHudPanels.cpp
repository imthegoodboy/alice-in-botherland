// Alice in Botherland - in-match HUD: stage panels

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBVehicle.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Online/AIBSessionSubsystem.h"
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
	auto Sessions = [this]() -> UAIBSessionSubsystem*
	{
		return PC() && PC()->GetGameInstance() ? PC()->GetGameInstance()->GetSubsystem<UAIBSessionSubsystem>() : nullptr;
	};

	TSharedRef<SVerticalBox> Seats = SNew(SVerticalBox);
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		auto Seat = [this, Slot]() -> AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
		const FLinearColor Color = AIB::Community(Slot).Color;
		Seats->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(0.f, 0.f, 12.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(5.f).HeightOverride(70.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(Color)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f)
				[
					SNew(SBox).WidthOverride(40.f).HeightOverride(40.f)
					[
						SNew(SImage).Image(AIBStyle::CrestBrush(Slot)).ColorAndOpacity(Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Font(AIBStyle::Heading(22))
						.Text_Lambda([this, Seat]()
						{
							const AAIBPlayerState* PS = Seat();
							if (!PS)
							{
								return LOCTEXT("OpenSeat", "OPEN SEAT");
							}
							FString Name = PS->GetPlayerName().ToUpper();
							if (PS == MyPS()) Name += TEXT("  (YOU)");
							if (PS->bHost) Name += TEXT("  - HOST");
							return FText::FromString(Name);
						})
						.ColorAndOpacity_Lambda([Seat]() { return FSlateColor(Seat() ? AIBStyle::Text() : AIBStyle::TextFaint()); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Font(AIBStyle::Body(14))
						.ColorAndOpacity(Color * 0.9f)
						.Text(FText::Format(LOCTEXT("SeatCommunity", "{0}  -  {1}"), AIB::Community(Slot).Name, AIB::Community(Slot).ShipName))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Heading(17))
					.Text_Lambda([this, Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						if (!PS) return LOCTEXT("Waiting", "WAITING...");
						if (PS->bDisconnected) return FText::Format(LOCTEXT("SeatReconnect", "RECONNECTING {0}"), AIBStyle::Clock(PS->ReconnectDeadline - ServerNow()));
						if (PS->bBot) return LOCTEXT("SeatAI", "AI - READY");
						return PS->bReady ? LOCTEXT("SeatReady", "READY") : LOCTEXT("SeatNotReady", "NOT READY");
					})
					.ColorAndOpacity_Lambda([Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						if (!PS) return FSlateColor(AIBStyle::TextFaint());
						if (PS->bDisconnected) return FSlateColor(AIBStyle::Gold());
						return FSlateColor(PS->bReady ? AIBStyle::Acid() : AIBStyle::Blood());
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
						.Height(34.f)
						.Kind(EAIBButtonKind::Ghost)
						.OnClicked_Lambda([this, Slot]() { if (PC()) PC()->ServerRemoveSeat(Slot); return FReply::Handled(); })
					]
				]
			]
		];
	}

	auto HostOnly = [this]() { return PC() && PC()->IsHost() && Phase() == EAIBPhase::Lobby ? EVisibility::Visible : EVisibility::Collapsed; };

	TSharedRef<SWidget> Left = SNew(SBox)
		.WidthOverride(640.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Gathering", "THE GATHERING")).Font(AIBStyle::Title(56)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock).Text(LOCTEXT("GatheringSub", "Five fleets answered the beacon. Five seats. One island.")).Font(AIBStyle::Story(16)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Body(14))
					.ColorAndOpacity(AIBStyle::Gold())
					.AutoWrapText(true)
					.Text_Lambda([Sessions, this]()
					{
						UAIBSessionSubsystem* S = Sessions();
						if (!S) return FText::GetEmpty();
						if (S->HasOnlinePlatform())
						{
							return FText::Format(LOCTEXT("OnlineInfo", "Online via {0}. Use INVITE FRIENDS, or friends can find this lobby in JOIN."), FText::FromString(S->GetPlatformName()));
						}
						return FText::Format(LOCTEXT("LanInfo", "LAN / direct connect. Friends join with your address: {0}  (port 7777 must be open for internet play)."), FText::FromString(S->GetLocalAddress()));
					})
				]
				+ SVerticalBox::Slot().AutoHeight()[Seats]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SAIBButton)
						.Kind(EAIBButtonKind::Primary)
						.FontSize(24)
						.Height(60.f)
						.Text_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->bReady ? LOCTEXT("Unready", "NOT READY") : LOCTEXT("Ready", "READY"); })
						.IsEnabled_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && Me->Slot != INDEX_NONE; })
						.OnClicked_Lambda([this]()
						{
							const AAIBPlayerState* Me = MyPS();
							if (PC() && Me) PC()->ServerSetReady(!Me->bReady);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f, 0.f, 0.f)
					[
						SNew(SAIBButton)
						.Text(LOCTEXT("Invite", "INVITE FRIENDS"))
						.Height(60.f)
						.Visibility_Lambda([this]() { return PC() && PC()->IsHost() ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked_Lambda([this, Sessions]()
						{
							UAIBSessionSubsystem* S = Sessions();
							if (!S || !S->ShowInviteUI())
							{
								if (PC())
								{
									PC()->Toasts.Add({ FText::Format(LOCTEXT("InviteManual", "Invites need Steam running. Share your address instead: {0}"), FText::FromString(S ? S->GetLocalAddress() : FString())), (float)PC()->GetWorld()->GetTimeSeconds() });
								}
							}
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda(HostOnly)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("AddAI", "ADD AI CHAMPION")).FontSize(17).Height(46.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerAddBot(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("FillAI", "FILL EMPTY SEATS WITH AI")).FontSize(17).Height(46.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerFillBots(); return FReply::Handled(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f).HAlign(HAlign_Left)
				[
					SNew(SAIBButton).Text(LOCTEXT("LeaveLobby", "LEAVE LOBBY")).Kind(EAIBButtonKind::Ghost).FontSize(16).Height(36.f)
					.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
				],
				FMargin(30.f))
		];

	TSharedRef<SWidget> Right = SNew(SBox)
		.WidthOverride(560.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(LOCTEXT("TheRules", "THE GUARDIAN'S LAW")).Font(AIBStyle::Title(36)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SVerticalBox::Slot().AutoHeight()[MakeRules(true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReadyRule", "The trials start 15 seconds after all five seats are filled and ready. Empty seats can be filled with AI champions."))
					.Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
				],
				FMargin(26.f))
		];

	return SNew(SOverlay)
		.Visibility_Lambda([IsLobby]() { return IsLobby() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(48.f, 40.f, 0.f, 40.f)[Left]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 40.f, 48.f, 40.f)[Right]
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

TSharedRef<SWidget> SAIBHud::MakeArrivalPanel()
{
	// The arrival message from the design document, as a title card over the cinematic.
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			return G && G->Phase == EAIBPhase::ArrivalCinematic && G->GetPhaseElapsed() > 13.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).MaxDesiredWidth(1100.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ArrivalMsg", "Five communities have found their last hope.\nOnly one community can claim this island.\nYour survival depends on your choices."))
				.Font(AIBStyle::StoryBold(30))
				.WrapTextAt(1100.f)
				.Justification(ETextJustify::Center)
				.ShadowOffset(FVector2D(2.f, 2.f))
				.ColorAndOpacity_Lambda([this]()
				{
					const AAIBGameState* G = GS();
					const float T = G ? G->GetPhaseElapsed() - 13.f : 0.f;
					return FSlateColor(FLinearColor(1.f, 0.96f, 0.9f, FMath::Clamp(T / 1.2f, 0.f, 1.f)));
				})
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
