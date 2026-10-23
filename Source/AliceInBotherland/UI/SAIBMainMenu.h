// Alice in Botherland - main menu, prologue, lobby browser, hall of champions

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class AAIBMenuPlayerController;
class UAIBGameInstance;
class UAIBSessionSubsystem;
class SVerticalBox;

class ALICEINBOTHERLAND_API SAIBMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AAIBMenuPlayerController>, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum class EPage : uint8 { Home, Join, Settings, Prologue, HowTo, Hall, Credits };

	TSharedRef<SWidget> MakeHome();
	TSharedRef<SWidget> MakeJoin();
	TSharedRef<SWidget> MakePrologue();
	TSharedRef<SWidget> MakeHowTo();
	TSharedRef<SWidget> MakeHall();
	TSharedRef<SWidget> MakeCredits();
	TSharedRef<SWidget> PageBox(EPage Page, TSharedRef<SWidget> Content);
	void RebuildLobbyList();
	void RebuildHall();

	void Go(EPage Page);
	void StartPrologue();
	void StopPrologue();

	UAIBGameInstance* GI() const;
	UAIBSessionSubsystem* Sessions() const;

	TWeakObjectPtr<AAIBMenuPlayerController> Owner;
	EPage Current = EPage::Home;
	TSharedPtr<SVerticalBox> LobbyList;
	TSharedPtr<SVerticalBox> HallList;
	FString DirectAddress;
	bool bLanSearch = false;
	int32 ShownResults = -1;
	bool bWasSearching = false;

	// Prologue timeline
	TArray<FName> PrologueLines;
	int32 PrologueIndex = INDEX_NONE;
	double LineStart = 0.0;
	float LineDuration = 0.f;
	double PrologueStart = 0.0;
	float PrologueTotal = 1.f;
	FText MenuMessage;
	double MenuMessageTime = -100.0;
};
