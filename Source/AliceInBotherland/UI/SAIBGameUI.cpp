// Alice in Botherland - game-style widgets: slanted buttons, icon buttons, sliders, switches, scroll wheels, reveals

#include "UI/SAIBGameUI.h"
#include "UI/AIBStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Core/AIBGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

namespace
{
	FButtonStyle InvisibleButtonStyle()
	{
		return FButtonStyle()
			.SetNormal(FSlateNoResource())
			.SetHovered(FSlateNoResource())
			.SetPressed(FSlateNoResource())
			.SetDisabled(FSlateNoResource())
			.SetNormalPadding(FMargin(0.f))
			.SetPressedPadding(FMargin(0.f));
	}

	const FSlateBrush* NoBrush()
	{
		static const FSlateNoResource Brush;
		return &Brush;
	}

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}

	float EaseOut(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - T, 3.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBSlantButton
// ---------------------------------------------------------------------------------------------------------------------

void SAIBSlantButton::Construct(const FArguments& InArgs)
{
	Kind = InArgs._Kind;
	Selected = InArgs._IsSelected;
	Accent = InArgs._AccentColor;
	OnClickedDelegate = InArgs._OnClicked;
	Style = InvisibleButtonStyle();

	const int32 Size = InArgs._FontSize;
	const bool bCentered = InArgs._Centered;
	const FSlateRenderTransform Shear(FShear2D(-InArgs._Slant, 0.f));
	const float SidePad = InArgs._Slant * InArgs._Height * 0.5f + (bCentered ? 6.f : 18.f);

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (InArgs._Icon)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::IconFont(FMath::RoundToInt(Size * 1.2f)))
			.Text(AIBIcon::Glyph(InArgs._Icon))
			.ColorAndOpacity(this, &SAIBSlantButton::GetTextColor)
		];
	}
	TSharedRef<SVerticalBox> Words = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(bCentered ? HAlign_Center : HAlign_Left)
		[
			SNew(STextBlock)
			.Text(InArgs._Text)
			.Font(AIBStyle::Spaced(AIBStyle::Heading(Size), 50))
			.ColorAndOpacity(this, &SAIBSlantButton::GetTextColor)
		];
	if (InArgs._SubText.IsSet())
	{
		Words->AddSlot().AutoHeight().HAlign(bCentered ? HAlign_Center : HAlign_Left).Padding(0.f, -1.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(InArgs._SubText)
			.Visibility_Lambda([Sub = InArgs._SubText]() { return Sub.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			.Font(AIBStyle::Body(FMath::Max(12, FMath::RoundToInt(Size * 0.56f))))
			.ColorAndOpacity_Lambda([this]()
			{
				FLinearColor C = GetTextColor().GetSpecifiedColor();
				C.A *= 0.72f;
				return FSlateColor(C);
			})
		];
	}
	Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(bCentered ? HAlign_Center : HAlign_Fill)[Words];
	if (Kind == EAIBButtonKind::Primary && !bCentered)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(AIBStyle::IconFont(FMath::RoundToInt(Size * 1.3f)))
			.Text(AIBIcon::Glyph(AIBIcon::Chevron))
			.ColorAndOpacity(this, &SAIBSlantButton::GetTextColor)
			.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(6.f * Hover, 0.f))); })
		];
	}

	// Primary and gold faces are two-tone; the rest are flat glass.
	TArray<FLinearColor> Ramp;
	if (Kind == EAIBButtonKind::Primary)
	{
		Ramp = { FLinearColor(0.98f, 0.24f, 0.1f), FLinearColor(0.62f, 0.02f, 0.05f) };
	}
	else if (Kind == EAIBButtonKind::Gold)
	{
		Ramp = { FLinearColor(1.f, 0.84f, 0.45f), FLinearColor(0.86f, 0.55f, 0.16f) };
	}

	TSharedRef<SOverlay> Face = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(this, &SAIBSlantButton::GetFill)
		];
	if (Ramp.Num() > 0)
	{
		Face->AddSlot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return IsEnabled() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SAIBGradient).Colors(Ramp).Vertical(false)
			]
		];
		// Brighten on hover.
		Face->AddSlot()
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor_Lambda([this]() { return FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.14f * Hover + 0.1f * Press)); })
		];
	}
	if (InArgs._Shine)
	{
		Face->AddSlot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return IsEnabled() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SAIBShine)
			]
		];
	}
	Face->AddSlot().HAlign(HAlign_Left)
	[
		SNew(SBox)
		.WidthOverride_Lambda([this]() { return FOptionalSize(Selected.Get() ? 7.f : 3.f + 6.f * Hover); })
		[
			SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(this, &SAIBSlantButton::GetEdge)
		]
	];
	Face->AddSlot()
	[
		SNew(SBorder).BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f)).BorderBackgroundColor(this, &SAIBSlantButton::GetOutline)
	];

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(InArgs._MinWidth)
		.HeightOverride(InArgs._Height)
		[
			SNew(SButton)
			.ButtonStyle(&Style)
			.OnClicked(this, &SAIBSlantButton::HandleClicked)
			.OnPressed_Lambda([this]() { Press = 1.f; })
			.ContentPadding(FMargin(0.f))
			.IsFocusable(false)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
					.RenderTransform(Shear)
					[
						Face
					]
				]
				+ SOverlay::Slot().Padding(FMargin(SidePad, 0.f, SidePad, 0.f)).VAlign(VAlign_Center)
				[
					SNew(SBox)
					.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(5.f * Hover, Press * 1.5f))); })
					[
						Row
					]
				]
			]
		]
	];
}

void SAIBSlantButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Hover = FMath::FInterpTo(Hover, bHovered && IsEnabled() ? 1.f : 0.f, InDeltaTime, 16.f);
	Press = FMath::FInterpTo(Press, 0.f, InDeltaTime, 10.f);
}

void SAIBSlantButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	bHovered = true;
	if (IsEnabled())
	{
		AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
	}
}

void SAIBSlantButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	bHovered = false;
}

FReply SAIBSlantButton::HandleClicked()
{
	AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
	return OnClickedDelegate.IsBound() ? OnClickedDelegate.Execute() : FReply::Handled();
}

FLinearColor SAIBSlantButton::AccentOrDefault() const
{
	const FLinearColor Custom = Accent.Get();
	if (Custom.A > 0.f)
	{
		return Custom;
	}
	return Kind == EAIBButtonKind::Gold ? AIBStyle::Gold() : AIBStyle::Ember();
}

FSlateColor SAIBSlantButton::GetFill() const
{
	if (!IsEnabled())
	{
		return FLinearColor(0.05f, 0.05f, 0.06f, 0.75f);
	}
	if (Selected.Get())
	{
		return FMath::Lerp(FLinearColor(0.32f, 0.05f, 0.04f, 0.88f), FLinearColor(0.42f, 0.08f, 0.05f, 0.92f), Hover);
	}
	switch (Kind)
	{
	case EAIBButtonKind::Ghost:
		return FLinearColor(1.f, 1.f, 1.f, 0.07f * Hover);
	case EAIBButtonKind::Primary:
	case EAIBButtonKind::Gold:
		return FLinearColor::Transparent;
	default:
		return FMath::Lerp(AIBStyle::Glass(), FLinearColor(0.09f, 0.09f, 0.11f, 0.9f), Hover);
	}
}

FSlateColor SAIBSlantButton::GetEdge() const
{
	if (!IsEnabled() || Kind == EAIBButtonKind::Ghost)
	{
		return FLinearColor::Transparent;
	}
	if (Kind == EAIBButtonKind::Primary || Kind == EAIBButtonKind::Gold)
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.35f + 0.4f * Hover);
	}
	const FLinearColor A = AccentOrDefault();
	return WithAlpha(A, Selected.Get() ? 1.f : 0.45f + 0.55f * Hover);
}

FSlateColor SAIBSlantButton::GetOutline() const
{
	if (!IsEnabled())
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.05f);
	}
	if (Kind == EAIBButtonKind::Ghost)
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.18f * Hover);
	}
	if (Selected.Get())
	{
		return WithAlpha(AccentOrDefault(), 0.85f);
	}
	if (Kind == EAIBButtonKind::Primary || Kind == EAIBButtonKind::Gold)
	{
		return FLinearColor(1.f, 1.f, 1.f, 0.18f + 0.3f * Hover);
	}
	return FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.1f), WithAlpha(AccentOrDefault(), 0.7f), Hover);
}

