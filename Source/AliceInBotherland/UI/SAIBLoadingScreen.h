// Alice in Botherland - loading screen and the shared settings panel

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Core/AIBSaveGame.h"

class ALICEINBOTHERLAND_API SAIBLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class UAIBGameInstance;

class ALICEINBOTHERLAND_API SAIBSettingsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBSettingsPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UAIBGameInstance>, GameInstance)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> MakeSlider(const FText& Label, float FAIBSettings::* Field, float Min, float Max, bool bPercent);
	TSharedRef<SWidget> MakeToggle(const FText& Label, bool FAIBSettings::* Field);
	TSharedRef<SWidget> MakeQualityRow();
	void Commit(bool bSave);

	TWeakObjectPtr<UAIBGameInstance> GameInstance;
	FAIBSettings Working;
	FSimpleDelegate OnClose;
};
