// Alice in Botherland - in-match HUD: frame, overlays, chat, menus

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBLoadingScreen.h"
#include "UI/SAIBStoryCards.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBCharacter.h"
#include "Player/AIBVehicle.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Core/AIBGameInstance.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Audio/AIBNarration.h"
#include "World/AIBLoreStones.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBHud"

// ---------------------------------------------------------------------------------------------------------------------
// World overlay
// ---------------------------------------------------------------------------------------------------------------------

void SAIBWorldOverlay::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SAIBWorldOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	AAIBPlayerController* PC = Owner.Get();
	const AAIBGameState* GS = PC ? PC->GetAIBGameState() : nullptr;
	if (!PC || !GS)
	{
		return LayerId;
	}
	const float Scale = AllottedGeometry.Scale;
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo NameFont = AIBStyle::SubHeading(15);
	const bool bShowNames = GS->Phase == EAIBPhase::IslandExplore || GS->Phase == EAIBPhase::Stage2Communication || GS->Phase == EAIBPhase::Stage2Guess
		|| GS->Phase == EAIBPhase::Stage2Results || (GS->Phase >= EAIBPhase::Stage1RoundIntro && GS->Phase <= EAIBPhase::Stage1Results);
	const FVector CamLoc = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;

	auto DrawText = [&](const FText& Text, const FSlateFontInfo& Font, const FVector2D& Center, const FLinearColor& Color, int32 Layer)
	{
		const FVector2D Size = Measure->Measure(Text, Font);
		// Shadow then text.
		FSlateDrawElement::MakeText(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Center - Size * 0.5f + FVector2D(1.5f, 1.5f)))), Text, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f));
		FSlateDrawElement::MakeText(OutDrawElements, Layer + 1, AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Center - Size * 0.5f))), Text, Font, ESlateDrawEffect::None, Color);
	};

	if (bShowNames)
	{
		for (TActorIterator<AAIBCharacter> It(PC->GetWorld()); It; ++It)
		{
			AAIBCharacter* Character = *It;
			const AAIBPlayerState* PS = Character->GetChampionState();
			if (!PS || Character == PC->GetPawn() || Character->IsHidden() || !PS->IsActiveChampion())
			{
				continue;
			}
			const FVector Head = Character->GetActorLocation() + FVector(0.f, 0.f, 118.f);
			const float Distance = FVector::Dist(Head, CamLoc);
			if (Distance > 6000.f)
			{
				continue;
			}
			FVector2D Screen;
			if (!PC->ProjectWorldLocationToScreen(Head, Screen, true))
			{
				continue;
			}
			const FVector2D Local = Screen / Scale;
			const float Alpha = FMath::Clamp(1.f - (Distance - 3500.f) / 2500.f, 0.f, 1.f);
			FLinearColor Color = PS->GetCommunityColor();
			Color.A = Alpha;
			DrawText(FText::FromString(PS->GetPlayerName().ToUpper()), NameFont, Local, FLinearColor(1.f, 1.f, 1.f, Alpha), LayerId);
			// Community underline.
			const TArray<FVector2D> Line = { Local + FVector2D(-22.f, 12.f), Local + FVector2D(22.f, 12.f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Color, true, 3.f);
		}
	}

	// Guardian waypoint while racing.
	if (GS->Phase == EAIBPhase::Stage3Race || GS->Phase == EAIBPhase::Stage3Countdown || GS->Phase == EAIBPhase::Stage3Briefing)
	{
		const FVector Target = GS->GuardianLocation + FVector(0.f, 0.f, 300.f);
		FVector2D Screen;
		bool bOnScreen = PC->ProjectWorldLocationToScreen(Target, Screen, true);
		FVector2D Local = Screen / Scale;
		const FVector ToTarget = (Target - CamLoc).GetSafeNormal();
		const FVector Forward = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector;
		const bool bBehind = FVector::DotProduct(ToTarget, Forward) < 0.f;
		const float Margin = 60.f;
		if (!bOnScreen || bBehind || Local.X < Margin || Local.Y < Margin || Local.X > LocalSize.X - Margin || Local.Y > LocalSize.Y - Margin)
		{
			// Clamp to the screen edge in the target's direction.
			FVector2D Dir = (Local - LocalSize * 0.5f);
			if (bBehind)
			{
				Dir = -Dir;
			}
			Dir = Dir.GetSafeNormal();
			const FVector2D Half = LocalSize * 0.5f - FVector2D(Margin);
			const float T = FMath::Min(Half.X / FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(Dir.X)), Half.Y / FMath::Max(KINDA_SMALL_NUMBER, FMath::Abs(Dir.Y)));
			Local = LocalSize * 0.5f + Dir * T;
		}
		const float Distance = FVector::Dist(GS->GuardianLocation, CamLoc) / 100.f;
		const FLinearColor Green = AIBStyle::Acid();
		const float Pulse = 0.75f + 0.25f * FMath::Sin(FPlatformTime::Seconds() * 4.0);
		TArray<FVector2D> Diamond = { Local + FVector2D(0.f, -14.f), Local + FVector2D(12.f, 0.f), Local + FVector2D(0.f, 14.f), Local + FVector2D(-12.f, 0.f), Local + FVector2D(0.f, -14.f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Diamond, ESlateDrawEffect::None, Green * Pulse, true, 3.f);
		DrawText(FText::Format(LOCTEXT("GuardianMarker", "GUARDIAN  {0} m"), FText::AsNumber(FMath::RoundToInt(Distance))), AIBStyle::SubHeading(16), Local + FVector2D(0.f, 28.f), Green, LayerId + 3);
	}
	return LayerId + 6;
}

// ---------------------------------------------------------------------------------------------------------------------
// HUD root
// ---------------------------------------------------------------------------------------------------------------------