FSlateColor SAIBSlantButton::GetTextColor() const
{
	if (!IsEnabled())
	{
		return AIBStyle::TextFaint();
	}
	if (Kind == EAIBButtonKind::Gold)
	{
		return FLinearColor(0.08f, 0.04f, 0.01f);
	}
	if (Kind == EAIBButtonKind::Primary || Selected.Get())
	{
		return FLinearColor::White;
	}
	return FMath::Lerp(AIBStyle::Text(), FLinearColor::White, Hover);
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBIconButton
// ---------------------------------------------------------------------------------------------------------------------

void SAIBIconButton::Construct(const FArguments& InArgs)
{
	Active = InArgs._IsActive;
	OnClickedDelegate = InArgs._OnClicked;
	Style = InvisibleButtonStyle();
	const float Size = InArgs._Size;
	TAttribute<TCHAR> IconAttr = InArgs._Icon;
	TAttribute<FText> Label = InArgs._Label;

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(&Style)
		.ContentPadding(FMargin(0.f))
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
			return OnClickedDelegate.IsBound() ? OnClickedDelegate.Execute() : FReply::Handled();
		})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(Size)
				.HeightOverride(Size)
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(1.f + 0.06f * Hover)); })
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBox)
						.RenderTransformPivot(FVector2D(0.5f, 0.5f))
						.RenderTransform(FSlateRenderTransform(FShear2D(-0.16f, 0.f)))
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBorder)
								.BorderImage(AIBStyle::WhiteBrush())
								.BorderBackgroundColor_Lambda([this]()
								{
									if (Active.Get())
									{
										return FSlateColor(FLinearColor(0.42f, 0.07f, 0.05f, 0.9f));
									}
									return FSlateColor(FMath::Lerp(AIBStyle::Glass(), FLinearColor(0.12f, 0.1f, 0.1f, 0.92f), Hover));
								})
							]
							+ SOverlay::Slot()
							[
								SNew(SBorder)
								.BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f))
								.BorderBackgroundColor_Lambda([this]()
								{
									return FSlateColor(Active.Get() ? AIBStyle::Ember() : FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.14f), AIBStyle::Gold(), Hover));
								})
							]
						]
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(AIBStyle::IconFont(FMath::RoundToInt(Size * 0.5f)))
						.Text_Lambda([IconAttr]() { return AIBIcon::Glyph(IconAttr.Get()); })
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(AIBStyle::Text(), AIBStyle::Gold(), Hover)); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([Label]() { return Label.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				.Text(Label)
				.Font(AIBStyle::Spaced(AIBStyle::SubHeading(12), 120))
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(FMath::Lerp(AIBStyle::TextDim(), AIBStyle::Gold(), Hover)); })
			]
		]
	];
}

void SAIBIconButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Hover = FMath::FInterpTo(Hover, bHovered && IsEnabled() ? 1.f : 0.f, InDeltaTime, 16.f);
}

void SAIBIconButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	bHovered = true;
	AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
}

void SAIBIconButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	bHovered = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBReveal
// ---------------------------------------------------------------------------------------------------------------------

void SAIBReveal::Construct(const FArguments& InArgs)
{
	Delay = InArgs._Delay;
	Duration = FMath::Max(0.05f, InArgs._Duration);
	Offset = InArgs._Offset;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(NoBrush())
		.Padding(0.f)
		.ColorAndOpacity_Lambda([this]() { return FLinearColor(1.f, 1.f, 1.f, Progress()); })
		.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(Offset * (1.f - EaseOut(Progress())))); })
		[
			InArgs._Content.Widget
		]
	];
}

void SAIBReveal::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Hidden widgets don't tick: a gap means the page was just (re)opened, so play the entrance again.
	if (InCurrentTime - LastTick > 0.3)
	{
		Start = InCurrentTime;
	}
	LastTick = InCurrentTime;
	Now = InCurrentTime;
}

