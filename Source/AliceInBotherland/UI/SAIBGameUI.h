// Alice in Botherland - game-style widgets: slanted buttons, icon buttons, sliders, switches, scroll wheels, reveals

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateTypes.h"
#include "UI/SAIBWidgets.h"

DECLARE_DELEGATE_OneParam(FOnAIBFloat, float);
DECLARE_DELEGATE_OneParam(FOnAIBBool, bool);
DECLARE_DELEGATE_OneParam(FOnAIBIndex, int32);

/**
 * The game's main button: a slanted face with an accent edge, an optional icon, a title and a subtitle. Hovering
 * slides the content and lights the edge; primary buttons get a moving shine.
 */
class ALICEINBOTHERLAND_API SAIBSlantButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBSlantButton)
		: _Kind(EAIBButtonKind::Secondary)
		, _Icon(0)
		, _FontSize(22)
		, _Height(56.f)
		, _MinWidth(0.f)
		, _Slant(0.18f)
		, _Shine(false)
		, _Centered(false)
		, _IsSelected(false)
		, _AccentColor(FLinearColor::Transparent)
	{}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FText, SubText)
		SLATE_ARGUMENT(EAIBButtonKind, Kind)
		SLATE_ARGUMENT(TCHAR, Icon)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(float, Height)
		SLATE_ARGUMENT(float, MinWidth)
		SLATE_ARGUMENT(float, Slant)
		SLATE_ARGUMENT(bool, Shine)
		SLATE_ARGUMENT(bool, Centered)
		SLATE_ATTRIBUTE(bool, IsSelected)
		SLATE_ATTRIBUTE(FLinearColor, AccentColor)
		SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:
	FReply HandleClicked();
	FLinearColor AccentOrDefault() const;
	FSlateColor GetFill() const;
	FSlateColor GetEdge() const;
	FSlateColor GetOutline() const;
	FSlateColor GetTextColor() const;

	EAIBButtonKind Kind = EAIBButtonKind::Secondary;
	TAttribute<bool> Selected;
	TAttribute<FLinearColor> Accent;
	FOnClicked OnClickedDelegate;
	float Hover = 0.f;
	float Press = 0.f;
	bool bHovered = false;
	FButtonStyle Style;
};

/** Square slanted icon button with a small label under it (top-bar navigation). */
class ALICEINBOTHERLAND_API SAIBIconButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBIconButton)
		: _Icon(0)
		, _Size(54.f)
		, _IsActive(false)
	{}
		SLATE_ATTRIBUTE(TCHAR, Icon)
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ARGUMENT(float, Size)
		SLATE_ATTRIBUTE(bool, IsActive)
		SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:
	TAttribute<bool> Active;
	FOnClicked OnClickedDelegate;
	float Hover = 0.f;
	bool bHovered = false;
	FButtonStyle Style;
};

/** Fades and slides its content in whenever it appears (each time its page opens), after an optional delay. */
class ALICEINBOTHERLAND_API SAIBReveal : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBReveal)
		: _Delay(0.f)
		, _Offset(FVector2D(-40.f, 0.f))
		, _Duration(0.5f)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(float, Delay)
		SLATE_ARGUMENT(FVector2D, Offset)
		SLATE_ARGUMENT(float, Duration)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	float Progress() const;

	float Delay = 0.f;
	float Duration = 0.5f;
	FVector2D Offset;
	double Start = -1.0;
	double LastTick = -10.0;
	double Now = 0.0;
};

/** Thick game slider: filled track, round knob, drag / click / mouse wheel. */
class ALICEINBOTHERLAND_API SAIBSlider : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBSlider)
		: _MinValue(0.f)
		, _MaxValue(1.f)
		, _StepSize(0.01f)
		, _Width(300.f)
	{}
		SLATE_ATTRIBUTE(float, Value)
		SLATE_ARGUMENT(float, MinValue)
		SLATE_ARGUMENT(float, MaxValue)
		SLATE_ARGUMENT(float, StepSize)
		SLATE_ARGUMENT(float, Width)
		SLATE_EVENT(FOnAIBFloat, OnValueChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Width, 28.f); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	void SetFromPosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void Commit(float NewValue);
	float Fraction() const;

	TAttribute<float> Value;
	float MinValue = 0.f;
	float MaxValue = 1.f;
	float StepSize = 0.01f;
	float Width = 300.f;
	FOnAIBFloat OnValueChanged;
};

