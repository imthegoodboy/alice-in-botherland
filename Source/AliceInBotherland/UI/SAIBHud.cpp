// Alice in Botherland - in-match HUD: frame, overlays, chat, menus

#include "UI/SAIBHud.h"
#include "UI/AIBStyle.h"
#include "UI/SAIBWidgets.h"
#include "UI/SAIBGameUI.h"
#include "UI/SAIBLoadingScreen.h"
#include "UI/SAIBStoryCards.h"
#include "Audio/AIBLastWords.h"
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
#include "Widgets/Layout/SBackgroundBlur.h"
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
			case EAIBPhase::Epilogue:
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
		+ SOverlay::Slot()[MakeFallScene()]
		+ SOverlay::Slot()[MakeStoryLayer()]
		+ SOverlay::Slot()[MakeEpilogueVerdict()]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom)
		.Padding(TAttribute<FMargin>::CreateLambda([this]()
		{
			// In the room the start column fills the bottom left, so the chat sits above it.
			const EAIBPhase P = Phase();
			const bool bLobby = P == EAIBPhase::Lobby || P == EAIBPhase::LobbyCountdown;
			return bLobby ? FMargin(84.f, 0.f, 0.f, 360.f) : FMargin(24.f, 0.f, 0.f, 24.f);
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
		.Padding(TAttribute<FMargin>::CreateLambda([this]() { return FMargin(0.f, Phase() == EAIBPhase::IslandExplore ? 250.f : 130.f, 28.f, 0.f); }))
		[MakeToasts()]
		// Menu / settings button (Esc does the same); the room has its own row of buttons.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 18.f, 24.f, 0.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				const EAIBPhase P = Phase();
				const bool bRoom = P == EAIBPhase::Lobby || P == EAIBPhase::LobbyCountdown || P == EAIBPhase::None;
				return bPauseOpen || bRoom ? EVisibility::Collapsed : EVisibility::Visible;
			})
			[
				SNew(SHorizontalBox)
				// Microphone: shows off / push-to-talk / open, lights up while sending. M toggles it from anywhere.
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(SAIBIconButton)
					.Icon_Lambda([this]() { return PC() && PC()->GetMicMode() == 0 ? AIBIcon::MicOff : AIBIcon::Mic; })
					.Label_Lambda([this]()
					{
						if (!PC())
						{
							return FText::GetEmpty();
						}
						if (PC()->IsSpeaking())
						{
							return LOCTEXT("MicLive", "LIVE");
						}
						switch (PC()->GetMicMode())
						{
						case 0: return LOCTEXT("MicHudOff", "MIC OFF  M");
						case 2: return LOCTEXT("MicHudOn", "MIC ON  M");
						default: return LOCTEXT("MicHudPtt", "HOLD V");
						}
					})
					.Size(46.f)
					.IsActive_Lambda([this]() { return PC() && PC()->IsSpeaking(); })
					.OnClicked_Lambda([this]() { if (PC()) PC()->ToggleMicMute(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SAIBIconButton)
					.Icon(AIBIcon::Settings)
					.Label(LOCTEXT("MenuButton", "MENU  ESC"))
					.Size(46.f)
					.OnClicked_Lambda([this]() { TogglePause(); return FReply::Handled(); })
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.f, 8.f, 0.f, 0.f)[SNew(SAIBFpsCounter)]
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
		if (UAIBGameInstance* GI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr)
		{
			GI->SaveProfile();
		}
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
		// No clock over the Guardian's judgement.
		if (G->Fallen.Num() > 0 && (G->Phase == EAIBPhase::Stage1Elimination || G->Phase == EAIBPhase::Stage2Elimination))
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
		case EAIBPhase::Epilogue:
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
							return G->bStage1SuddenDeath ? LOCTEXT("SubSD", "Sudden death - everyone picks, only the tied can fall") : FText::Format(LOCTEXT("SubRound", "Round {0}"), G->Stage1Round);
						case EAIBPhase::Stage2Communication:
						case EAIBPhase::Stage2Guess:
							return G->bStage2SuddenDeath ? LOCTEXT("SubSD2", "Sudden death round") : FText::Format(LOCTEXT("SubRound2", "Symbol round {0} of 3"), G->Stage2Round);
						case EAIBPhase::Stage3Race:
							return G->bRaceSuddenDeath ? LOCTEXT("SubSD3", "Sudden death race") : LOCTEXT("SubRace", "First into the Guardian's ring takes the first choice");
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
						if (G->bStage1SuddenDeath && G->Stage1AtRisk.Contains(PS->Slot) && G->Phase >= EAIBPhase::Stage1RoundIntro && G->Phase <= EAIBPhase::Stage1Results)
						{
							return LOCTEXT("AtRisk", "AT RISK");
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
					.ColorAndOpacity_Lambda([this, Seat]()
					{
						const AAIBPlayerState* PS = Seat();
						const AAIBGameState* G = GS();
						if (PS && G && G->bStage1SuddenDeath && G->Stage1AtRisk.Contains(PS->Slot) && G->Phase != EAIBPhase::Stage1Selection)
						{
							return FSlateColor(AIBStyle::Blood());
						}
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
			if (G->Fallen.Num() > 0 && (G->Phase == EAIBPhase::Stage1Elimination || G->Phase == EAIBPhase::Stage2Elimination))
			{
				return EVisibility::Collapsed;
			}
			switch (G->Phase)
			{
			case EAIBPhase::StoryIntro:
			case EAIBPhase::ArrivalCinematic:
			case EAIBPhase::EndingCinematic:
			case EAIBPhase::Epilogue:
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
			const EAIBNarration Which = G ? AIBNarration::ForPhase(G->Phase, G->Ending) : EAIBNarration::None;
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
			case EAIBPhase::Epilogue:
				return PC() && PC()->HasLocallySkipped() ? LOCTEXT("HintSkipped", "Waiting for the others to skip...") : LOCTEXT("HintSkip", "[SPACE] Skip");
			case EAIBPhase::IslandExplore:
				return LOCTEXT("HintExplore", "WASD move   SHIFT sprint   E read stone   V talk   M mic   ENTER chat   TAB scores");
			case EAIBPhase::Stage1Briefing:
			case EAIBPhase::Stage2Briefing:
				return LOCTEXT("HintBrief", "WASD move   V talk   ENTER chat   TAB scores");
			case EAIBPhase::Stage2Communication:
				return LOCTEXT("HintComm", "WASD move   V talk   ENTER chat / quick-tell   TAB scores");
			case EAIBPhase::Stage3Briefing:
			case EAIBPhase::Stage3Countdown:
			case EAIBPhase::Stage3Race:
				return LOCTEXT("HintRace", "W/S throttle   A/D steer   SPACE handbrake   R reset   first into the Guardian's ring wins");
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
			const bool bShow = PC() && PC()->IsSpectating() && G && G->Phase != EAIBPhase::Results && G->Phase != EAIBPhase::EndingCinematic && G->Phase != EAIBPhase::Epilogue
				&& !(G->Fallen.Num() > 0 && (G->Phase == EAIBPhase::Stage1Elimination || G->Phase == EAIBPhase::Stage2Elimination));
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
		.Visibility_Lambda([this]() { return Phase() == EAIBPhase::StoryIntro || Phase() == EAIBPhase::Epilogue ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(SAIBStoryCards)
			.Fullscreen(true)
			// The opening story, or (after the ending) the truth about the island.
			.Sequence_Lambda([this]()
			{
				const AAIBGameState* G = GS();
				return G && G->Phase == EAIBPhase::Epilogue ? AIBNarration::EpilogueFor(G->Ending) : EAIBNarration::Story;
			})
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
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 116.f, 28.f, 0.f)
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
// The Guardian's judgement and the epilogue's verdict
// ---------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SAIBHud::MakeFallScene()
{
	auto InScene = [this]()
	{
		const AAIBGameState* G = GS();
		return G && G->Fallen.Num() > 0 && (G->Phase == EAIBPhase::Stage1Elimination || G->Phase == EAIBPhase::Stage2Elimination);
	};
	// Letterbox bars slide in as the scene starts.
	auto BarHeight = [this, InScene]()
	{
		const AAIBGameState* G = GS();
		return InScene() && G ? 110.f * FMath::SmoothStep(0.f, 0.8f, G->GetPhaseElapsed()) : 0.f;
	};
	// The fallen champion speaking now (until the beam comes).
	auto Speaking = [this]() -> const FAIBFallen*
	{
		const AAIBGameState* G = GS();
		if (!G || G->Fallen.Num() == 0)
		{
			return nullptr;
		}
		const float Elapsed = G->GetPhaseElapsed();
		const FAIBFallen* Current = nullptr;
		for (const FAIBFallen& Fallen : G->Fallen)
		{
			if (Elapsed >= Fallen.SpeakAt - 0.2f)
			{
				Current = &Fallen;
			}
		}
		return Elapsed < G->FallStrikeAt - 0.7f ? Current : nullptr;
	};
	auto QuoteAlpha = [this, Speaking]()
	{
		const AAIBGameState* G = GS();
		const FAIBFallen* Fallen = Speaking();
		if (!G || !Fallen)
		{
			return 0.f;
		}
		const float Elapsed = G->GetPhaseElapsed();
		// Fades in as they start, out before the next speaker or the beam.
		float End = G->FallStrikeAt - 0.7f;
		for (const FAIBFallen& Other : G->Fallen)
		{
			if (Other.SpeakAt > Fallen->SpeakAt)
			{
				End = FMath::Min(End, Other.SpeakAt - 0.2f);
			}
		}
		return FMath::Min(FMath::Clamp((Elapsed - Fallen->SpeakAt + 0.2f) / 0.6f, 0.f, 1.f), FMath::Clamp((End - Elapsed) / 0.5f, 0.f, 1.f));
	};
	auto Flash = [this, InScene]()
	{
		const AAIBGameState* G = GS();
		if (!InScene() || !G)
		{
			return FLinearColor::Transparent;
		}
		const float Since = G->GetPhaseElapsed() - G->FallStrikeAt;
		const float Alpha = Since < 0.f ? 0.f : 0.55f * FMath::Max(0.f, 1.f - Since / 0.9f);
		return FLinearColor(0.9f, 0.04f, 0.02f, Alpha);
	};

	return SNew(SOverlay)
		.Visibility_Lambda([InScene]() { return InScene() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride_Lambda(BarHeight)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride_Lambda(BarHeight)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda([Flash]() { return FSlateColor(Flash()); })
		]
		// Last words, in the lower third above the bottom bar.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 150.f)
		[
			SNew(SBox)
			.WidthOverride(1300.f)
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::WhiteBrush())
				.BorderBackgroundColor(FLinearColor::Transparent)
				.Padding(0.f)
				.ColorAndOpacity_Lambda([QuoteAlpha]() { return FLinearColor(1.f, 1.f, 1.f, QuoteAlpha()); })
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Story(40))
					.ColorAndOpacity(FLinearColor(0.97f, 0.94f, 0.88f))
					.ShadowOffset(FVector2D(2.f, 2.f))
					.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.Text_Lambda([Speaking]()
					{
						const FAIBFallen* Fallen = Speaking();
						return Fallen ? FText::Format(LOCTEXT("LastWordsQuote", "\u201C{0}\u201D"), AIBLastWords::Text(Fallen->Words)) : FText::GetEmpty();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Spaced(AIBStyle::SubHeading(20), 260))
					.ShadowOffset(FVector2D(1.f, 1.f))
					.ColorAndOpacity_Lambda([this, Speaking]()
					{
						const FAIBFallen* Fallen = Speaking();
						return FSlateColor(Fallen ? AIB::Community(Fallen->Slot).Color : AIBStyle::Text());
					})
					.Text_Lambda([this, Speaking]()
					{
						const FAIBFallen* Fallen = Speaking();
						const AAIBGameState* G = GS();
						const AAIBPlayerState* PS = Fallen && G ? G->GetPlayerBySlot(Fallen->Slot) : nullptr;
						return PS ? FText::Format(LOCTEXT("LastWordsBy", "{0}  -  CHAMPION OF {1}"), FText::FromString(PS->GetPlayerName().ToUpper()), PS->GetCommunityName().ToUpper()) : FText::GetEmpty();
					})
				]
				]
			]
		];
}

TSharedRef<SWidget> SAIBHud::MakeEpilogueVerdict()
{
	// Shows over the epilogue's last picture: the champion the island chose, or that nobody claimed it.
	auto Alpha = [this]()
	{
		const AAIBGameState* G = GS();
		if (!G || G->Phase != EAIBPhase::Epilogue)
		{
			return 0.f;
		}
		const EAIBNarration Which = AIBNarration::EpilogueFor(G->Ending);
		const AIBNarration::FSequence& Sequence = AIBNarration::Get(Which);
		if (Sequence.Starts.Num() == 0)
		{
			return 0.f;
		}
		return FMath::Clamp((G->GetPhaseElapsed() - Sequence.Starts.Last() - 0.8f) / 1.2f, 0.f, 1.f);
	};
	auto Champion = [this]() -> const AAIBPlayerState*
	{
		const AAIBGameState* G = GS();
		return G && G->Ending != EAIBEnding::DoubleSacrifice && G->Ending != EAIBEnding::SoloSacrifice ? G->GetPlayerBySlot(G->ChampionSlot) : nullptr;
	};
	return SNew(SBox)
		.Visibility_Lambda([Alpha]() { return Alpha() > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, 0.f, 0.f, 330.f))
		[
			SNew(SBorder)
			.BorderImage(AIBStyle::WhiteBrush())
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(0.f)
			.ColorAndOpacity_Lambda([Alpha]() { return FLinearColor(1.f, 1.f, 1.f, Alpha()); })
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Spaced(AIBStyle::StoryBold(24), 300))
				.ColorAndOpacity(AIBStyle::Gold())
				.ShadowOffset(FVector2D(2.f, 2.f))
				.Text_Lambda([Champion]() { return Champion() ? LOCTEXT("VerdictChosen", "THE ISLAND HAS CHOSEN") : LOCTEXT("VerdictNone", "THE ISLAND REMAINS UNCLAIMED"); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Title(120))
				.ShadowOffset(FVector2D(3.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f))
				.ColorAndOpacity_Lambda([Champion]() { const AAIBPlayerState* PS = Champion(); return FSlateColor(PS ? PS->GetCommunityColor() : FLinearColor(0.85f, 0.95f, 1.f)); })
				.Text_Lambda([Champion]()
				{
					const AAIBPlayerState* PS = Champion();
					return PS ? FText::FromString(PS->GetPlayerName().ToUpper()) : LOCTEXT("VerdictSaved", "THEY CHOSE EACH OTHER");
				})
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(AIBStyle::Story(26))
				.ColorAndOpacity(AIBStyle::Text())
				.ShadowOffset(FVector2D(2.f, 2.f))
				.Text_Lambda([Champion]()
				{
					const AAIBPlayerState* PS = Champion();
					return PS ? FText::Format(LOCTEXT("VerdictOf", "of {0}"), PS->GetCommunityName()) : LOCTEXT("VerdictSavedSub", "and every community lives on");
				})
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
		{ LOCTEXT("R1T", "STAGE 1 - THE NUMBER GAME"), LOCTEXT("R1B", "Everyone secretly picks 0-100. Target = average x 0.8. Closest wins the round; FARTHEST IS ELIMINATED. A tie for farthest: everyone picks again, but only the tied can fall. Not locked in: your dial counts; no number at all = random."), AIBStyle::Blood() },
		{ LOCTEXT("R2T", "STAGE 2 - THE HIDDEN SYMBOL"), LOCTEXT("R2B", "Each champion carries a symbol everyone can see except them (symbols can repeat). Talk for 30 s - others may lie - then guess your own. 3 rounds; the top 2 advance."), AIBStyle::Ice() },
		{ LOCTEXT("R3T", "STAGE 3 - RACE TO THE GUARDIAN"), LOCTEXT("R3B", "The two finalists race to the beam of light. The first to drive into the Guardian's glowing ring wins the FIRST CHOICE - not the island. 90 s, then a shorter 45 s sudden-death race."), AIBStyle::Acid() },
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
	TWeakObjectPtr<UAIBGameInstance> WeakGI = PC() ? PC()->GetGameInstance<UAIBGameInstance>() : nullptr;
	// Quick settings change the live profile directly (the controller re-applies them every frame); the profile is
	// saved when the menu closes.
	auto Live = [WeakGI]() -> FAIBSettings* { return WeakGI.IsValid() ? &WeakGI->GetProfile()->Settings : nullptr; };
	auto Apply = [WeakGI]() { if (WeakGI.IsValid()) WeakGI->ApplySettings(); };

	auto QuickRow = [](const FText& Label, TSharedRef<SWidget> Control)
	{
		return SNew(SBox)
			.Padding(FMargin(0.f, 3.f))
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::WhiteBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.035f))
				.Padding(FMargin(18.f, 10.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Label).Font(AIBStyle::BodyMedium(18)).ColorAndOpacity(AIBStyle::Text())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Control]
				]
			];
	};
	auto QuickSlider = [Live, Apply](float FAIBSettings::* Field, float Min, float Max, float Step, bool bPercent)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SAIBSlider)
				.Width(230.f)
				.MinValue(Min)
				.MaxValue(Max)
				.StepSize(Step)
				.Value_Lambda([Live, Field]() { const FAIBSettings* S = Live(); return S ? S->*Field : 0.f; })
				.OnValueChanged_Lambda([Live, Apply, Field](float Value) { if (FAIBSettings* S = Live()) { S->*Field = Value; Apply(); } })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(70.f).HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Font(AIBStyle::Title(26))
					.ColorAndOpacity(AIBStyle::Gold())
					.Text_Lambda([Live, Field, bPercent]()
					{
						const FAIBSettings* S = Live();
						const float Value = S ? S->*Field : 0.f;
						return bPercent ? FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))) : FText::FromString(Value >= 10.f ? FString::Printf(TEXT("%.0f"), Value) : FString::Printf(TEXT("%.2f"), Value));
					})
				]
			];
	};
	TSharedRef<SHorizontalBox> MicChoices = SNew(SHorizontalBox);
	const FText MicNames[] = { LOCTEXT("QMicOff", "OFF"), LOCTEXT("QMicPtt", "HOLD V"), LOCTEXT("QMicOpen", "ON") };
	for (int32 Choice = 0; Choice < 3; ++Choice)
	{
		MicChoices->AddSlot().AutoWidth().Padding(3.f, 0.f)
		[
			SNew(SAIBSlantButton)
			.Text(MicNames[Choice])
			.FontSize(15)
			.Height(38.f)
			.MinWidth(92.f)
			.Centered(true)
			.IsSelected_Lambda([Live, Choice]() { const FAIBSettings* S = Live(); return S && S->MicMode == Choice; })
			.OnClicked_Lambda([Live, Apply, Choice]() { if (FAIBSettings* S = Live()) { S->MicMode = Choice; Apply(); } return FReply::Handled(); })
		];
	}

	// Other champions' voices: mute anyone (their chat too).
	TSharedRef<SVerticalBox> VoiceRows = SNew(SVerticalBox);
	auto AnyOtherHuman = [this]()
	{
		const AAIBGameState* G = GS();
		if (!G)
		{
			return false;
		}
		for (const AAIBPlayerState* PS : G->GetSeatedPlayers())
		{
			if (!PS->bBot && PS != MyPS())
			{
				return true;
			}
		}
		return false;
	};
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		auto Seat = [this, Slot]() -> const AAIBPlayerState* { const AAIBGameState* G = GS(); return G ? G->GetPlayerBySlot(Slot) : nullptr; };
		auto IsOther = [this, Seat]() { const AAIBPlayerState* PS = Seat(); return PS && !PS->bBot && PS != MyPS(); };
		auto IsMuted = [this, Seat]() { return PC() && PC()->IsMutedLocally(Seat()); };
		VoiceRows->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SBox)
			.Visibility_Lambda([IsOther]() { return IsOther() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::WhiteBrush())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.035f))
				.Padding(FMargin(14.f, 5.f, 8.f, 5.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
					[
						SNew(SBox).WidthOverride(4.f).HeightOverride(26.f)
						[
							SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIB::Community(Slot).Color)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(AIBStyle::BodyMedium(17))
						.ColorAndOpacity_Lambda([IsMuted]() { return FSlateColor(IsMuted() ? AIBStyle::TextFaint() : AIBStyle::Text()); })
						.Text_Lambda([Seat]() { const AAIBPlayerState* PS = Seat(); return PS ? FText::FromString(PS->GetPlayerName()) : FText::GetEmpty(); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
					[
						AIBUI::TalkDot(TAttribute<bool>::CreateLambda([Seat, IsMuted]() { const AAIBPlayerState* PS = Seat(); return PS && PS->bTalking && !IsMuted(); }))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(STextBlock)
						.Font(AIBStyle::IconFont(20))
						.Text_Lambda([IsMuted]() { return AIBIcon::Glyph(IsMuted() ? AIBIcon::VolumeOff : AIBIcon::Volume); })
						.ColorAndOpacity_Lambda([IsMuted]() { return FSlateColor(IsMuted() ? AIBStyle::Blood() : AIBStyle::TextDim()); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SAIBSlantButton)
						.Text_Lambda([IsMuted]() { return IsMuted() ? LOCTEXT("Unmute", "UNMUTE") : LOCTEXT("Mute", "MUTE"); })
						.FontSize(14)
						.Height(32.f)
						.MinWidth(96.f)
						.Centered(true)
						.IsSelected_Lambda(IsMuted)
						.OnClicked_Lambda([this, Seat]() { if (PC()) PC()->ToggleMuteLocally(Seat()); return FReply::Handled(); })
					]
				]
			]
		];
	}
	VoiceRows->AddSlot().AutoHeight().Padding(0.f, 2.f)
	[
		SNew(STextBlock)
		.Visibility_Lambda([AnyOtherHuman]() { return AnyOtherHuman() ? EVisibility::Collapsed : EVisibility::Visible; })
		.Text(LOCTEXT("NoVoices", "Only AI champions with you - nobody to hear."))
		.Font(AIBStyle::Body(14)).ColorAndOpacity(AIBStyle::TextFaint())
	];

	TSharedRef<SWidget> QuickSettings = SNew(SBox)
		.WidthOverride(640.f)
		[
			AIBUI::Frame(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[AIBUI::Icon(AIBIcon::Tune, 28, AIBStyle::Gold())]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("QuickTitle", "QUICK SETTINGS")).Font(AIBStyle::Spaced(AIBStyle::Title(36), 40)).ColorAndOpacity(AIBStyle::Text())
					]
				]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QSens", "Mouse sensitivity"), QuickSlider(&FAIBSettings::MouseSensitivity, 0.2f, 3.f, 0.05f, false))]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QFov", "Field of view"), QuickSlider(&FAIBSettings::FieldOfView, 60.f, 110.f, 1.f, false))]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QMaster", "Master volume"), QuickSlider(&FAIBSettings::MasterVolume, 0.f, 1.f, 0.01f, true))]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QVoice", "Narrator voice"), QuickSlider(&FAIBSettings::VoiceVolume, 0.f, 1.f, 0.01f, true))]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QMic", "Microphone  (M)"), MicChoices)]
				+ SVerticalBox::Slot().AutoHeight()[QuickRow(LOCTEXT("QVoiceChat", "Voice chat volume"), QuickSlider(&FAIBSettings::VoiceChatVolume, 0.f, 1.5f, 0.01f, true))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)[AIBUI::Icon(AIBIcon::Voice, 18, AIBStyle::Gold())]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("VoicesTitle", "VOICES")).Font(AIBStyle::Spaced(AIBStyle::SubHeading(14), 200)).ColorAndOpacity(AIBStyle::Gold())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Visibility_Lambda([this]() { const AAIBGameState* G = GS(); return G && G->bMatchInProgress ? EVisibility::Visible : EVisibility::Collapsed; })
						.Text(LOCTEXT("VoicesNote", "Champions in the trials can't hear the eliminated"))
						.Font(AIBStyle::Body(12)).ColorAndOpacity(AIBStyle::TextFaint())
					]
				]
				+ SVerticalBox::Slot().AutoHeight()[VoiceRows]
				+ SVerticalBox::Slot().AutoHeight()
				[
					QuickRow(LOCTEXT("QCaptions", "Captions"),
						SNew(SAIBToggle)
						.IsOn_Lambda([Live]() { const FAIBSettings* S = Live(); return S && S->bSubtitles; })
						.OnToggled_Lambda([Live, Apply](bool bOn) { if (FAIBSettings* S = Live()) { S->bSubtitles = bOn; Apply(); } }))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 12.f, 0.f, 0.f)
				[
					SNew(SAIBSlantButton).Kind(EAIBButtonKind::Ghost).Icon(AIBIcon::Settings).Text(LOCTEXT("AllSettings", "ALL SETTINGS")).FontSize(17).Height(42.f)
					.OnClicked_Lambda([this]() { bSettingsOpen = true; return FReply::Handled(); })
				],
				FMargin(28.f, 24.f), FLinearColor(1.f, 0.74f, 0.32f, 0.9f), 0.86f)
		];

	auto MenuButton = [](TCHAR Glyph, const FText& Text, EAIBButtonKind Kind, TFunction<void()> Action)
	{
		return SNew(SAIBSlantButton)
			.Kind(Kind)
			.Icon(Glyph)
			.Text(Text)
			.FontSize(Kind == EAIBButtonKind::Primary ? 34 : 26)
			.Height(Kind == EAIBButtonKind::Primary ? 88.f : 70.f)
			.MinWidth(470.f)
			.Shine(Kind == EAIBButtonKind::Primary)
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
	};

	TSharedRef<SWidget> Menu = SNew(SBox)
		.WidthOverride(480.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SAIBReveal)[AIBUI::PageTitle(LOCTEXT("MenuKicker", "THE MATCH GOES ON"), LOCTEXT("Paused", "MENU"), 92)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MenuNote", "The other champions keep playing while this menu is open."))
				.Font(AIBStyle::Body(15)).ColorAndOpacity(AIBStyle::TextDim()).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SAIBReveal).Delay(0.05f)[MenuButton(AIBIcon::Play, LOCTEXT("Resume", "RESUME"), EAIBButtonKind::Primary, [this]() { TogglePause(); })]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SAIBReveal).Delay(0.1f)[MenuButton(AIBIcon::Settings, LOCTEXT("SettingsBtn", "SETTINGS"), EAIBButtonKind::Secondary, [this]() { bSettingsOpen = true; })]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SAIBReveal).Delay(0.15f)[MenuButton(AIBIcon::Book, LOCTEXT("HowTo", "HOW TO PLAY"), EAIBButtonKind::Secondary, [this]() { bRulesOpen = true; })]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { const AAIBGameState* G = GS(); return PC() && PC()->IsHost() && G && G->bMatchInProgress && G->Phase != EAIBPhase::Results ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SAIBReveal).Delay(0.18f)
					[
						SNew(SAIBSlantButton)
						.Icon(AIBIcon::Restart)
						.Text_Lambda([this]() { return FPlatformTime::Seconds() < EndMatchConfirmUntil ? LOCTEXT("EndMatchSure", "CLICK AGAIN TO END") : LOCTEXT("EndMatch", "END MATCH"); })
						.SubText(LOCTEXT("EndMatchSub", "Everyone goes back to the room"))
						.AccentColor(AIBStyle::Ember())
						.FontSize(26)
						.Height(70.f)
						.MinWidth(470.f)
						.OnClicked_Lambda([this]()
						{
							// Two clicks: ending a match for five people shouldn't be an accident.
							if (FPlatformTime::Seconds() < EndMatchConfirmUntil)
							{
								EndMatchConfirmUntil = 0.0;
								if (PC())
								{
									PC()->ServerReturnToLobby();
								}
								TogglePause();
							}
							else
							{
								EndMatchConfirmUntil = FPlatformTime::Seconds() + 3.0;
							}
							return FReply::Handled();
						})
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
			[
				SNew(SAIBReveal).Delay(0.2f)
				[
					SNew(SAIBSlantButton)
					.Icon(AIBIcon::Logout)
					.Text(LOCTEXT("Leave", "LEAVE MATCH"))
					.SubText_Lambda([this]() { return PC() && PC()->IsHost() ? LOCTEXT("LeaveHost", "Ends the match for everyone") : LOCTEXT("LeaveGuest", "Back to the main menu"); })
					.AccentColor(AIBStyle::Blood())
					.FontSize(26)
					.Height(70.f)
					.MinWidth(470.f)
					.OnClicked_Lambda([this]() { if (PC()) PC()->LeaveMatch(); return FReply::Handled(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(30.f, 16.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("MenuKeys", "ESC  close menu")).Font(AIBStyle::Spaced(AIBStyle::SubHeading(14), 160)).ColorAndOpacity(AIBStyle::TextFaint())
			]
		];

	auto Backdrop = [](float Alpha)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SBackgroundBlur).BlurStrength(12.f)]
			+ SOverlay::Slot()[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, Alpha))]
			+ SOverlay::Slot()[SNew(SAIBGradient).Vertical(false).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.7f), FLinearColor(0.f, 0.f, 0.f, 0.2f), FLinearColor(0.f, 0.f, 0.f, 0.f) })];
	};

	return SNew(SBox)
		.Visibility_Lambda([this]() { return bPauseOpen ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SOverlay)
			+ SOverlay::Slot()[Backdrop(0.62f)]
			// Menu and quick settings.
			+ SOverlay::Slot()
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return !bSettingsOpen && !bRulesOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(110.f, 0.f, 70.f, 0.f)[Menu]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SAIBReveal).Offset(FVector2D(40.f, 0.f)).Delay(0.15f)[QuickSettings]
					]
				]
			]
			// Full settings.
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bSettingsOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					SNew(SAIBReveal).Offset(FVector2D(0.f, 30.f))
					[
						SNew(SAIBSettingsPanel)
						.GameInstance(WeakGI)
						.OnClose_Lambda([this]()
						{
							// Opened from the room: straight back to it. From the menu: back to the menu.
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
			]
			// Rules.
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return bRulesOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				.WidthOverride(900.f)
				[
					SNew(SAIBReveal).Offset(FVector2D(0.f, 30.f))
					[
						AIBUI::Frame(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
							[
								AIBUI::PageTitle(LOCTEXT("RulesKicker", "THE GUARDIAN'S LAW"), LOCTEXT("HowToTitle", "HOW TO PLAY"), 60)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).MaxDesiredHeight(560.f)
								[
									SNew(SScrollBox).ScrollBarThickness(FVector2D(5.f, 5.f)) + SScrollBox::Slot().Padding(0.f, 0.f, 12.f, 0.f)[MakeRules(false)]
								]
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 16.f, 0.f, 0.f)
							[
								SNew(SAIBSlantButton).Kind(EAIBButtonKind::Primary).Icon(AIBIcon::Back).Text(LOCTEXT("Back", "BACK")).FontSize(22).Height(56.f).MinWidth(200.f)
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
							FMargin(34.f, 28.f), FLinearColor(1.f, 0.74f, 0.32f, 0.9f), 0.92f)
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