float SAIBReveal::Progress() const
{
	if (Start < 0.0)
	{
		return 0.f;
	}
	return FMath::Clamp(float(Now - Start - Delay) / Duration, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBSlider
// ---------------------------------------------------------------------------------------------------------------------

void SAIBSlider::Construct(const FArguments& InArgs)
{
	Value = InArgs._Value;
	MinValue = InArgs._MinValue;
	MaxValue = FMath::Max(InArgs._MaxValue, InArgs._MinValue + KINDA_SMALL_NUMBER);
	StepSize = InArgs._StepSize;
	Width = InArgs._Width;
	OnValueChanged = InArgs._OnValueChanged;
}

float SAIBSlider::Fraction() const
{
	return FMath::Clamp((Value.Get() - MinValue) / (MaxValue - MinValue), 0.f, 1.f);
}

int32 SAIBSlider::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Knob = IsHovered() || HasMouseCapture() ? 22.f : 18.f;
	const float Pad = 11.f;
	const float TrackW = FMath::Max(1.f, float(Size.X) - Pad * 2.f);
	const float TrackH = 6.f;
	const float Y = float(Size.Y) * 0.5f;
	const float F = Fraction();
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A * (bParentEnabled ? 1.f : 0.4f);

	auto Box = [&](float X0, float Y0, float W, float H, const FSlateBrush* Brush, const FLinearColor& Color, int32 Layer)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X0, Y0))), Brush, ESlateDrawEffect::None, WithAlpha(Color, Color.A * Alpha));
	};
	// Track, filled part, knob (ring + core).
	Box(Pad, Y - TrackH * 0.5f, TrackW, TrackH, AIBStyle::RoundBrush(3.f), FLinearColor(1.f, 1.f, 1.f, 0.12f), LayerId);
	if (F > 0.001f)
	{
		TArray<FSlateGradientStop> Stops = { FSlateGradientStop(FVector2D(0.f, 0.f), WithAlpha(AIBStyle::Gold(), Alpha)), FSlateGradientStop(FVector2D(TrackW * F, 0.f), WithAlpha(AIBStyle::Ember(), Alpha)) };
		FSlateDrawElement::MakeGradient(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(TrackW * F, TrackH), FSlateLayoutTransform(FVector2f(Pad, Y - TrackH * 0.5f))), Stops, Orient_Vertical, ESlateDrawEffect::None, FVector4f(3.f, 3.f, 3.f, 3.f));
	}
	const float KX = Pad + TrackW * F - Knob * 0.5f;
	Box(KX - 3.f, Y - Knob * 0.5f - 3.f, Knob + 6.f, Knob + 6.f, AIBStyle::RoundBrush((Knob + 6.f) * 0.5f), WithAlpha(AIBStyle::Ember(), IsHovered() ? 0.45f : 0.25f), LayerId + 2);
	Box(KX, Y - Knob * 0.5f, Knob, Knob, AIBStyle::RoundBrush(Knob * 0.5f), FLinearColor(0.97f, 0.95f, 0.92f, 1.f), LayerId + 3);
	return LayerId + 4;
}

void SAIBSlider::Commit(float NewValue)
{
	if (StepSize > 0.f)
	{
		NewValue = MinValue + FMath::RoundToFloat((NewValue - MinValue) / StepSize) * StepSize;
	}
	NewValue = FMath::Clamp(NewValue, MinValue, MaxValue);
	if (!FMath::IsNearlyEqual(NewValue, Value.Get()))
	{
		OnValueChanged.ExecuteIfBound(NewValue);
	}
}

void SAIBSlider::SetFromPosition(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float Pad = 11.f;
	const float TrackW = FMath::Max(1.f, float(MyGeometry.GetLocalSize().X) - Pad * 2.f);
	Commit(MinValue + FMath::Clamp((float(Local.X) - Pad) / TrackW, 0.f, 1.f) * (MaxValue - MinValue));
}

FReply SAIBSlider::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !IsEnabled())
	{
		return FReply::Unhandled();
	}
	SetFromPosition(MyGeometry, MouseEvent);
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAIBSlider::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SAIBSlider::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		SetFromPosition(MyGeometry, MouseEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SAIBSlider::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!IsEnabled())
	{
		return FReply::Unhandled();
	}
	const float Notch = FMath::Max(StepSize, (MaxValue - MinValue) / 20.f);
	Commit(Value.Get() + (MouseEvent.GetWheelDelta() > 0.f ? Notch : -Notch));
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBToggle
// ---------------------------------------------------------------------------------------------------------------------

