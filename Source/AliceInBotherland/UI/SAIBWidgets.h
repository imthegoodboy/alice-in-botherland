// Alice in Botherland - reusable Slate widgets

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateTypes.h"

enum class EAIBButtonKind : uint8
{
	Primary,   // blood red
	Secondary, // dark glass
	Gold,
	Ghost      // text only
};

/** Big readable button with hover animation and sounds. */
class ALICEINBOTHERLAND_API SAIBButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBButton)
		: _Kind(EAIBButtonKind::Secondary)
		, _FontSize(20)
		, _MinWidth(0.f)
		, _Height(52.f)
		, _AccentColor(FLinearColor::Transparent)
		, _Icon(nullptr)
	{}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FText, SubText)
		SLATE_ARGUMENT(EAIBButtonKind, Kind)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(float, MinWidth)
		SLATE_ARGUMENT(float, Height)
		SLATE_ATTRIBUTE(FLinearColor, AccentColor)
		SLATE_ARGUMENT(const FSlateBrush*, Icon)
		SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FReply HandleClicked();
	FSlateColor GetFill() const;
	FSlateColor GetBorder() const;
	FLinearColor BaseColor() const;

	EAIBButtonKind Kind = EAIBButtonKind::Secondary;
	TAttribute<FLinearColor> Accent;
	FOnClicked OnClickedDelegate;
	float Hover = 0.f;
	bool bHovered = false;
	FButtonStyle Style;
};

/** Circular countdown: ring that empties as time runs out, number in the middle. */
class ALICEINBOTHERLAND_API SAIBTimerRing : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBTimerRing)
		: _Size(96.f)
	{}
		SLATE_ATTRIBUTE(float, Fraction)
		SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ARGUMENT(float, Size)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size, Size); }

private:
	TAttribute<float> Fraction;
	TAttribute<FLinearColor> Color;
	TAttribute<FText> Label;
	float Size = 96.f;
};

/** Smooth colour ramp (evenly spaced stops), used to shade pictures behind text. */
class ALICEINBOTHERLAND_API SAIBGradient : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBGradient)
		: _Vertical(true)
	{}
		SLATE_ARGUMENT(TArray<FLinearColor>, Colors)
		/** True: the stops run top to bottom. False: left to right. */
		SLATE_ARGUMENT(bool, Vertical)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	TArray<FLinearColor> Colors;
	bool bVertical = true;
};

namespace AIBUI
{
	/** Glass panel with padding. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, const FMargin& Padding = FMargin(24.f), bool bSolid = false);
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Label(const TAttribute<FText>& Text, const FSlateFontInfo& Font, const TAttribute<FSlateColor>& Color, bool bWrap = false, ETextJustify::Type Justify = ETextJustify::Left);
	/** Small all-caps section header with a red tick. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Header(const FText& Text);
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Divider();
	/** Small pulsing green dot shown while a player's microphone is sending. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> TalkDot(const TAttribute<bool>& bTalking, float Size = 10.f);
	/** Horizontal progress bar. */
	ALICEINBOTHERLAND_API TSharedRef<SWidget> Bar(const TAttribute<float>& Fraction, const TAttribute<FLinearColor>& Color, float Height = 6.f);
}
