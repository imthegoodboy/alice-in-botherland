// Alice in Botherland - reusable Slate widgets

#include "UI/SAIBWidgets.h"
#include "UI/AIBStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

// ---------------------------------------------------------------------------------------------------------------------
// SAIBButton
// ---------------------------------------------------------------------------------------------------------------------

void SAIBButton::Construct(const FArguments& InArgs)
{
	Kind = InArgs._Kind;
	Accent = InArgs._AccentColor;
	OnClickedDelegate = InArgs._OnClicked;

	// Fully transparent button frame: the visuals come from the border below so they can animate.
	Style = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(FMargin(0.f))
		.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));

	const bool bGhost = Kind == EAIBButtonKind::Ghost;
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (InArgs._Icon)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SBox).WidthOverride(InArgs._FontSize * 1.3f).HeightOverride(InArgs._FontSize * 1.3f)
			[
				SNew(SImage).Image(InArgs._Icon)
			]
		];
	}
	TSharedRef<SVerticalBox> TextColumn = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(InArgs._Text)
			.Font(AIBStyle::Heading(InArgs._FontSize))
			.ColorAndOpacity_Lambda([this]()
			{
				const bool bDark = Kind == EAIBButtonKind::Gold;
				const FLinearColor Base = bDark ? FLinearColor(0.05f, 0.03f, 0.01f) : AIBStyle::Text();
				return FSlateColor(IsEnabled() ? Base : AIBStyle::TextFaint());
			})
		];
	if (InArgs._SubText.IsSet())
	{
		TextColumn->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(InArgs._SubText)
			.Font(AIBStyle::Body(FMath::Max(11, InArgs._FontSize / 2 + 2)))
			.ColorAndOpacity(Kind == EAIBButtonKind::Gold ? FLinearColor(0.15f, 0.09f, 0.02f) : AIBStyle::TextDim())
		];
	}
	Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)[TextColumn];

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(InArgs._MinWidth)
		.MinDesiredHeight(InArgs._Height)
		[
			SNew(SButton)
			.ButtonStyle(&Style)
			.OnClicked(this, &SAIBButton::HandleClicked)
			.ContentPadding(FMargin(0.f))
			.IsFocusable(false)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::RoundBrush(6.f))
					.BorderBackgroundColor(this, &SAIBButton::GetFill)
					.Padding(0.f)
				]
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 6.f, 1.5f))
					.BorderBackgroundColor(this, &SAIBButton::GetBorder)
					.Padding(FMargin(bGhost ? 8.f : 22.f, 8.f))
					.VAlign(VAlign_Center)
					[
						Row
					]
				]
			]
		]
	];
}

FLinearColor SAIBButton::BaseColor() const
{
	const FLinearColor AccentColor = Accent.Get();
	if (AccentColor.A > 0.f)
	{
		return AccentColor;
	}
	switch (Kind)
	{
	case EAIBButtonKind::Primary: return AIBStyle::Blood();
	case EAIBButtonKind::Gold: return AIBStyle::Gold();
	case EAIBButtonKind::Ghost: return FLinearColor::Transparent;
	default: return FLinearColor(0.06f, 0.06f, 0.07f, 0.85f);
	}
}

FSlateColor SAIBButton::GetFill() const
{
	FLinearColor Base = BaseColor();
	if (!IsEnabled())
	{
		return FLinearColor(0.04f, 0.04f, 0.045f, 0.7f);
	}
	if (Kind == EAIBButtonKind::Ghost)
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.06f * Hover);
	}
	// Brighten on hover.
	Base = FMath::Lerp(Base, Base * 1.35f + FLinearColor(0.04f, 0.04f, 0.04f, 0.f), Hover);
	Base.A = FMath::Clamp(Base.A, 0.f, 1.f);
	return Base;
}

FSlateColor SAIBButton::GetBorder() const
{
	if (!IsEnabled())
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.05f);
	}
	if (Kind == EAIBButtonKind::Ghost)
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.f);
	}
	const FLinearColor Glow = Kind == EAIBButtonKind::Secondary ? AIBStyle::Blood() : FLinearColor::White;
	return FLinearColor(Glow.R, Glow.G, Glow.B, 0.12f + 0.55f * Hover);
}

void SAIBButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	bHovered = true;
	if (IsEnabled())
	{
		AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
	}
}

void SAIBButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	bHovered = false;
}

void SAIBButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Hover = FMath::FInterpTo(Hover, bHovered && IsEnabled() ? 1.f : 0.f, InDeltaTime, 14.f);
}

