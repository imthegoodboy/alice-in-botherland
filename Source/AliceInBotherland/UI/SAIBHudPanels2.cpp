// Alice in Botherland - in-match HUD: Hidden Symbol, race, final choice and results panels

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBVehicle.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "World/AIBGuardianSite.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "AIBHudPanels2"

namespace
{
	TSharedRef<SWidget> Suit(EAIBSymbol Symbol, float Size, float Alpha = 1.f)
	{
		if (Symbol == EAIBSymbol::None)
		{
			return SNew(SBox).WidthOverride(Size).HeightOverride(Size).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("?"))).Font(AIBStyle::Title(FMath::RoundToInt(Size * 0.9f))).ColorAndOpacity(AIBStyle::TextDim())
				];
		}
		FLinearColor Color = AIB::SymbolColor(Symbol);
		Color.A = Alpha;
		if (const FSlateBrush* Brush = AIBStyle::SuitBrush(Symbol, true))
		{
			return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SImage).Image(Brush).ColorAndOpacity(Color)];
		}
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(AIB::SymbolGlyph(Symbol))).Font(AIBStyle::Heading(FMath::RoundToInt(Size * 0.8f))).ColorAndOpacity(Color)
			];
	}

	const EAIBSymbol AllSymbols[] = { EAIBSymbol::Heart, EAIBSymbol::Diamond, EAIBSymbol::Club, EAIBSymbol::Spade };
}

// ---------------------------------------------------------------------------------------------------------------------
// Stage 2 - Hidden Symbol
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeSymbolPanel()
{
	SAssignNew(SymbolRows, SVerticalBox);
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const EAIBPhase P = Phase();
			return (P == EAIBPhase::Stage2Communication || P == EAIBPhase::Stage2Guess) && !bScoreboard ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 36.f, 0.f))
		[
			SNew(SBox).WidthOverride(430.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(LOCTEXT("YourSymbol", "YOUR SYMBOL")).Font(AIBStyle::Heading(22)).ColorAndOpacity(AIBStyle::Text())
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Font(AIBStyle::Body(14))
								.ColorAndOpacity(AIBStyle::TextDim())
								.AutoWrapText(true)
								.Text_Lambda([this]()
								{
									const AAIBPlayerState* Me = MyPS();
									const AAIBGameState* G = GS();
									if (!Me || !G || !G->Stage2Participants.Contains(Me->Slot))
									{
										return LOCTEXT("SpecSymbols", "You see every symbol. You cannot help them.");
									}
									return FText::Format(LOCTEXT("PointsLine", "Hidden from you.  Points: {0}"), Me->Stage2Points);
								})
							]
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBorder).BorderImage(AIBStyle::OutlineBrush(AIBStyle::Blood(), 8.f, 2.f)).Padding(FMargin(14.f, 6.f))
							[
								Suit(EAIBSymbol::None, 54.f)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 6.f)[AIBUI::Header(LOCTEXT("WhatYouSee", "WHAT YOU SEE ABOVE THE OTHERS"))]
					+ SVerticalBox::Slot().AutoHeight()[SymbolRows.ToSharedRef()]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(AIBStyle::Body(13))
						.ColorAndOpacity(AIBStyle::TextFaint())
						.AutoWrapText(true)
						.Text_Lambda([this]()
						{
							return IsChatOpen() ? LOCTEXT("QuickTellOn", "Click a symbol to tell that champion it's theirs - truth or lie.")
								: LOCTEXT("QuickTellOff", "Press ENTER to talk and use the quick-tell buttons.");
						})
					],
					FMargin(22.f))
			]
		];
}