/** On / off switch with a sliding knob. */
class ALICEINBOTHERLAND_API SAIBToggle : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBToggle) {}
		SLATE_ATTRIBUTE(bool, IsOn)
		SLATE_EVENT(FOnAIBBool, OnToggled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(60.f, 30.f); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TAttribute<bool> IsOn;
	FOnAIBBool OnToggled;
	float Knob = 0.f;
};

/** Scroll-wheel picker (like a phone's date picker): mouse wheel, drag, click a row, or the arrows. */
class ALICEINBOTHERLAND_API SAIBWheel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBWheel)
		: _Width(120.f)
		, _Wrap(true)
	{}
		SLATE_ATTRIBUTE(int32, NumItems)
		SLATE_ATTRIBUTE(int32, Selected)
		SLATE_ARGUMENT(TFunction<FText(int32)>, ItemText)
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(bool, Wrap)
		SLATE_EVENT(FOnAIBIndex, OnSelect)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	static constexpr float RowHeight = 40.f;

private:
	/** Moves the selection by Steps (wrapping or clamping) and animates the drum. */
	void Step(int32 Steps);
	FText RowText(int32 Offset) const;

	TAttribute<int32> NumItems;
	TAttribute<int32> Selected;
	TFunction<FText(int32)> ItemText;
	FOnAIBIndex OnSelect;
	bool bWrap = true;
	float Anim = 0.f;
	bool bDragging = false;
	float DragStartY = 0.f;
	float DragSteps = 0.f;
	TSharedPtr<SWidget> Drum;
};

/** Paints HUD-style corner brackets over whatever is under it. */
class ALICEINBOTHERLAND_API SAIBCorners : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBCorners)
		: _Length(20.f)
		, _Thickness(2.f)
	{}
		SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_ARGUMENT(float, Length)
		SLATE_ARGUMENT(float, Thickness)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	TAttribute<FLinearColor> Color;
	float Length = 20.f;
	float Thickness = 2.f;
};

/** A light band sweeping across (on primary buttons). */
class ALICEINBOTHERLAND_API SAIBShine : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBShine)
		: _Period(3.4f)
	{}
		SLATE_ARGUMENT(float, Period)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	float Period = 3.4f;
	float Seed = 0.f;
};

/** Picture card (story art, title, line of text) that zooms and lights up on hover. */
class ALICEINBOTHERLAND_API SAIBFeatureCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBFeatureCard)
		: _Size(FVector2D(420.f, 236.f))
		, _Icon(0)
	{}
		SLATE_ARGUMENT(FName, Image)
		SLATE_ARGUMENT(FText, Kicker)
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(FText, Body)
		SLATE_ARGUMENT(FVector2D, Size)
		SLATE_ARGUMENT(TCHAR, Icon)
		SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

private:
	FOnClicked OnClickedDelegate;
	float Hover = 0.f;
	bool bHovered = false;
	FButtonStyle Style;
};

/** Small frames-per-second readout (shown when the setting is on). */
class ALICEINBOTHERLAND_API SAIBFpsCounter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBFpsCounter) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	float Smoothed = 60.f;
};

namespace AIBUI
{
	/** Glass panel with corner brackets (the game's standard frame). */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Frame(TSharedRef<SWidget> Content, const FMargin& Padding = FMargin(28.f), const FLinearColor& Accent = FLinearColor(1.f, 0.74f, 0.32f, 0.9f), float Opacity = 0.78f);
	/** Small spaced kicker over a big title, with an accent rule under it. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> PageTitle(const FText& Kicker, const TAttribute<FText>& Title, int32 TitleSize = 72);
	/** Icon glyph as a text block. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Icon(TCHAR Code, int32 Size, const TAttribute<FSlateColor>& Color);
	/** Rounded status pill (READY, HOST, ...). */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Pill(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Color, int32 FontSize = 14);
	/** Full-screen story picture behind a page, shaded on the left (and bottom) so text reads over it. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> ArtBackdrop(FName Image, float LeftShade = 0.92f);
}
