// Alice in Botherland - main menu: champion profile, rooms, the story, hall of champions

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class AAIBMenuPlayerController;
class UAIBGameInstance;
class UAIBSessionSubsystem;
class SVerticalBox;
class SEditableTextBox;

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
	enum class EPage : uint8 { Home, Profile, Join, Settings, Story, HowTo, Hall, Credits };

	TSharedRef<SWidget> MakeHome();
	TSharedRef<SWidget> MakeProfile();
	TSharedRef<SWidget> MakeJoin();
	TSharedRef<SWidget> MakeStory();
	TSharedRef<SWidget> MakeHowTo();
	TSharedRef<SWidget> MakeHall();
	TSharedRef<SWidget> MakeCredits();
	TSharedRef<SWidget> PageBox(EPage Page, TSharedRef<SWidget> Content);
	/** "< value >" picker used for the date of birth. */
	TSharedRef<SWidget> MakeStepper(const FText& Label, TAttribute<FText> Value, TFunction<void(int32)> Step, float Width);
	void RebuildLobbyList();
	void RebuildHall();

	void Go(EPage Page);
	void StartStory();
	void StopStory();
	/** Checks the profile form; empty when it can be saved. */
	FText ProfileProblem() const;
	void SaveProfile();
	void JoinWithCode();

	UAIBGameInstance* GI() const;
	UAIBSessionSubsystem* Sessions() const;

	TWeakObjectPtr<AAIBMenuPlayerController> Owner;
	EPage Current = EPage::Home;
	TSharedPtr<SVerticalBox> LobbyList;
	TSharedPtr<SVerticalBox> HallList;
	TSharedPtr<SEditableTextBox> NameBox;
	TSharedPtr<SEditableTextBox> CodeBox;
	FString DirectAddress;
	FString RoomCode;
	FText JoinError;
	int32 ShownResults = -1;
	bool bWasSearching = false;

	// Profile form
	FString DraftName;
	int32 DraftDay = 1;
	int32 DraftMonth = 1;
	int32 DraftYear = 2000;
	bool bProfileTouched = false;

	// Story replay
	double StoryStart = 0.0;
	int32 StoryBeat = INDEX_NONE;

	FText MenuMessage;
	double MenuMessageTime = -100.0;
};