void SAIBHud::RebuildSymbolRows()
{
	const AAIBGameState* G = GS();
	if (!G || !SymbolRows.IsValid())
	{
		return;
	}
	SymbolRows->ClearChildren();
	const AAIBPlayerState* Me = MyPS();
	const int32 MySlot = Me ? Me->Slot : INDEX_NONE;
	const bool bCompeting = G->Stage2Participants.Contains(MySlot);
	for (int32 Slot : G->Stage2Participants)
	{
		if (Slot == MySlot)
		{
			continue;
		}
		const AAIBPlayerState* PS = G->GetPlayerBySlot(Slot);
		if (!PS)
		{
			continue;
		}
		const FString Name = PS->GetPlayerName();
		TSharedRef<SHorizontalBox> Tells = SNew(SHorizontalBox);
		for (EAIBSymbol Symbol : AllSymbols)
		{
			Tells->AddSlot().AutoWidth().Padding(2.f, 0.f)
			[
				SNew(SBox).WidthOverride(40.f)
				[
					SNew(SAIBButton)
					.Kind(EAIBButtonKind::Secondary)
					.Height(36.f)
					.FontSize(18)
					.Text(FText::FromString(AIB::SymbolGlyph(Symbol)))
					.AccentColor(AIB::SymbolColor(Symbol) * FLinearColor(0.35f, 0.35f, 0.35f, 0.9f))
					.IsEnabled_Lambda([this]() { return Phase() == EAIBPhase::Stage2Communication && IsChatOpen(); })
					.OnClicked_Lambda([this, Slot, Symbol, Name]()
					{
						if (PC())
						{
							const FString Line = FString::Printf(TEXT("%s, your symbol is the %s %s."), *Name, *AIB::SymbolName(Symbol).ToString(), *AIB::SymbolGlyph(Symbol));
							PC()->SendChatMessage(Line, Slot, Symbol);
						}
						return FReply::Handled();
					})
				]
			];
		}
		SymbolRows->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(10.f, 6.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(4.f).HeightOverride(34.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(PS->GetCommunityColor())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(Name.ToUpper())).Font(AIBStyle::SubHeading(18)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					Suit(PC() ? PC()->GetVisibleSymbol(Slot) : EAIBSymbol::None, 34.f)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).Visibility(bCompeting ? EVisibility::Visible : EVisibility::Collapsed)[Tells]
				]
			]
		];
	}
}

TSharedRef<SWidget> SAIBHud::MakeGuessPanel()
{
	auto CanGuess = [this]()
	{
		const AAIBGameState* G = GS();
		const AAIBPlayerState* Me = MyPS();
		return G && Me && G->Phase == EAIBPhase::Stage2Guess && G->Stage2Participants.Contains(Me->Slot);
	};
	TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (EAIBSymbol Symbol : AllSymbols)
	{
		Cards->AddSlot().AutoWidth().Padding(10.f, 0.f)
		[
			SNew(SBox).WidthOverride(200.f)
			[
				SNew(SAIBButton)
				.Kind(EAIBButtonKind::Secondary)
				.Height(200.f)
				.FontSize(24)
				.Icon(AIBStyle::SuitBrush(Symbol, true))
				.Text(AIB::SymbolName(Symbol))
				.AccentColor_Lambda([this, Symbol]()
				{
					const AAIBPlayerState* Me = MyPS();
					const bool bPicked = PC() && PC()->DraftGuess == Symbol && Me && Me->bSubmitted;
					return bPicked ? AIB::SymbolColor(Symbol) * 0.6f : FLinearColor::Transparent;
				})
				.IsEnabled_Lambda([this]() { const AAIBPlayerState* Me = MyPS(); return Me && !Me->bSubmitted; })
				.OnClicked_Lambda([this, Symbol]()
				{
					if (PC())
					{
						PC()->DraftGuess = Symbol;
						PC()->ServerSubmitGuess(Symbol);
						AIBStyle::PlayUISound(TEXT("SFX_Submit"));
					}
					return FReply::Handled();
				})
			]
		];
	}
	return SNew(SBox)
		.Visibility_Lambda([CanGuess]() { return CanGuess() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 70.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 14.f)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(46))
				.ShadowOffset(FVector2D(2.f, 2.f))
				.ColorAndOpacity(AIBStyle::Text())
				.Text_Lambda([this]()
				{
					const AAIBPlayerState* Me = MyPS();
					if (Me && Me->bSubmitted && PC())
					{
						return FText::Format(LOCTEXT("GuessLocked", "LOCKED: {0} {1}"), AIB::SymbolName(PC()->DraftGuess), FText::FromString(AIB::SymbolGlyph(PC()->DraftGuess)));
					}
					return LOCTEXT("NameSymbol", "NAME YOUR SYMBOL");
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[Cards]
		];
}

TSharedRef<SWidget> SAIBHud::MakeSymbolResults()
{
	SAssignNew(SymbolResultsBox, SVerticalBox);
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const EAIBPhase P = Phase();
			return P == EAIBPhase::Stage2Results || P == EAIBPhase::Stage2Elimination ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 40.f, 0.f))
		[
			SNew(SBox).WidthOverride(520.f)
			[
				AIBUI::Panel(SymbolResultsBox.ToSharedRef(), FMargin(22.f))
			]
		];
}