void SAIBToggle::Construct(const FArguments& InArgs)
{
	IsOn = InArgs._IsOn;
	OnToggled = InArgs._OnToggled;
	Knob = IsOn.Get() ? 1.f : 0.f;
}

void SAIBToggle::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Knob = FMath::FInterpTo(Knob, IsOn.Get() ? 1.f : 0.f, InDeltaTime, 18.f);
}

int32 SAIBToggle::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size(56.f, 28.f);
	const FVector2f Origin(float(AllottedGeometry.GetLocalSize().X) - Size.X, (float(AllottedGeometry.GetLocalSize().Y) - Size.Y) * 0.5f);
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A;
	const FLinearColor Track = FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.12f), FLinearColor(0.85f, 0.2f, 0.08f, 1.f), Knob);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Origin)), AIBStyle::RoundBrush(14.f), ESlateDrawEffect::None, WithAlpha(Track, Track.A * Alpha));
	const float D = 22.f;
	const FVector2f KnobPos(Origin.X + 3.f + Knob * (Size.X - D - 6.f), Origin.Y + 3.f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(D, D), FSlateLayoutTransform(KnobPos)), AIBStyle::RoundBrush(11.f), ESlateDrawEffect::None, FLinearColor(0.97f, 0.95f, 0.92f, Alpha * (IsHovered() ? 1.f : 0.92f)));
	return LayerId + 2;
}

FReply SAIBToggle::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !IsEnabled())
	{
		return FReply::Unhandled();
	}
	AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
	OnToggled.ExecuteIfBound(!IsOn.Get());
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBWheel
// ---------------------------------------------------------------------------------------------------------------------

void SAIBWheel::Construct(const FArguments& InArgs)
{
	NumItems = InArgs._NumItems;
	Selected = InArgs._Selected;
	ItemText = InArgs._ItemText;
	OnSelect = InArgs._OnSelect;
	bWrap = InArgs._Wrap;
	const float W = InArgs._Width;

	// Seven rows (-3..3); the outer two are hidden by the clip and only show while the drum turns.
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Offset = -3; Offset <= 3; ++Offset)
	{
		const int32 Distance = FMath::Abs(Offset);
		const FSlateFontInfo Font = Distance == 0 ? AIBStyle::Spaced(AIBStyle::Heading(30), 40) : AIBStyle::SubHeading(Distance == 1 ? 21 : 17);
		const float Alpha = Distance == 0 ? 1.f : Distance == 1 ? 0.55f : Distance == 2 ? 0.22f : 0.08f;
		Rows->AddSlot().AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(RowHeight)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(Font)
				.Text_Lambda([this, Offset]() { return RowText(Offset); })
				.ColorAndOpacity(Distance == 0 ? FLinearColor(1.f, 0.97f, 0.92f, 1.f) : FLinearColor(0.85f, 0.83f, 0.8f, Alpha))
			]
		];
	}

	auto Arrow = [this](TCHAR Glyph, int32 Steps)
	{
		return SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.IsFocusable(false)
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([this, Steps]() { Step(Steps); return FReply::Handled(); })
			[
				SNew(STextBlock).Font(AIBStyle::IconFont(24)).Text(AIBIcon::Glyph(Glyph)).ColorAndOpacity(FLinearColor(1.f, 0.74f, 0.32f, 0.85f))
			];
	};

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(W)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Arrow(AIBIcon::Up, -1)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(RowHeight * 5.f)
				.Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SOverlay)
					// The selection band.
					+ SOverlay::Slot().VAlign(VAlign_Center)
					[
						SNew(SBox).HeightOverride(RowHeight)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(FLinearColor(1.f, 0.36f, 0.12f, 0.16f))]
							+ SOverlay::Slot().VAlign(VAlign_Top)[SNew(SBox).HeightOverride(2.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Gold())]]
							+ SOverlay::Slot().VAlign(VAlign_Bottom)[SNew(SBox).HeightOverride(2.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Gold())]]
						]
					]
					+ SOverlay::Slot().VAlign(VAlign_Top)
					[
						SAssignNew(Drum, SBox)
						.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2D(0.f, (Anim - 1.f) * RowHeight))); })
						[
							Rows
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[Arrow(AIBIcon::Down, 1)]
		]
	];
}