AAIBPlayerController* SAIBHud::PC() const { return Owner.Get(); }
AAIBGameState* SAIBHud::GS() const { return Owner.IsValid() ? Owner->GetAIBGameState() : nullptr; }
AAIBPlayerState* SAIBHud::MyPS() const { return Owner.IsValid() ? Owner->GetAIBPlayerState() : nullptr; }
EAIBPhase SAIBHud::Phase() const { const AAIBGameState* G = GS(); return G ? G->Phase : EAIBPhase::None; }
float SAIBHud::ServerNow() const { const AAIBGameState* G = GS(); return G ? G->GetServerWorldTimeSeconds() : 0.f; }

void SAIBHud::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;

	auto FadeAlpha = [this]() -> FSlateColor
	{
		const AAIBGameState* G = GS();
		if (!G)
		{
			return FLinearColor(0.f, 0.f, 0.f, 1.f);
		}
		const float Now = G->GetServerWorldTimeSeconds();
		float Alpha = 0.f;
		if (G->FadeOutStartTime >= 0.f && Now >= G->FadeOutStartTime)
		{
			Alpha = FMath::Clamp((Now - G->FadeOutStartTime) / FMath::Max(0.01f, G->FadeOutEndTime - G->FadeOutStartTime), 0.f, 1.f);
		}
		else
		{
			switch (G->Phase)
			{
			case EAIBPhase::None:
				Alpha = 1.f;
				break;
			case EAIBPhase::Lobby:
			case EAIBPhase::StoryIntro:
			case EAIBPhase::ArrivalCinematic:
			case EAIBPhase::IslandExplore:
			case EAIBPhase::Stage1Briefing:
			case EAIBPhase::Stage2Briefing:
			case EAIBPhase::Stage3Briefing:
			case EAIBPhase::FinalBriefing:
			case EAIBPhase::EndingCinematic:
				Alpha = 1.f - FMath::Clamp((Now - G->PhaseStartTime) / 1.2f, 0.f, 1.f);
				break;
			default:
				break;
			}
		}
		return FLinearColor(0.f, 0.f, 0.f, Alpha);
	};

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAIBWorldOverlay).Owner(Owner)
		]
		+ SOverlay::Slot()[MakeLetterbox()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 22.f, 0.f, 0.f)[MakeTopBar()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 150.f, 0.f, 0.f)[MakeBanner()]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 120.f, 0.f, 0.f)[MakeRoster()]
		+ SOverlay::Slot()[MakeLobbyPanel()]
		+ SOverlay::Slot()[MakeNumberPanel()]
		+ SOverlay::Slot()[MakeNumberResults()]
		+ SOverlay::Slot()[MakeSymbolPanel()]
		+ SOverlay::Slot()[MakeGuessPanel()]
		+ SOverlay::Slot()[MakeSymbolResults()]
		+ SOverlay::Slot()[MakeRacePanel()]
		+ SOverlay::Slot()[MakeFinalPanel()]
		+ SOverlay::Slot()[MakeResultsPanel()]
		+ SOverlay::Slot()[MakeExplorePanel()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 40.f, 36.f, 0.f)[MakeRulesCard()]
		+ SOverlay::Slot()[MakeStoryLayer()]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom)
		.Padding(TAttribute<FMargin>::CreateLambda([this]()
		{
			// In the lobby the Gathering panel fills the left column, so the chat moves beside it.
			const EAIBPhase P = Phase();
			const bool bLobby = P == EAIBPhase::Lobby || P == EAIBPhase::LobbyCountdown;
			return FMargin(bLobby ? 700.f : 24.f, 0.f, 0.f, 24.f);
		}))
		[MakeChat()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
		.Padding(TAttribute<FMargin>::CreateLambda([this]()
		{
			const EAIBPhase P = Phase();
			const bool bCardsBelow = P == EAIBPhase::Stage2Guess || P == EAIBPhase::FinalChoice;
			const bool bPanelRight = P >= EAIBPhase::Stage1RoundIntro && P <= EAIBPhase::Stage1Elimination;
			return FMargin(0.f, 0.f, bPanelRight ? 640.f : 0.f, bCardsBelow ? 400.f : 96.f);
		}))
		[MakeSubtitles()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
		.Padding(TAttribute<FMargin>::CreateLambda([this]() { return FMargin(0.f, Phase() == EAIBPhase::IslandExplore ? 160.f : 130.f, 28.f, 0.f); }))
		[MakeToasts()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 28.f, 24.f)[MakeHints()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 36.f)[MakeSpectatorBar()]
		+ SOverlay::Slot()[MakeScoreboard()]
		+ SOverlay::Slot()[MakePauseMenu()]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(AIBStyle::WhiteBrush())
			.BorderBackgroundColor_Lambda(FadeAlpha)
		]
	];
}

void SAIBHud::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const AAIBGameState* G = GS();
	if (!G)
	{
		return;
	}
	if (G->Phase == EAIBPhase::Results && BuiltResultsSerial != G->PhaseSerial)
	{
		BuiltResultsSerial = G->PhaseSerial;
		RebuildResults();
	}
	if ((G->Phase == EAIBPhase::Stage2Communication || G->Phase == EAIBPhase::Stage2Guess) && BuiltSymbolSerial != G->PhaseSerial)
	{
		BuiltSymbolSerial = G->PhaseSerial;
		RebuildSymbolRows();
	}
	if ((G->Phase == EAIBPhase::Stage1Results || G->Phase == EAIBPhase::Stage1Elimination || G->Phase == EAIBPhase::Stage1Reveal) && BuiltNumberSerial != G->PhaseSerial)
	{
		BuiltNumberSerial = G->PhaseSerial;
		RebuildNumberResults();
	}
	if ((G->Phase == EAIBPhase::Stage2Results || G->Phase == EAIBPhase::Stage2Elimination) && BuiltSymbolResultsSerial != G->PhaseSerial)
	{
		BuiltSymbolResultsSerial = G->PhaseSerial;
		RebuildSymbolResults();
	}
	if (bChatOpen && ChatInput.IsValid() && !ChatInput->HasKeyboardFocus())
	{
		FSlateApplication::Get().SetKeyboardFocus(ChatInput, EFocusCause::SetDirectly);
	}
}

