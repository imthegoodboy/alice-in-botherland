// Alice in Botherland - in-match HUD (lobby, trials, race, final choice, results)

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Core/AIBTypes.h"

class AAIBPlayerController;
class AAIBGameState;
class AAIBPlayerState;
class SVerticalBox;
class SEditableTextBox;
class SScrollBox;

/** Paints nameplates over champions and the Guardian waypoint during the race. */
class SAIBWorldOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBWorldOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AAIBPlayerController>, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100.f, 100.f); }

private:
	TWeakObjectPtr<AAIBPlayerController> Owner;
};

class ALICEINBOTHERLAND_API SAIBHud : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBHud) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AAIBPlayerController>, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	bool WantsCursor() const;
	bool BlocksGameInput() const;
	void OpenChat();
	void CloseChat();
	bool IsChatOpen() const { return bChatOpen; }
	void TogglePause();
	void SetScoreboardVisible(bool bVisible) { bScoreboard = bVisible; }
	void OnChatMessage(const FAIBChatMessage& Message);
	void RebuildChat();

	/** Rules summary used in the lobby, pause menu and main menu. */
	static TSharedRef<SWidget> MakeRules(bool bCompact);

private:
	// Accessors
	AAIBPlayerController* PC() const;
	AAIBGameState* GS() const;
	AAIBPlayerState* MyPS() const;
	EAIBPhase Phase() const;
	float ServerNow() const;

	// Layers (SAIBHud.cpp)
	TSharedRef<SWidget> MakeTopBar();
	TSharedRef<SWidget> MakeBanner();
	TSharedRef<SWidget> MakeRoster();
	TSharedRef<SWidget> MakeChat();
	TSharedRef<SWidget> MakeSubtitles();
	TSharedRef<SWidget> MakeToasts();
	TSharedRef<SWidget> MakeHints();
	TSharedRef<SWidget> MakeScoreboard();
	TSharedRef<SWidget> MakePauseMenu();
	TSharedRef<SWidget> MakeLetterbox();
	TSharedRef<SWidget> MakeSpectatorBar();

	// Stage panels (SAIBHudPanels.cpp)
	TSharedRef<SWidget> MakeLobbyPanel();
	TSharedRef<SWidget> MakeArrivalPanel();
	TSharedRef<SWidget> MakeNumberPanel();
	TSharedRef<SWidget> MakeNumberResults();
	TSharedRef<SWidget> MakeSymbolPanel();
	TSharedRef<SWidget> MakeGuessPanel();
	TSharedRef<SWidget> MakeSymbolResults();
	TSharedRef<SWidget> MakeRacePanel();
	TSharedRef<SWidget> MakeFinalPanel();
	TSharedRef<SWidget> MakeResultsPanel();
	TSharedRef<SWidget> MakeSeatRow(int32 Slot);
	void RebuildResults();
	void RebuildSymbolRows();
	void RebuildNumberResults();
	void RebuildSymbolResults();

	TWeakObjectPtr<AAIBPlayerController> Owner;

	TSharedPtr<SVerticalBox> ChatList;
	TSharedPtr<SScrollBox> ChatScroll;
	TSharedPtr<SEditableTextBox> ChatInput;
	TSharedPtr<SVerticalBox> ResultsBox;
	TSharedPtr<SVerticalBox> SymbolRows;
	TSharedPtr<SVerticalBox> NumberResultsBox;
	TSharedPtr<SVerticalBox> SymbolResultsBox;

	bool bChatOpen = false;
	bool bPauseOpen = false;
	bool bSettingsOpen = false;
	bool bRulesOpen = false;
	bool bScoreboard = false;
	float LastChatTime = -100.f;
	int32 BuiltResultsSerial = -1;
	int32 BuiltSymbolSerial = -1;
	int32 BuiltNumberSerial = -1;
	int32 BuiltSymbolResultsSerial = -1;
	int32 InviteNotice = 0;
};