void SAIBHud::RebuildSymbolResults()
{
	const AAIBGameState* G = GS();
	if (!G || !SymbolResultsBox.IsValid())
	{
		return;
	}
	SymbolResultsBox->ClearChildren();
	SymbolResultsBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(G->Phase == EAIBPhase::Stage2Elimination ? LOCTEXT("FinalStandings", "FINAL STANDINGS") : LOCTEXT("Revealed", "SYMBOLS REVEALED"))
		.Font(AIBStyle::Title(38)).ColorAndOpacity(AIBStyle::Text())
	];
	for (const FAIBSymbolResult& Result : G->Stage2Last)
	{
		const AAIBPlayerState* PS = G->GetPlayerBySlot(Result.Slot);
		if (!PS)
		{
			continue;
		}
		SymbolResultsBox->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(10.f, 6.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(4.f).HeightOverride(34.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(PS->GetCommunityColor())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center).Padding(10.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(PS->GetPlayerName().ToUpper())).Font(AIBStyle::SubHeading(17)).ColorAndOpacity(AIBStyle::Text())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Was", "WAS")).Font(AIBStyle::Body(12)).ColorAndOpacity(AIBStyle::TextFaint())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)[Suit(Result.Actual, 30.f)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 4.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Said", "SAID")).Font(AIBStyle::Body(12)).ColorAndOpacity(AIBStyle::TextFaint())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)[Suit(Result.Guess, 30.f, Result.Guess == EAIBSymbol::None ? 0.4f : 1.f)]
				+ SHorizontalBox::Slot().FillWidth(0.25f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("ResPts", "{0}  {1} PT"), Result.bCorrect ? FText::FromString(TEXT("+1")) : FText::FromString(TEXT("--")), PS->Stage2Points))
					.Font(AIBStyle::Heading(18))
					.ColorAndOpacity(Result.bCorrect ? AIBStyle::Acid() : AIBStyle::Blood())
				]
			]
		];
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Stage 3 - Race
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeRacePanel()
{
	auto MyVehicle = [this]() -> AAIBVehicle* { return PC() ? Cast<AAIBVehicle>(PC()->GetPawn()) : nullptr; };
	auto MyIndex = [this]() { const AAIBGameState* G = GS(); const AAIBPlayerState* Me = MyPS(); return G && Me ? G->Finalists.IndexOfByKey(Me->Slot) : INDEX_NONE; };
	auto InZone = [this, MyVehicle]()
	{
		const AAIBGameState* G = GS();
		const AAIBVehicle* V = MyVehicle();
		return G && V && G->Phase == EAIBPhase::Stage3Race && FVector::Dist2D(V->GetActorLocation(), G->GuardianLocation) < AAIBGuardianSite::SwitchRadius;
	};

	TSharedRef<SVerticalBox> Racers = SNew(SVerticalBox);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Racers->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBox)
			.WidthOverride(380.f)
			.Visibility_Lambda([this, Index]() { const AAIBGameState* G = GS(); return G && G->Finalists.IsValidIndex(Index) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::PanelBrush())
				.Padding(FMargin(12.f, 6.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(STextBlock)
							.Font(AIBStyle::SubHeading(17))
							.Text_Lambda([this, Index]()
							{
								const AAIBGameState* G = GS();
								const AAIBPlayerState* PS = G && G->Finalists.IsValidIndex(Index) ? G->GetPlayerBySlot(G->Finalists[Index]) : nullptr;
								return PS ? FText::FromString(PS->GetPlayerName().ToUpper()) : FText::GetEmpty();
							})
							.ColorAndOpacity_Lambda([this, Index]()
							{
								const AAIBGameState* G = GS();
								return FSlateColor(G && G->Finalists.IsValidIndex(Index) ? AIB::Community(G->Finalists[Index]).Color : AIBStyle::Text());
							})
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Font(AIBStyle::Title(22))
							.ColorAndOpacity(AIBStyle::Text())
							.Text_Lambda([this, Index]()
							{
								const AAIBGameState* G = GS();
								return G && G->RaceDistance.IsValidIndex(Index) ? FText::Format(LOCTEXT("RaceM", "{0} M"), FText::AsNumber(FMath::RoundToInt(G->RaceDistance[Index]))) : FText::GetEmpty();
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						AIBUI::Bar(
							TAttribute<float>::CreateLambda([this, Index]() { const AAIBGameState* G = GS(); return G && G->SwitchProgress.IsValidIndex(Index) ? G->SwitchProgress[Index] : 0.f; }),
							TAttribute<FLinearColor>::CreateLambda([]() { return AIBStyle::Acid(); }), 4.f)
					]
				]
			]
		];
	}

	return SNew(SOverlay)
		.Visibility_Lambda([this]()
		{
			const EAIBPhase P = Phase();
			return P >= EAIBPhase::Stage3Briefing && P <= EAIBPhase::Stage3Finish ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		// Countdown.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Visibility_Lambda([this]()
			{
				const AAIBGameState* G = GS();
				const bool bGo = G && G->Phase == EAIBPhase::Stage3Race && G->GetPhaseElapsed() < 1.2f;
				return (Phase() == EAIBPhase::Stage3Countdown || bGo) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			.Font(AIBStyle::Title(220))
			.ShadowOffset(FVector2D(3.f, 3.f))
			.Text_Lambda([this]()
			{
				const AAIBGameState* G = GS();
				if (!G) return FText::GetEmpty();
				return G->Phase == EAIBPhase::Stage3Race ? LOCTEXT("Go", "GO!") : FText::AsNumber(FMath::CeilToInt(G->GetPhaseRemaining()));
			})
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(Phase() == EAIBPhase::Stage3Race ? AIBStyle::Acid() : AIBStyle::Text()); })
		]
		// Racers (only while racing: the briefing and finish banners own the top of the screen).
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 120.f, 0.f, 0.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { const EAIBPhase P = Phase(); return P == EAIBPhase::Stage3Countdown || P == EAIBPhase::Stage3Race ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				Racers
			]
		]
		// Switch prompt.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0.f, 220.f, 0.f, 0.f)
		[
			SNew(SBox)
			.Visibility_Lambda([InZone]() { return InZone() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.WidthOverride(520.f)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(AIBStyle::Heading(28))
						.ColorAndOpacity(AIBStyle::Acid())
						.Text_Lambda([MyVehicle]()
						{
							const AAIBVehicle* V = MyVehicle();
							return V && V->GetSpeedKmh() > 25.f ? LOCTEXT("SlowDown", "SLOW DOWN - STOP IN THE RING") : LOCTEXT("HoldE", "HOLD [E] TO ACTIVATE THE SWITCH");
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
					[
						AIBUI::Bar(
							TAttribute<float>::CreateLambda([this, MyIndex]() { const AAIBGameState* G = GS(); const int32 I = MyIndex(); return G && G->SwitchProgress.IsValidIndex(I) ? G->SwitchProgress[I] : 0.f; }),
							TAttribute<FLinearColor>::CreateLambda([]() { return AIBStyle::Acid(); }), 10.f)
					],
					FMargin(20.f, 14.f))
			]
		]
		// Speedometer.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 40.f, 70.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([MyVehicle]() { return MyVehicle() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(96))
				.ColorAndOpacity(AIBStyle::Text())
				.ShadowOffset(FVector2D(2.f, 2.f))
				.Text_Lambda([MyVehicle]() { const AAIBVehicle* V = MyVehicle(); return FText::AsNumber(V ? FMath::RoundToInt(V->GetSpeedKmh()) : 0); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(STextBlock).Text(LOCTEXT("Kmh", "KM/H")).Font(AIBStyle::Heading(18)).ColorAndOpacity(AIBStyle::TextDim())
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Final - the Guardian's choice
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeFinalPanel()
{
	auto IsChooser = [this]() { const AAIBGameState* G = GS(); const AAIBPlayerState* Me = MyPS(); return G && Me && G->Phase == EAIBPhase::FinalChoice && G->GetCurrentChooserSlot() == Me->Slot; };
	auto Confirming = [this]() { const AAIBGameState* G = GS(); return G && G->PendingOption != EAIBFinalChoice::None && G->ConfirmEndTime > 0.f; };

	auto OptionCard = [this, Confirming](EAIBFinalChoice Choice)
	{
		const bool bKing = Choice == EAIBFinalChoice::King;
		return SNew(SBox)
			.WidthOverride(420.f)
			[
				SNew(SAIBButton)
				.Kind(bKing ? EAIBButtonKind::Gold : EAIBButtonKind::Secondary)
				.AccentColor(bKing ? FLinearColor::Transparent : FLinearColor(0.55f, 0.65f, 0.8f, 0.9f))
				.Height(190.f)
				.FontSize(34)
				.Text(bKing ? LOCTEXT("BecomeKing", "BECOME THE KING") : LOCTEXT("SavePeople", "SAVE YOUR PEOPLE"))
				.SubText_Lambda([this, bKing]()
				{
					// After the first finalist sacrificed, the second one claims the island for their people (game.md 7.3 C).
					const AAIBGameState* G = GS();
					if (!bKing)
					{
						return LOCTEXT("SaveDesc", "You give your life.\nYour community is carried to safety.");
					}
					return G && G->ChooserIndex > 0
						? LOCTEXT("KingDescSecond", "You survive and claim the island.\nYour community settles on it.")
						: LOCTEXT("KingDesc", "You survive and rule the island alone.\nYour community does not survive.");
				})
				.IsEnabled_Lambda([Confirming]() { return !Confirming(); })
				.OnClicked_Lambda([this, Choice]() { if (PC()) PC()->ServerFinalSelect(Choice); return FReply::Handled(); })
			];
	};

	TSharedRef<SWidget> ConfirmBox = SNew(SBox)
		.WidthOverride(620.f)
		[
			AIBUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Title(46))
					.ColorAndOpacity(AIBStyle::Text())
					.Text_Lambda([this]()
					{
						const AAIBGameState* G = GS();
						return G && G->PendingOption == EAIBFinalChoice::King ? LOCTEXT("ConfirmKing", "BECOME THE KING?") : LOCTEXT("ConfirmSave", "SAVE YOUR PEOPLE?");
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Irreversible", "Once confirmed, this cannot be undone.")).Font(AIBStyle::Body(17)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					AIBUI::Bar(TAttribute<float>::CreateLambda([this]() { const AAIBGameState* G = GS(); return G ? FMath::Clamp((G->ConfirmEndTime - G->GetServerWorldTimeSeconds()) / AIB::FinalConfirm, 0.f, 1.f) : 0.f; }),
						TAttribute<FLinearColor>::CreateLambda([]() { return AIBStyle::Blood(); }), 6.f)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(SAIBButton).Kind(EAIBButtonKind::Primary).Text(LOCTEXT("Confirm", "CONFIRM")).FontSize(24).Height(58.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerFinalConfirm(true); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f, 0.f, 0.f)
					[
						SNew(SAIBButton).Text(LOCTEXT("GoBack", "GO BACK")).FontSize(24).Height(58.f)
						.OnClicked_Lambda([this]() { if (PC()) PC()->ServerFinalConfirm(false); return FReply::Handled(); })
					]
				],
				FMargin(28.f), true)
		];

	return SNew(SOverlay)
		.Visibility_Lambda([this]()
		{
			const EAIBPhase P = Phase();
			return P == EAIBPhase::FinalBriefing || P == EAIBPhase::FinalChoice ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		// Chooser: two cards.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 70.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([IsChooser, Confirming]() { return IsChooser() && !Confirming() ? EVisibility::Visible : EVisibility::Collapsed; })
			+ SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f)[OptionCard(EAIBFinalChoice::King)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f)[OptionCard(EAIBFinalChoice::Sacrifice)]
		]
		// Chooser: confirmation.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility_Lambda([IsChooser, Confirming]() { return IsChooser() && Confirming() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				ConfirmBox
			]
		]
		// Everyone else.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 90.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this, IsChooser]() { return Phase() == EAIBPhase::FinalChoice && !IsChooser() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				AIBUI::Panel(
					SNew(STextBlock)
					.Font(AIBStyle::Heading(28))
					.ColorAndOpacity(AIBStyle::Text())
					.Text_Lambda([this]()
					{
						const AAIBGameState* G = GS();
						const AAIBPlayerState* Chooser = G ? G->GetPlayerBySlot(G->GetCurrentChooserSlot()) : nullptr;
						return Chooser ? FText::Format(LOCTEXT("IsChoosing", "{0} IS CHOOSING..."), FText::FromString(Chooser->GetPlayerName().ToUpper())) : FText::GetEmpty();
					}),
					FMargin(26.f, 14.f))
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeResultsPanel()
{
	SAssignNew(ResultsBox, SVerticalBox);
	return SNew(SBox)
		.Visibility_Lambda([this]() { return Phase() == EAIBPhase::Results ? EVisibility::Visible : EVisibility::Collapsed; })
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 30.f))
		[
			SNew(SBox).WidthOverride(1060.f)
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::PanelSolidBrush())
				.Padding(FMargin(1.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ResultsBox.ToSharedRef()]
					+ SVerticalBox::Slot().AutoHeight().Padding(30.f, 18.f, 30.f, 26.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SAIBButton)
							.Kind(EAIBButtonKind::Primary)
							.Text(LOCTEXT("PlayAgain", "BACK TO THE ROOM"))
							.SubText_Lambda([this]() { return PC() && PC()->IsHost() ? LOCTEXT("PlayAgainSub", "Everyone returns to the gathering") : LOCTEXT("HostDecides", "Waiting for the host"); })
							.IsEnabled_Lambda([this]() { return PC() && PC()->IsHost(); })
							.OnClicked_Lambda([this]() { if (PC()) PC()->ServerReturnToLobby(); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f, 0.f, 0.f)
						[
							SNew(SAIBButton)
							.Text(LOCTEXT("MainMenu", "MAIN MENU"))
							.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
						]
					]
				]
			]
		];
}