FReply SAIBHud::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (bChatOpen)
		{
			CloseChat();
			return FReply::Handled();
		}
		TogglePause();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

bool SAIBHud::WantsCursor() const
{
	if (bChatOpen || bPauseOpen)
	{
		return true;
	}
	const AAIBGameState* G = GS();
	const AAIBPlayerState* Me = MyPS();
	if (!G)
	{
		return false;
	}
	switch (G->Phase)
	{
	case EAIBPhase::Lobby:
	case EAIBPhase::LobbyCountdown:
	case EAIBPhase::Results:
		return true;
	case EAIBPhase::Stage1RoundIntro:
	case EAIBPhase::Stage1Selection:
		return Me && G->Stage1Participants.Contains(Me->Slot) && !Me->bSubmitted;
	case EAIBPhase::Stage2Guess:
		return Me && G->Stage2Participants.Contains(Me->Slot) && !Me->bSubmitted;
	case EAIBPhase::FinalChoice:
		return Me && G->GetCurrentChooserSlot() == Me->Slot;
	default:
		return false;
	}
}

bool SAIBHud::BlocksGameInput() const
{
	return bChatOpen || bPauseOpen;
}

void SAIBHud::OpenChat()
{
	bChatOpen = true;
	LastChatTime = FPlatformTime::Seconds();
	if (ChatInput.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(ChatInput, EFocusCause::SetDirectly);
	}
}