FText SAIBWheel::RowText(int32 Offset) const
{
	const int32 Num = NumItems.Get();
	if (Num <= 0 || !ItemText)
	{
		return FText::GetEmpty();
	}
	int32 Index = Selected.Get() + Offset;
	if (bWrap)
	{
		Index = ((Index % Num) + Num) % Num;
	}
	else if (Index < 0 || Index >= Num)
	{
		return FText::GetEmpty();
	}
	return ItemText(Index);
}

void SAIBWheel::Step(int32 Steps)
{
	const int32 Num = NumItems.Get();
	if (Num <= 0 || Steps == 0)
	{
		return;
	}
	const int32 Old = Selected.Get();
	int32 New = Old + Steps;
	New = bWrap ? ((New % Num) + Num) % Num : FMath::Clamp(New, 0, Num - 1);
	if (New == Old)
	{
		return;
	}
	// The new value is drawn centred straight away; start the drum one row back and let it roll into place.
	Anim = FMath::Clamp(Anim + float(FMath::Sign(Steps)) * FMath::Min(FMath::Abs(Steps), 2), -2.f, 2.f);
	AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
	OnSelect.ExecuteIfBound(New);
}

void SAIBWheel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Anim = FMath::FInterpTo(Anim, 0.f, InDeltaTime, 14.f);
}

FReply SAIBWheel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Step(MouseEvent.GetWheelDelta() > 0.f ? -1 : 1);
	return FReply::Handled();
}

FReply SAIBWheel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = true;
	DragStartY = float(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).Y);
	DragSteps = 0.f;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAIBWheel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDragging || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}
	// Dragging the drum: every row's height of travel moves one value (drag up = later values).
	const float Y = float(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).Y);
	const int32 Wanted = FMath::TruncToInt((DragStartY - Y) / (RowHeight * 0.8f));
	if (Wanted != int32(DragSteps))
	{
		Step(Wanted - int32(DragSteps));
		DragSteps = float(Wanted);
	}
	return FReply::Handled();
}

FReply SAIBWheel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDragging)
	{
		return FReply::Unhandled();
	}
	bDragging = false;
	if (FMath::IsNearlyZero(DragSteps))
	{
		// A click without a drag: pick the row that was clicked (relative to the drum's centre).
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (Drum.IsValid())
		{
			const FGeometry& DrumGeo = Drum->GetTickSpaceGeometry();
			const FVector2D DrumLocal = DrumGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const int32 Row = FMath::FloorToInt(float(DrumLocal.Y) / RowHeight) - 3;
			if (FMath::Abs(Row) <= 2)
			{
				Step(Row);
			}
		}
		(void)Local;
	}
	return FReply::Handled().ReleaseMouseCapture();
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBCorners / SAIBShine
// ---------------------------------------------------------------------------------------------------------------------

void SAIBCorners::Construct(const FArguments& InArgs)
{
	Color = InArgs._Color;
	Length = InArgs._Length;
	Thickness = InArgs._Thickness;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SAIBCorners::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D S = AllottedGeometry.GetLocalSize();
	FLinearColor C = Color.Get(FLinearColor(1.f, 0.74f, 0.32f, 0.9f));
	C.A *= InWidgetStyle.GetColorAndOpacityTint().A;
	const float L = Length;
	const float H = Thickness * 0.5f;
	auto Corner = [&](const FVector2D& P, const FVector2D& DX, const FVector2D& DY)
	{
		const TArray<FVector2D> Points = { P + DY * L, P, P + DX * L };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, Thickness);
	};
	Corner(FVector2D(H, H), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f));
	Corner(FVector2D(S.X - H, H), FVector2D(-1.f, 0.f), FVector2D(0.f, 1.f));
	Corner(FVector2D(H, S.Y - H), FVector2D(1.f, 0.f), FVector2D(0.f, -1.f));
	Corner(FVector2D(S.X - H, S.Y - H), FVector2D(-1.f, 0.f), FVector2D(0.f, -1.f));
	return LayerId + 1;
}