void SAIBHud::RebuildResults()
{
	const AAIBGameState* G = GS();
	if (!G || !ResultsBox.IsValid())
	{
		return;
	}
	ResultsBox->ClearChildren();

	const AAIBPlayerState* Champion = G->GetPlayerBySlot(G->ChampionSlot);
	const FText ChampionName = FText::FromString(Champion ? Champion->GetPlayerName().ToUpper() : FString());
	FText Headline;
	FName Picture = TEXT("T4_Choice");
	switch (G->Ending)
	{
	case EAIBEnding::KingFirst:
	case EAIBEnding::SoloKing:
		Headline = FText::Format(LOCTEXT("HKing", "{0} claimed the island - alone. Their people were left to the sea."), ChampionName);
		break;
	case EAIBEnding::SacrificeThenKing:
		Headline = FText::Format(LOCTEXT("HSacKing", "{0} claimed the island for {1}."), ChampionName, Champion ? Champion->GetCommunityName() : FText::GetEmpty());
		Picture = TEXT("S3_Island");
		break;
	case EAIBEnding::DoubleSacrifice:
		Headline = LOCTEXT("HDouble", "Both finalists gave their lives. The island remains unclaimed.");
		Picture = TEXT("S2_Fleets");
		break;
	case EAIBEnding::SoloSacrifice:
		Headline = LOCTEXT("HSolo", "The last champion gave their life. The island remains unclaimed.");
		Picture = TEXT("S2_Fleets");
		break;
	default:
		Headline = LOCTEXT("HNone", "No champion remained to stand before the Guardian.");
		Picture = TEXT("S1_DyingEarth");
		break;
	}
	const FText Title = AIB::EndingTitle(G->Ending).IsEmpty() ? LOCTEXT("TheEnd", "THE END") : AIB::EndingTitle(G->Ending);
	const FLinearColor PanelColor(0.012f, 0.012f, 0.016f, 0.97f);

	// Header: the ending over its picture.
	ResultsBox->AddSlot().AutoHeight()
	[
		SNew(SBox)
		.HeightOverride(230.f)
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				[
					SNew(SImage).Image(AIBStyle::StoryImage(Picture))
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SAIBGradient).Colors({ FLinearColor(PanelColor.R, PanelColor.G, PanelColor.B, 0.15f), FLinearColor(PanelColor.R, PanelColor.G, PanelColor.B, 0.55f), PanelColor })
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(30.f, 0.f, 30.f, 14.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("TrialsOver", "THE TRIALS ARE OVER")).Font(AIBStyle::SubHeading(16)).ColorAndOpacity(AIBStyle::TextDim())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Title).Font(AIBStyle::Title(68)).ColorAndOpacity(AIBStyle::Gold()).ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Headline).Font(AIBStyle::BodyMedium(20)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
				]
			]
		]
	];

	// Leaderboard: how far each champion got, best first.
	auto Rank = [](const AAIBPlayerState* PS)
	{
		switch (PS->Status)
		{
		case EAIBPlayerStatus::Champion: return 0;
		case EAIBPlayerStatus::Sacrificed: return 1;
		case EAIBPlayerStatus::Active: return 2;
		case EAIBPlayerStatus::Eliminated: return 5 - FMath::Clamp(PS->EliminatedInStage, 0, 2);
		default: return 6;
		}
	};
	TArray<AAIBPlayerState*> Players = G->GetSeatedPlayers();
	Players.Sort([&Rank](const AAIBPlayerState& A, const AAIBPlayerState& B)
	{
		if (Rank(&A) != Rank(&B)) return Rank(&A) < Rank(&B);
		if (A.Stage2Points != B.Stage2Points) return A.Stage2Points > B.Stage2Points;
		return A.Stage1Wins > B.Stage1Wins;
	});

	auto Fate = [G](const AAIBPlayerState* PS) -> FText
	{
		switch (PS->Status)
		{
		case EAIBPlayerStatus::Champion:
			return G->Ending == EAIBEnding::SacrificeThenKing ? LOCTEXT("FateKingPeople", "Became King - their people settle the island") : LOCTEXT("FateKingAlone", "Became King - their people are lost");
		case EAIBPlayerStatus::Sacrificed: return LOCTEXT("FateSacrificed", "Gave their life - their people are safe");
		case EAIBPlayerStatus::Active: return LOCTEXT("FateFinalist", "Reached the Guardian");
		case EAIBPlayerStatus::Eliminated: return PS->EliminatedInStage >= 2 ? LOCTEXT("FateOut2", "Out in Trial II - The Hidden Symbol") : LOCTEXT("FateOut1", "Out in Trial I - The Number Game");
		default: return LOCTEXT("FateForfeit", "Left the trials");
		}
	};
	auto FateColor = [](const AAIBPlayerState* PS)
	{
		switch (PS->Status)
		{
		case EAIBPlayerStatus::Champion: return AIBStyle::Gold();
		case EAIBPlayerStatus::Sacrificed: return AIBStyle::Ice();
		case EAIBPlayerStatus::Active: return AIBStyle::Text();
		default: return AIBStyle::TextDim();
		}
	};

	TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
	auto HeaderCell = [](const FText& Text) { return SNew(STextBlock).Text(Text).Font(AIBStyle::SubHeading(13)).ColorAndOpacity(AIBStyle::TextFaint()); };
	Table->AddSlot().AutoHeight().Padding(14.f, 0.f, 14.f, 6.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(84.f)[HeaderCell(LOCTEXT("ColRank", "#"))]]
		+ SHorizontalBox::Slot().FillWidth(0.36f)[HeaderCell(LOCTEXT("ColChampion", "CHAMPION"))]
		+ SHorizontalBox::Slot().FillWidth(0.4f)[HeaderCell(LOCTEXT("ColFate", "FATE"))]
		+ SHorizontalBox::Slot().FillWidth(0.24f).HAlign(HAlign_Right)[HeaderCell(LOCTEXT("ColTrials", "WINS  /  POINTS  /  STONES"))]
	];
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		const AAIBPlayerState* PS = Players[Index];
		const bool bMe = PS == MyPS();
		const FLinearColor Accent = FateColor(PS);
		Table->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(bMe ? AIBStyle::OutlineBrush(AIBStyle::Gold() * FLinearColor(1.f, 1.f, 1.f, 0.5f), 8.f, 1.5f) : AIBStyle::CardBrush())
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(0.f))
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::CardBrush())
				.Padding(FMargin(14.f, 8.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(36.f)
						[
							SNew(STextBlock).Text(FText::AsNumber(Index + 1)).Font(AIBStyle::Title(34)).ColorAndOpacity(Index == 0 && PS->Status == EAIBPlayerStatus::Champion ? AIBStyle::Gold() : AIBStyle::TextDim())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
					[
						SNew(SBox).WidthOverride(34.f).HeightOverride(34.f)
						[
							SNew(SImage).Image(AIBStyle::CrestBrush(PS->Slot)).ColorAndOpacity(PS->GetCommunityColor())
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.36f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(PS->GetPlayerName().ToUpper() + (bMe ? TEXT("  (YOU)") : TEXT("")) + (PS->bBot ? TEXT("  - AI") : TEXT(""))))
							.Font(AIBStyle::Heading(21))
							.ColorAndOpacity(PS->Status == EAIBPlayerStatus::Champion ? AIBStyle::Gold() : AIBStyle::Text())
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(PS->GetCommunityName()).Font(AIBStyle::Body(13)).ColorAndOpacity(PS->GetCommunityColor())
						]
					]
					+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
					[
						SNew(STextBlock).Text(Fate(PS)).Font(AIBStyle::BodyMedium(16)).ColorAndOpacity(Accent).AutoWrapText(true)
					]
					+ SHorizontalBox::Slot().FillWidth(0.24f).VAlign(VAlign_Center).HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("StatsLine", "{0}   /   {1}   /   {2}"), PS->Stage1Wins, PS->Stage2Points, int32(FMath::CountBits64(uint64(PS->LoreFound)))))
						.Font(AIBStyle::Heading(19))
						.ColorAndOpacity(AIBStyle::TextDim())
					]
				]
			]
		];
	}
	ResultsBox->AddSlot().AutoHeight().Padding(30.f, 8.f, 30.f, 0.f)[Table];
	ResultsBox->AddSlot().AutoHeight().Padding(44.f, 6.f, 30.f, 0.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("StatsLegend", "Wins: rounds won in the Number Game.   Points: correct Hidden Symbol guesses.   Stones: marker stones read on the island."))
		.Font(AIBStyle::Body(12))
		.ColorAndOpacity(AIBStyle::TextFaint())
		.AutoWrapText(true)
	];
}

#undef LOCTEXT_NAMESPACE