void SAIBHud::CloseChat()
{
	bChatOpen = false;
	if (ChatInput.IsValid())
	{
		ChatInput->SetText(FText::GetEmpty());
	}
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void SAIBHud::OpenSettings()
{
	bPauseOpen = true;
	bSettingsOpen = true;
	bSettingsOnly = true;
	bRulesOpen = false;
	AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
}

void SAIBHud::OpenRules()
{
	bPauseOpen = true;
	bRulesOpen = true;
	bSettingsOpen = false;
	bSettingsOnly = true;
	AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
}

void SAIBHud::TogglePause()
{
	bPauseOpen = !bPauseOpen;
	bSettingsOpen = false;
	bSettingsOnly = false;
	bRulesOpen = false;
	AIBStyle::PlayUISound(bPauseOpen ? TEXT("SFX_UI_Click") : TEXT("SFX_UI_Back"));
	if (!bPauseOpen)
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Top bar, banner, letterbox
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeTopBar()
{
	auto IsVisible = [this]()
	{
		const AAIBGameState* G = GS();
		if (!G || G->PhaseEndTime <= G->PhaseStartTime + 0.5f)
		{
			return EVisibility::Collapsed;
		}
		switch (G->Phase)
		{
		case EAIBPhase::Lobby:
		case EAIBPhase::LobbyCountdown:
		case EAIBPhase::StoryIntro:
		case EAIBPhase::ArrivalCinematic:
		case EAIBPhase::EndingCinematic:
		case EAIBPhase::Results:
		case EAIBPhase::Stage3Countdown:
			return EVisibility::Collapsed;
		default:
			return EVisibility::HitTestInvisible;
		}
	};
	return SNew(SBox)
		.Visibility_Lambda(IsVisible)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SAIBTimerRing)
				.Size(78.f)
				.Fraction_Lambda([this]()
				{
					const AAIBGameState* G = GS();
					return G ? 1.f - G->GetPhaseAlpha() : 0.f;
				})
				.Color_Lambda([this]()
				{
					const AAIBGameState* G = GS();
					const float Remaining = G ? G->GetPhaseRemaining() : 0.f;
					return Remaining <= 5.f ? AIBStyle::Blood() : AIBStyle::Text();
				})
				.Label_Lambda([this]()
				{
					const AAIBGameState* G = GS();
					return FText::AsNumber(G ? FMath::CeilToInt(G->GetPhaseRemaining()) : 0);
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Font(AIBStyle::Heading(26))
					.ColorAndOpacity(AIBStyle::Text())
					.ShadowOffset(FVector2D(1.f, 1.f))
					.Text_Lambda([this]() { return AIB::PhaseTitle(Phase()); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Font(AIBStyle::Body(15))
					.ColorAndOpacity(AIBStyle::TextDim())
					.ShadowOffset(FVector2D(1.f, 1.f))
					.Text_Lambda([this]()
					{
						const AAIBGameState* G = GS();
						if (!G)
						{
							return FText::GetEmpty();
						}
						switch (G->Phase)
						{
						case EAIBPhase::Stage1RoundIntro:
						case EAIBPhase::Stage1Selection:
							return G->bStage1SuddenDeath ? LOCTEXT("SubSD", "Sudden death between the tied champions") : FText::Format(LOCTEXT("SubRound", "Round {0}"), G->Stage1Round);
						case EAIBPhase::Stage2Communication:
						case EAIBPhase::Stage2Guess:
							return G->bStage2SuddenDeath ? LOCTEXT("SubSD2", "Sudden death round") : FText::Format(LOCTEXT("SubRound2", "Symbol round {0} of 3"), G->Stage2Round);
						case EAIBPhase::Stage3Race:
							return G->bRaceSuddenDeath ? LOCTEXT("SubSD3", "Sudden death race") : LOCTEXT("SubRace", "First to hold the switch for 3 seconds");
						case EAIBPhase::FinalChoice:
						{
							const AAIBPlayerState* Chooser = G->GetPlayerBySlot(G->GetCurrentChooserSlot());
							return Chooser ? FText::Format(LOCTEXT("SubChooser", "{0} decides"), FText::FromString(Chooser->GetPlayerName())) : FText::GetEmpty();
						}
						default:
							return FText::GetEmpty();
						}
					})
				]
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeBanner()
{
	auto Alpha = [this]()
	{
		const AAIBGameState* G = GS();
		if (!G)
		{
			return 0.f;
		}
		const float T = G->GetServerWorldTimeSeconds() - G->BannerTime;
		if (T < 0.f || T > 6.f)
		{
			return 0.f;
		}
		return FMath::Min(FMath::Clamp(T / 0.35f, 0.f, 1.f), FMath::Clamp((6.f - T) / 0.8f, 0.f, 1.f));
	};
	return SNew(SBox)
		.Visibility_Lambda([Alpha]() { return Alpha() > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		.MaxDesiredWidth(1300.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(72))
				.Justification(ETextJustify::Center)
				.WrapTextAt(1250.f)
				.ShadowOffset(FVector2D(2.f, 2.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
				.Text_Lambda([this]() { const AAIBGameState* G = GS(); return G ? G->BannerTitle : FText::GetEmpty(); })
				.ColorAndOpacity_Lambda([this, Alpha]()
				{
					const AAIBGameState* G = GS();
					FLinearColor C = G ? G->BannerColor : FLinearColor::White;
					C.A = Alpha();
					return FSlateColor(C);
				})
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(AIBStyle::BodyMedium(21))
				.Justification(ETextJustify::Center)
				.WrapTextAt(1100.f)
				.ShadowOffset(FVector2D(1.f, 1.f))
				.Text_Lambda([this]() { const AAIBGameState* G = GS(); return G ? G->BannerSubtitle : FText::GetEmpty(); })
				.ColorAndOpacity_Lambda([Alpha]() { return FSlateColor(FLinearColor(0.92f, 0.9f, 0.86f, Alpha())); })
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeLetterbox()
{
	auto Amount = [this]()
	{
		const AAIBGameState* G = GS();
		if (!G)
		{
			return 0.f;
		}
		const bool bCinematic = G->Phase == EAIBPhase::ArrivalCinematic || G->Phase == EAIBPhase::EndingCinematic || G->Phase == EAIBPhase::Stage3Finish;
		return bCinematic ? FMath::Clamp(G->GetPhaseElapsed() / 0.8f, 0.f, 1.f) : 0.f;
	};
	auto Bar = [Amount]()
	{
		return SNew(SBox)
			.HeightOverride_Lambda([Amount]() { return FOptionalSize(Amount() * 90.f); })
			[
				SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)
			];
	};
	return SNew(SVerticalBox)
		.Visibility(EVisibility::HitTestInvisible)
		+ SVerticalBox::Slot().AutoHeight()[Bar()]
		+ SVerticalBox::Slot().FillHeight(1.f)[SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight()[Bar()];
}

// ---------------------------------------------------------------------------------------------------------------------
// Roster
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeSeatRow(int32 Slot)
{
	auto Seat = [this, Slot]() -> AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
	return SNew(SBox)
		.WidthOverride(270.f)
		.Visibility_Lambda([Seat]() { return Seat() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.BorderBackgroundColor_Lambda([Seat]()
			{
				const AAIBPlayerState* PS = Seat();
				return FSlateColor(PS && PS->IsActiveChampion() ? FLinearColor(1.f, 1.f, 1.f, 0.9f) : FLinearColor(1.f, 1.f, 1.f, 0.45f));
			})
			.Padding(FMargin(0.f, 6.f, 10.f, 6.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(4.f)
					[
						SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIB::Community(Slot).Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(10.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Font(AIBStyle::SubHeading(17))
						.Text_Lambda([this, Seat]()
						{
							const AAIBPlayerState* PS = Seat();
							if (!PS)
							{
								return FText::GetEmpty();
							}
							const bool bMe = PS == MyPS();
							return FText::FromString(PS->GetPlayerName().ToUpper() + (bMe ? TEXT("  (YOU)") : TEXT("")));
						})
						.ColorAndOpacity_Lambda([Seat]()
						{
							const AAIBPlayerState* PS = Seat();
							return FSlateColor(PS && PS->IsActiveChampion() ? AIBStyle::Text() : AIBStyle::TextFaint());
						})
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Font(AIBStyle::Body(12))
						.ColorAndOpacity(AIBStyle::TextDim())
						.Text_Lambda([this, Seat]()
						{
							const AAIBPlayerState* PS = Seat();
							if (!PS)
							{
								return FText::GetEmpty();
							}
							if (PS->bDisconnected)
							{
								return FText::Format(LOCTEXT("Reconnecting", "RECONNECTING {0}"), AIBStyle::Clock(PS->ReconnectDeadline - ServerNow()));
							}
							switch (PS->Status)
							{
							case EAIBPlayerStatus::Eliminated: return FText::Format(LOCTEXT("ElimIn", "ELIMINATED - STAGE {0}"), PS->EliminatedInStage);
							case EAIBPlayerStatus::Forfeited: return LOCTEXT("Forfeited", "FORFEITED");
							case EAIBPlayerStatus::Sacrificed: return LOCTEXT("Sacrificed", "SACRIFICED");
							case EAIBPlayerStatus::Champion: return LOCTEXT("Champion", "CHAMPION");
							default: break;
							}
							return PS->GetCommunityName();
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					AIBUI::TalkDot(TAttribute<bool>::CreateLambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bTalking; }))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Title(24))
					.Text_Lambda([this, Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						const AAIBGameState* G = GS();
						if (!PS || !G)
						{
							return FText::GetEmpty();
						}
						if ((G->Phase == EAIBPhase::Stage1Selection && G->Stage1Participants.Contains(PS->Slot))
							|| (G->Phase == EAIBPhase::Stage2Guess && G->Stage2Participants.Contains(PS->Slot)))
						{
							return PS->bSubmitted ? FText::FromString(TEXT("LOCKED")) : FText::FromString(TEXT("..."));
						}
						if (AIB::ZoneForPhase(G->Phase) == EAIBSceneZone::Hotel && PS->IsActiveChampion())
						{
							return FText::Format(LOCTEXT("Pts", "{0} PT"), PS->Stage2Points);
						}
						if (G->Phase >= EAIBPhase::Stage3Briefing && G->Phase <= EAIBPhase::Stage3Finish)
						{
							const int32 Index = G->Finalists.IndexOfByKey(PS->Slot);
							if (G->RaceDistance.IsValidIndex(Index))
							{
								return FText::Format(LOCTEXT("Dist", "{0} M"), FText::AsNumber(FMath::RoundToInt(G->RaceDistance[Index])));
							}
						}
						return FText::GetEmpty();
					})
					.ColorAndOpacity_Lambda([Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						return FSlateColor(PS && PS->bSubmitted ? AIBStyle::Acid() : AIBStyle::Gold());
					})
				]
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeRoster()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		Box->AddSlot().AutoHeight()[MakeSeatRow(Slot)];
	}
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			if (!G || !G->bMatchInProgress || bScoreboard)
			{
				return EVisibility::Collapsed;
			}
			switch (G->Phase)
			{
			case EAIBPhase::StoryIntro:
			case EAIBPhase::ArrivalCinematic:
			case EAIBPhase::EndingCinematic:
			case EAIBPhase::Results:
			case EAIBPhase::LobbyCountdown:
				return EVisibility::Collapsed;
			default:
				return EVisibility::HitTestInvisible;
			}
		})
		[
			Box
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Chat
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeChat()
{
	SAssignNew(ChatList, SVerticalBox);
	return SNew(SBox)
		.WidthOverride(460.f)
		.Visibility_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			if (!G || G->Phase == EAIBPhase::Results || bPauseOpen)
			{
				return bChatOpen ? EVisibility::Visible : EVisibility::Collapsed;
			}
			const bool bAlways = G->Phase == EAIBPhase::Stage2Communication || G->Phase == EAIBPhase::Stage2Guess;
			const bool bRecent = FPlatformTime::Seconds() - LastChatTime < 10.0;
			if (bChatOpen)
			{
				return EVisibility::Visible;
			}
			// Never an empty box over the scene (e.g. the lobby notices are cleared when the story starts).
			if (!ChatList.IsValid() || ChatList->NumSlots() == 0)
			{
				return EVisibility::Collapsed;
			}
			return (bAlways || bRecent) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::PanelBrush())
				.BorderBackgroundColor_Lambda([this]() { return FSlateColor(FLinearColor(1.f, 1.f, 1.f, bChatOpen ? 1.f : 0.55f)); })
				.Padding(FMargin(14.f, 10.f))
				[
					SNew(SBox).HeightOverride(210.f)
					[
						SAssignNew(ChatScroll, SScrollBox)
						.ScrollBarVisibility(EVisibility::Collapsed)
						+ SScrollBox::Slot()[ChatList.ToSharedRef()]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bChatOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SAssignNew(ChatInput, SEditableTextBox)
					.Font(AIBStyle::Body(17))
					.HintText(LOCTEXT("ChatHint", "Say something...  (Enter to send, Esc to close)"))
					.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit)
					{
						if (Commit == ETextCommit::OnEnter)
						{
							if (AAIBPlayerController* Controller = PC())
							{
								Controller->SendChatMessage(Text.ToString());
							}
							ChatInput->SetText(FText::GetEmpty());
							CloseChat();
						}
						else if (Commit == ETextCommit::OnCleared)
						{
							CloseChat();
						}
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([this]() { return bChatOpen ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				.Font(AIBStyle::Body(13))
				.ColorAndOpacity(AIBStyle::TextFaint())
				.Text(LOCTEXT("ChatOpenHint", "[ENTER] chat"))
			]
		];
}

void SAIBHud::OnChatMessage(const FAIBChatMessage& Message)
{
	LastChatTime = FPlatformTime::Seconds();
	if (!ChatList.IsValid())
	{
		return;
	}
	const AAIBGameState* G = GS();
	FLinearColor NameColor = AIBStyle::TextDim();
	if (Message.FromSlot != INDEX_NONE)
	{
		NameColor = AIB::Community(Message.FromSlot).Color;
	}
	FString Body = Message.Text;
	if (Message.ClaimSymbol != EAIBSymbol::None && !Body.Contains(AIB::SymbolGlyph(Message.ClaimSymbol)))
	{
		Body += TEXT("  ") + AIB::SymbolGlyph(Message.ClaimSymbol);
	}
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (Message.bSystem)
	{
		Row->AddSlot().FillWidth(1.f)
		[
			SNew(STextBlock).Text(FText::FromString(Body)).Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::Gold()).AutoWrapText(true)
		];
	}
	else
	{
		Row->AddSlot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString((Message.bSpectator ? TEXT("[SPECTATOR] ") : TEXT("")) + Message.FromName + TEXT(":")))
			.Font(AIBStyle::SubHeading(16))
			.ColorAndOpacity(NameColor)
		];
		Row->AddSlot().FillWidth(1.f)
		[
			SNew(STextBlock).Text(FText::FromString(Body)).Font(AIBStyle::Body(16)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
		];
	}
	ChatList->AddSlot().AutoHeight().Padding(0.f, 2.f)[Row];
	while (ChatList->NumSlots() > 60)
	{
		ChatList->RemoveSlot(ChatList->GetSlot(0).GetWidget());
	}
	if (ChatScroll.IsValid())
	{
		ChatScroll->ScrollToEnd();
	}
	(void)G;
}

void SAIBHud::RebuildChat()
{
	if (!ChatList.IsValid() || !PC())
	{
		return;
	}
	ChatList->ClearChildren();
	for (const FAIBChatMessage& Message : PC()->GetChatLog())
	{
		OnChatMessage(Message);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Subtitles, toasts, hints, spectator bar
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeSubtitles()
{
	auto Audio = [this]() -> UAIBAudioSubsystem*
	{
		AAIBPlayerController* Controller = PC();
		return Controller && Controller->GetGameInstance() ? Controller->GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>() : nullptr;
	};
	auto Scale = [this]()
	{
		AAIBPlayerController* Controller = PC();
		UAIBGameInstance* GI = Controller ? Controller->GetGameInstance<UAIBGameInstance>() : nullptr;
		return GI ? FMath::Clamp(GI->GetSettings().SubtitleScale, 0.8f, 1.5f) : 1.f;
	};
	return SNew(SBox)
		.MaxDesiredWidth(1180.f)
		.Visibility_Lambda([this, Audio]()
		{
			UAIBAudioSubsystem* A = Audio();
			if (!A || A->GetSubtitle().IsEmpty())
			{
				return EVisibility::Collapsed;
			}
			// Lines already written on the story screen or the rules card are not captioned twice.
			const AAIBGameState* G = GS();
			const EAIBNarration Which = G ? AIBNarration::ForPhase(G->Phase) : EAIBNarration::None;
			if (G && SAIBStoryCards::IsShowing(Which, G->GetPhaseElapsed()) && AIBNarration::Contains(Which, A->GetSubtitleLine()))
			{
				return EVisibility::Collapsed;
			}
			return EVisibility::HitTestInvisible;
		})
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::RoundBrush(8.f))
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.62f))
			.Padding(FMargin(26.f, 12.f, 26.f, 14.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font_Lambda([Scale]() { return AIBStyle::SubHeading(FMath::RoundToInt(15 * Scale())); })
					.Text_Lambda([Audio]() { UAIBAudioSubsystem* A = Audio(); return A ? A->GetSubtitleSpeaker() : FText::GetEmpty(); })
					.ColorAndOpacity_Lambda([Audio]()
					{
						// Alice, the narrator, in gold; everyone else in red.
						UAIBAudioSubsystem* A = Audio();
						return FSlateColor(A && AIBNarration::IsNarratorLine(A->GetSubtitleLine()) ? AIBStyle::Gold() : AIBStyle::Blood());
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font_Lambda([Scale]() { return AIBStyle::BodyMedium(FMath::RoundToInt(23 * Scale())); })
					.ColorAndOpacity(AIBStyle::Text())
					.Justification(ETextJustify::Center)
					.WrapTextAt(1080.f)
					.LineHeightPercentage(1.1f)
					.Text_Lambda([Audio]() { UAIBAudioSubsystem* A = Audio(); return A ? A->GetSubtitle() : FText::GetEmpty(); })
				]
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeToasts()
{
	return SNew(SBox)
		.WidthOverride(380.f)
		.Visibility_Lambda([this]() { return PC() && PC()->Toasts.Num() > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::PanelBrush())
			.Padding(FMargin(16.f, 12.f))
			[
				SNew(STextBlock)
				.Font(AIBStyle::BodyMedium(17))
				.ColorAndOpacity(AIBStyle::Gold())
				.WrapTextAt(350.f)
				.Text_Lambda([this]() { return PC() && PC()->Toasts.Num() > 0 ? PC()->Toasts.Last().Text : FText::GetEmpty(); })
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeHints()
{
	return SNew(STextBlock)
		.Visibility(EVisibility::HitTestInvisible)
		.Font(AIBStyle::Body(14))
		.ColorAndOpacity(AIBStyle::TextDim())
		.Justification(ETextJustify::Right)
		.ShadowOffset(FVector2D(1.f, 1.f))
		.Text_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			if (!G)
			{
				return FText::GetEmpty();
			}
			switch (G->Phase)
			{
			case EAIBPhase::StoryIntro:
			case EAIBPhase::ArrivalCinematic:
				return PC() && PC()->HasLocallySkipped() ? LOCTEXT("HintSkipped", "Waiting for the others to skip...") : LOCTEXT("HintSkip", "[SPACE] Skip");
			case EAIBPhase::IslandExplore:
				return LOCTEXT("HintExplore", "WASD move   SHIFT sprint   SPACE jump   E read stone   V talk   ENTER chat   TAB scores");
			case EAIBPhase::Stage1Briefing:
			case EAIBPhase::Stage2Briefing:
				return LOCTEXT("HintBrief", "WASD move   V talk   ENTER chat   TAB scores");
			case EAIBPhase::Stage2Communication:
				return LOCTEXT("HintComm", "WASD move   V talk   ENTER chat / quick-tell   TAB scores");
			case EAIBPhase::Stage3Briefing:
			case EAIBPhase::Stage3Countdown:
			case EAIBPhase::Stage3Race:
				return LOCTEXT("HintRace", "W/S throttle   A/D steer   SPACE handbrake   R reset   hold E on the switch");
			default:
				return FText::GetEmpty();
			}
		});
}

TSharedRef<SWidget> SAIBHud::MakeSpectatorBar()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]()
		{
			const AAIBGameState* G = GS();
			const bool bShow = PC() && PC()->IsSpectating() && G && G->Phase != EAIBPhase::Results && G->Phase != EAIBPhase::EndingCinematic;
			return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.BorderImage(AIBStyle::PanelBrush())
		.Padding(FMargin(18.f, 8.f))
		[
			SNew(STextBlock)
			.Font(AIBStyle::SubHeading(17))
			.ColorAndOpacity(AIBStyle::TextDim())
			.Text_Lambda([this]()
			{
				const APawn* Target = PC() ? PC()->GetSpectateTarget() : nullptr;
				const AAIBPlayerState* PS = Target ? Target->GetPlayerState<AAIBPlayerState>() : nullptr;
				return PS ? FText::Format(LOCTEXT("Spectating", "SPECTATING  {0}     LMB / RMB switch"), FText::FromString(PS->GetPlayerName().ToUpper()))
					: LOCTEXT("SpectatingNone", "SPECTATING - your fate is sealed. Watch the others decide theirs.");
			})
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Story, rules cards, exploration
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeStoryLayer()
{
	return SNew(SBox)
		.Visibility_Lambda([this]() { return Phase() == EAIBPhase::StoryIntro ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(SAIBStoryCards)
			.Fullscreen(true)
			.Sequence(EAIBNarration::Story)
			.Elapsed_Lambda([this]() { const AAIBGameState* G = GS(); return G ? G->GetPhaseElapsed() : 0.f; })
		];
}

TSharedRef<SWidget> SAIBHud::MakeRulesCard()
{
	auto Which = [this]() { const AAIBGameState* G = GS(); return G ? AIBNarration::ForPhase(G->Phase) : EAIBNarration::None; };
	return SNew(SBox)
		.Visibility_Lambda([this, Which]()
		{
			const AAIBGameState* G = GS();
			const EAIBNarration Sequence = Which();
			const bool bShow = G && !bScoreboard && Sequence != EAIBNarration::Story && SAIBStoryCards::IsShowing(Sequence, G->GetPhaseElapsed());
			return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		[
			SNew(SAIBStoryCards)
			.Fullscreen(false)
			.Sequence_Lambda(Which)
			.Elapsed_Lambda([this]() { const AAIBGameState* G = GS(); return G ? G->GetPhaseElapsed() : 0.f; })
		];
}

TSharedRef<SWidget> SAIBHud::MakeExplorePanel()
{
	auto Found = [this]() { return PC() ? int32(FMath::CountBits64(uint64(PC()->GetLoreFound()))) : 0; };
	TSharedRef<SHorizontalBox> Dots = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < AAIBLoreStones::Count; ++Index)
	{
		Dots->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(3.f, 0.f)
		[
			SNew(SBox).WidthOverride(12.f).HeightOverride(12.f)
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::RoundBrush(6.f))
				.BorderBackgroundColor_Lambda([this, Index]()
				{
					const bool bRead = PC() && (PC()->GetLoreFound() & (1 << Index));
					return FSlateColor(bRead ? AIBStyle::Acid() : FLinearColor(1.f, 1.f, 1.f, 0.16f));
				})
			]
		];
	}

	return SNew(SOverlay)
		.Visibility_Lambda([this]() { return Phase() == EAIBPhase::IslandExplore && !bScoreboard ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		// Objective.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 26.f, 28.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(340.f)
			.Visibility(EVisibility::HitTestInvisible)
			[
				AIBUI::Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("ExploreTitle", "EXPLORE THE ISLAND")).Font(AIBStyle::Heading(20)).ColorAndOpacity(AIBStyle::Gold())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 8.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ExploreBody", "Follow the green light pillars to the old marker stones."))
						.Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(AIBStyle::SubHeading(17))
							.ColorAndOpacity_Lambda([Found]() { return FSlateColor(Found() >= AAIBLoreStones::Count ? AIBStyle::Acid() : AIBStyle::Text()); })
							.Text_Lambda([Found]()
							{
								return Found() >= AAIBLoreStones::Count ? LOCTEXT("AllStones", "ALL STONES READ")
									: FText::Format(LOCTEXT("StonesRead", "STONES READ  {0} / {1}"), Found(), AAIBLoreStones::Count);
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Dots]
					],
					FMargin(18.f, 12.f))
			]
		]
		// Read prompt at a stone.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0.f, 300.f, 0.f, 0.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return PC() && PC()->GetNearLore() != INDEX_NONE ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::RoundBrush(8.f))
				.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.7f))
				.Padding(FMargin(20.f, 10.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
					[
						SNew(SBorder)
						.BorderImage(AIBStyle::OutlineBrush(AIBStyle::Acid(), 5.f, 2.f))
						.Padding(FMargin(10.f, 2.f))
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("E"))).Font(AIBStyle::Title(26)).ColorAndOpacity(AIBStyle::Acid())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("ReadStone", "READ THE MARKER STONE")).Font(AIBStyle::Heading(22)).ColorAndOpacity(AIBStyle::Text())
					]
				]
			]
		];
}

// ---------------------------------------------------------------------------------------------------------------------
// Scoreboard & pause
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeScoreboard()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(STextBlock).Text(LOCTEXT("Scores", "CHAMPIONS")).Font(AIBStyle::Title(40)).ColorAndOpacity(AIBStyle::Text())
		];
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		auto Seat = [this, Slot]() -> AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
		Rows->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::CardBrush())
			.Padding(FMargin(12.f, 8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
					[
						SNew(SImage).Image(AIBStyle::CrestBrush(Slot)).ColorAndOpacity(AIB::Community(Slot).Color)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.34f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(AIBStyle::SubHeading(18)).ColorAndOpacity(AIBStyle::Text())
					.Text_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS ? FText::FromString(PS->GetPlayerName() + (PS->bBot && !PS->bDisconnected ? TEXT("  (AI)") : TEXT(""))) : LOCTEXT("Empty", "-"); })
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(AIBStyle::Body(15)).ColorAndOpacity(AIB::Community(Slot).Color).Text(AIB::Community(Slot).Name)
				]
				+ SHorizontalBox::Slot().FillWidth(0.22f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(AIBStyle::SubHeading(15)).ColorAndOpacity(AIBStyle::TextDim())
					.Text_Lambda([Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						if (!PS) return FText::GetEmpty();
						switch (PS->Status)
						{
						case EAIBPlayerStatus::Active: return LOCTEXT("SActive", "IN THE TRIALS");
						case EAIBPlayerStatus::Eliminated: return FText::Format(LOCTEXT("SElim", "OUT - STAGE {0}"), PS->EliminatedInStage);
						case EAIBPlayerStatus::Sacrificed: return LOCTEXT("SSac", "SACRIFICED");
						case EAIBPlayerStatus::Champion: return LOCTEXT("SChamp", "CHAMPION");
						default: return LOCTEXT("SForf", "FORFEITED");
						}
					})
				]
				+ SHorizontalBox::Slot().FillWidth(0.14f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(STextBlock).Font(AIBStyle::Title(22)).ColorAndOpacity(AIBStyle::Gold())
					.Text_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS ? FText::Format(LOCTEXT("SPts", "{0} WINS  {1} PTS"), PS->Stage1Wins, PS->Stage2Points) : FText::GetEmpty(); })
				]
			]
		];
	}
	return SNew(SBox)
		.Visibility_Lambda([this]() { return bScoreboard ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(860.f)
			[
				AIBUI::Panel(Rows, FMargin(28.f), true)
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeRules(bool bCompact)
{
	struct FRule { FText Title; FText Body; FLinearColor Color; };
	const FRule Rules[] =
	{
		{ LOCTEXT("R1T", "STAGE 1 - THE NUMBER GAME"), LOCTEXT("R1B", "Everyone secretly picks 0-100. Target = average x 0.8. Closest wins the round; FARTHEST IS ELIMINATED. Ties for farthest play sudden death. No pick in 10 s = random number."), AIBStyle::Blood() },
		{ LOCTEXT("R2T", "STAGE 2 - THE HIDDEN SYMBOL"), LOCTEXT("R2B", "Each champion carries a symbol everyone can see except them (symbols can repeat). Talk for 30 s - others may lie - then guess your own. 3 rounds; the top 2 advance."), AIBStyle::Ice() },
		{ LOCTEXT("R3T", "STAGE 3 - RACE TO THE GUARDIAN"), LOCTEXT("R3B", "The two finalists race to the beam of light and hold [E] on the switch for 3 s. Winning only earns the FIRST CHOICE. 90 s, then a shorter 45 s sudden-death race."), AIBStyle::Acid() },
		{ LOCTEXT("R4T", "THE GUARDIAN'S CHOICE"), LOCTEXT("R4B", "BECOME THE KING: you win, your community is lost. SAVE YOUR PEOPLE: you die, your community is saved and the choice passes on. No choice in time = King."), AIBStyle::Gold() }
	};
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	for (const FRule& Rule : Rules)
	{
		Box->AddSlot().AutoHeight().Padding(0.f, bCompact ? 5.f : 9.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3.f)
				[
					SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(Rule.Color)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(12.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Rule.Title).Font(AIBStyle::Heading(bCompact ? 16 : 20)).ColorAndOpacity(Rule.Color)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Rule.Body).Font(AIBStyle::Body(bCompact ? 14 : 16)).ColorAndOpacity(AIBStyle::Text()).AutoWrapText(true)
				]
			]
		];
	}
	return Box;
}

TSharedRef<SWidget> SAIBHud::MakePauseMenu()
{
	UAIBGameInstance* GI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr;
	return SNew(SBox)
		.Visibility_Lambda([this]() { return bPauseOpen ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return !bSettingsOpen && !bRulesOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				.WidthOverride(420.f)
				[
					AIBUI::Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
						[
							SNew(STextBlock).Text(LOCTEXT("Paused", "PAUSED")).Font(AIBStyle::Title(52)).ColorAndOpacity(AIBStyle::Text())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[
							SNew(SAIBButton).Text(LOCTEXT("Resume", "RESUME")).Kind(EAIBButtonKind::Primary)
							.OnClicked_Lambda([this]() { TogglePause(); return FReply::Handled(); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[
							SNew(SAIBButton).Text(LOCTEXT("HowTo", "HOW TO PLAY"))
							.OnClicked_Lambda([this]() { bRulesOpen = true; return FReply::Handled(); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[
							SNew(SAIBButton).Text(LOCTEXT("SettingsBtn", "SETTINGS"))
							.OnClicked_Lambda([this]() { bSettingsOpen = true; return FReply::Handled(); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[
							SNew(SAIBButton).Text(LOCTEXT("Leave", "LEAVE MATCH"))
							.SubText_Lambda([this]() { return PC() && PC()->IsHost() ? LOCTEXT("LeaveHost", "You are the host - this ends the match for everyone") : FText::GetEmpty(); })
							.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
						],
						FMargin(32.f), true)
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bSettingsOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SAIBSettingsPanel)
					.GameInstance(GI)
					.OnClose_Lambda([this]()
					{
						// Opened from the lobby: straight back to it. From the pause menu: back to the menu.
						bSettingsOpen = false;
						if (bSettingsOnly)
						{
							bSettingsOnly = false;
							bPauseOpen = false;
							FSlateApplication::Get().SetAllUserFocusToGameViewport();
						}
					})
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bRulesOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				.WidthOverride(760.f)
				[
					AIBUI::Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(LOCTEXT("HowToTitle", "HOW TO PLAY")).Font(AIBStyle::Title(44)).ColorAndOpacity(AIBStyle::Text())
						]
						+ SVerticalBox::Slot().AutoHeight()[MakeRules(false)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 14.f, 0.f, 0.f)
						[
							SNew(SAIBButton).Text(LOCTEXT("Back", "BACK")).MinWidth(160.f)
							.OnClicked_Lambda([this]()
							{
								bRulesOpen = false;
								if (bSettingsOnly)
								{
									bSettingsOnly = false;
									bPauseOpen = false;
									FSlateApplication::Get().SetAllUserFocusToGameViewport();
								}
								return FReply::Handled();
							})
						],
						FMargin(32.f), true)
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