void SAIBShine::Construct(const FArguments& InArgs)
{
	Period = FMath::Max(1.f, InArgs._Period);
	Seed = FMath::FRand() * Period;
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SAIBShine::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const float Sweep = 0.85f;
	const float Phase = FMath::Fmod(float(FPlatformTime::Seconds()) + Seed, Period) / Sweep;
	if (Phase > 1.f)
	{
		return LayerId;
	}
	const FVector2D S = AllottedGeometry.GetLocalSize();
	const float Band = float(S.Y) * 1.4f;
	const float X = -Band + Phase * (float(S.X) + Band * 2.f);
	const float A = 0.3f * InWidgetStyle.GetColorAndOpacityTint().A;
	TArray<FSlateGradientStop> Stops =
	{
		FSlateGradientStop(FVector2D(X, 0.f), FLinearColor(1.f, 1.f, 1.f, 0.f)),
		FSlateGradientStop(FVector2D(X + Band * 0.5f, 0.f), FLinearColor(1.f, 1.f, 1.f, A)),
		FSlateGradientStop(FVector2D(X + Band, 0.f), FLinearColor(1.f, 1.f, 1.f, 0.f))
	};
	FSlateDrawElement::MakeGradient(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Stops, Orient_Vertical);
	return LayerId + 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// SAIBFeatureCard / SAIBFpsCounter
// ---------------------------------------------------------------------------------------------------------------------

void SAIBFeatureCard::Construct(const FArguments& InArgs)
{
	OnClickedDelegate = InArgs._OnClicked;
	Style = InvisibleButtonStyle();
	const FVector2D Size = InArgs._Size;
	TSharedRef<SHorizontalBox> Bottom = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(InArgs._Kicker).Font(AIBStyle::Spaced(AIBStyle::SubHeading(14), 260)).ColorAndOpacity(AIBStyle::Gold())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, -2.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text(InArgs._Title).Font(AIBStyle::Spaced(AIBStyle::Title(44), 20)).ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D(1.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(InArgs._Body).Font(AIBStyle::Body(14)).ColorAndOpacity(FLinearColor(0.9f, 0.88f, 0.85f, 0.85f)).AutoWrapText(true)
			]
		];
	if (InArgs._Icon)
	{
		Bottom->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(12.f, 0.f, 0.f, 4.f)
		[
			SNew(SBox)
			.WidthOverride(52.f)
			.HeightOverride(52.f)
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(1.f + 0.12f * Hover)); })
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SBorder).BorderImage(AIBStyle::RoundBrush(26.f)).BorderBackgroundColor_Lambda([this]() { return FSlateColor(FMath::Lerp(FLinearColor(0.85f, 0.2f, 0.08f, 0.85f), FLinearColor(1.f, 0.36f, 0.12f, 1.f), Hover)); })]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[AIBUI::Icon(InArgs._Icon, 30, FLinearColor::White)]
			]
		];
	}

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(&Style)
		.ContentPadding(FMargin(0.f))
		.IsFocusable(false)
		.OnClicked_Lambda([this]()
		{
			AIBStyle::PlayUISound(TEXT("SFX_UI_Click"));
			return OnClickedDelegate.IsBound() ? OnClickedDelegate.Execute() : FReply::Handled();
		})
		[
			SNew(SBox)
			.WidthOverride(Size.X)
			.HeightOverride(Size.Y)
			.Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFill)
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
					.RenderTransform_Lambda([this]() { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(1.03f + 0.06f * Hover)); })
					[
						SNew(SImage).Image(AIBStyle::StoryImage(InArgs._Image))
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.05f), FLinearColor(0.f, 0.f, 0.f, 0.25f), FLinearColor(0.f, 0.f, 0.f, 0.88f) })
				]
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f))
					.BorderBackgroundColor_Lambda([this]() { return FSlateColor(FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.12f), AIBStyle::Gold(), Hover)); })
				]
				+ SOverlay::Slot()
				[
					SNew(SAIBCorners).Color_Lambda([this]() { return FMath::Lerp(FLinearColor(1.f, 0.74f, 0.32f, 0.5f), FLinearColor(1.f, 0.74f, 0.32f, 1.f), Hover); })
				]
				+ SOverlay::Slot().Padding(FMargin(22.f, 16.f))[Bottom]
			]
		]
	];
}

void SAIBFeatureCard::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Hover = FMath::FInterpTo(Hover, bHovered ? 1.f : 0.f, InDeltaTime, 10.f);
}

void SAIBFeatureCard::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	bHovered = true;
	AIBStyle::PlayUISound(TEXT("SFX_UI_Hover"));
}