FReply SAIBButton::HandleClicked()
{
	AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
	return OnClickedDelegate.IsBound() ? OnClickedDelegate.Execute() : FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBTimerRing
// ---------------------------------------------------------------------------------------------------------------------

void SAIBTimerRing::Construct(const FArguments& InArgs)
{
	Fraction = InArgs._Fraction;
	Color = InArgs._Color;
	Label = InArgs._Label;
	Size = InArgs._Size;
}

int32 SAIBTimerRing::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;
	const float Radius = FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f - 5.f;
	const float Thickness = FMath::Max(3.f, Size * 0.055f);
	const int32 Segments = 72;

	auto DrawArc = [&](float FromFrac, float ToFrac, const FLinearColor& ArcColor, int32 Layer)
	{
		TArray<FVector2D> Points;
		const int32 Steps = FMath::Max(2, FMath::CeilToInt(Segments * (ToFrac - FromFrac)));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float T = FMath::Lerp(FromFrac, ToFrac, float(Index) / Steps);
			const float Angle = -PI * 0.5f + T * 2.f * PI;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, ArcColor, true, Thickness);
	};

	const float F = FMath::Clamp(Fraction.Get(1.f), 0.f, 1.f);
	DrawArc(0.f, 1.f, FLinearColor(1.f, 1.f, 1.f, 0.08f), LayerId);
	if (F > 0.001f)
	{
		DrawArc(0.f, F, Color.Get(FLinearColor::White), LayerId + 1);
	}

	const FSlateFontInfo Font = AIBStyle::Title(FMath::RoundToInt(Size * 0.42f));
	const FText Text = Label.Get(FText::GetEmpty());
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = Measure->Measure(Text, Font);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(Center - TextSize * 0.5f + FVector2D(0.f, 2.f)))),
		Text, Font, ESlateDrawEffect::None, AIBStyle::Text());
	return LayerId + 3;
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBGradient
// ---------------------------------------------------------------------------------------------------------------------

void SAIBGradient::Construct(const FArguments& InArgs)
{
	Colors = InArgs._Colors;
	bVertical = InArgs._Vertical;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SAIBGradient::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Colors.Num() < 2)
	{
		return LayerId;
	}
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	TArray<FSlateGradientStop> Stops;
	for (int32 Index = 0; Index < Colors.Num(); ++Index)
	{
		const float T = float(Index) / (Colors.Num() - 1);
		FLinearColor Color = Colors[Index];
		Color.A *= Opacity;
		// Slate's "horizontal" gradient has horizontal bands, i.e. its stops run down the Y axis.
		Stops.Add(FSlateGradientStop(bVertical ? FVector2D(0.f, LocalSize.Y * T) : FVector2D(LocalSize.X * T, 0.f), Color));
	}
	FSlateDrawElement::MakeGradient(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Stops, bVertical ? Orient_Horizontal : Orient_Vertical);
	return LayerId + 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

namespace AIBUI
{
	TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, const FMargin& Padding, bool bSolid)
	{
		return SNew(SBorder)
			.BorderImage(bSolid ? AIBStyle::PanelSolidBrush() : AIBStyle::PanelBrush())
			.Padding(Padding)
			[
				Content
			];
	}

	TSharedRef<SWidget> Label(const TAttribute<FText>& Text, const FSlateFontInfo& Font, const TAttribute<FSlateColor>& Color, bool bWrap, ETextJustify::Type Justify)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(Font)
			.ColorAndOpacity(Color)
			.AutoWrapText(bWrap)
			.Justification(Justify);
	}

	TSharedRef<SWidget> Header(const FText& Text)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
			[
				SNew(SBox).WidthOverride(14.f).HeightOverride(3.f)
				[
					SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Blood())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Text).Font(AIBStyle::SubHeading(15)).ColorAndOpacity(AIBStyle::TextDim())
			];
	}

	TSharedRef<SWidget> Divider()
	{
		return SNew(SBox).HeightOverride(1.f)
			[
				SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Line())
			];
	}

	TSharedRef<SWidget> TalkDot(const TAttribute<bool>& bTalking, float Size)
	{
		return SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			.Visibility_Lambda([bTalking]() { return bTalking.Get() ? EVisibility::HitTestInvisible : EVisibility::Hidden; })
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::RoundBrush(Size * 0.5f))
				.BorderBackgroundColor_Lambda([]()
				{
					FLinearColor C = AIBStyle::Acid();
					C.A = 0.65f + 0.35f * FMath::Sin(FPlatformTime::Seconds() * 9.0);
					return FSlateColor(C);
				})
			];
	}

	TSharedRef<SWidget> Bar(const TAttribute<float>& Fraction, const TAttribute<FLinearColor>& Color, float Height)
	{
		return SNew(SBox).HeightOverride(Height)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(AIBStyle::RoundBrush(Height * 0.5f)).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.08f))
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Fraction]() { return FMath::Clamp(Fraction.Get(), 0.001f, 1.f); }))
					[
						SNew(SBorder).BorderImage(AIBStyle::RoundBrush(Height * 0.5f)).BorderBackgroundColor_Lambda([Color]() { return FSlateColor(Color.Get()); })
					]
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Fraction]() { return FMath::Clamp(1.f - Fraction.Get(), 0.f, 1.f); }))
					[
						SNullWidget::NullWidget
					]
				]
			];
	}
}