void SAIBFeatureCard::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	bHovered = false;
}

void SAIBFpsCounter::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Visibility_Lambda([]()
		{
			const UAIBGameInstance* GI = GEngine && GEngine->GameViewport ? Cast<UAIBGameInstance>(GEngine->GameViewport->GetGameInstance()) : nullptr;
			return GI && const_cast<UAIBGameInstance*>(GI)->GetSettings().bShowFPS ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		})
		.BorderImage(AIBStyle::RoundBrush(4.f))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f))
		.Padding(FMargin(8.f, 2.f))
		[
			SNew(STextBlock)
			.Font(AIBStyle::Spaced(AIBStyle::Heading(15), 60))
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(Smoothed))); })
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(Smoothed >= 50.f ? AIBStyle::Acid() : Smoothed >= 30.f ? AIBStyle::Gold() : AIBStyle::Blood()); })
		]
	];
}

void SAIBFpsCounter::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (InDeltaTime > KINDA_SMALL_NUMBER)
	{
		Smoothed = FMath::Lerp(Smoothed, 1.f / InDeltaTime, FMath::Clamp(InDeltaTime * 2.f, 0.f, 1.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

namespace AIBUI
{
	TSharedRef<SWidget> Frame(TSharedRef<SWidget> Content, const FMargin& Padding, const FLinearColor& Accent, float Opacity)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(AIBStyle::WhiteBrush())
				.BorderBackgroundColor(FLinearColor(0.012f, 0.013f, 0.018f, Opacity))
				.Padding(Padding)
				[
					Content
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(AIBStyle::OutlineBrush(FLinearColor::White, 0.f, 1.f)).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.07f))
			]
			+ SOverlay::Slot()
			[
				SNew(SAIBCorners).Color(Accent)
			];
	}

	TSharedRef<SWidget> PageTitle(const FText& Kicker, const TAttribute<FText>& Title, int32 TitleSize)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
				[
					SNew(SBox).WidthOverride(26.f).HeightOverride(3.f)[SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Ember())]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(Kicker).Font(AIBStyle::Spaced(AIBStyle::SubHeading(17), 260)).ColorAndOpacity(AIBStyle::Gold())
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(AIBStyle::Spaced(AIBStyle::Title(TitleSize), 20))
				.ColorAndOpacity(FLinearColor(0.97f, 0.95f, 0.91f))
				.ShadowOffset(FVector2D(2.f, 3.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.55f))
			];
	}

	TSharedRef<SWidget> Icon(TCHAR Code, int32 Size, const TAttribute<FSlateColor>& Color)
	{
		return SNew(STextBlock).Font(AIBStyle::IconFont(Size)).Text(AIBIcon::Glyph(Code)).ColorAndOpacity(Color);
	}

	TSharedRef<SWidget> Pill(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Color, int32 FontSize)
	{
		return SNew(SBorder)
			.BorderImage(AIBStyle::RoundBrush(12.f))
			.BorderBackgroundColor_Lambda([Color]()
			{
				FLinearColor C = Color.Get().GetSpecifiedColor();
				C.A = 0.18f;
				return FSlateColor(C);
			})
			.Padding(FMargin(12.f, 3.f))
			[
				SNew(STextBlock).Text(Text).Font(AIBStyle::Spaced(AIBStyle::Heading(FontSize), 120)).ColorAndOpacity(Color)
			];
	}

	TSharedRef<SWidget> ArtBackdrop(FName Image, float LeftShade)
	{
		return SNew(SOverlay)
			.Visibility(EVisibility::HitTestInvisible)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(AIBStyle::WhiteBrush()).BorderBackgroundColor(AIBStyle::Ink())
			]
			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				[
					SNew(SImage).Image(AIBStyle::StoryImage(Image))
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SAIBGradient).Vertical(false).Colors({ FLinearColor(0.f, 0.f, 0.f, LeftShade), FLinearColor(0.f, 0.f, 0.f, LeftShade * 0.75f), FLinearColor(0.f, 0.f, 0.f, 0.1f) })
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBox).HeightOverride(320.f)
				[
					SNew(SAIBGradient).Colors({ FLinearColor(0.f, 0.f, 0.f, 0.f), FLinearColor(0.f, 0.f, 0.f, 0.7f) })
				]
			];
	}
}
