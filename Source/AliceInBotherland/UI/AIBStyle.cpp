// Alice in Botherland - UI look: fonts, colours, brushes (pure Slate, files loaded at runtime)

#include "UI/AIBStyle.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Core/AIBAssets.h"

namespace AIBStyle
{
	FString UIRoot()
	{
		return FPaths::ProjectContentDir() / TEXT("AliceInBotherland/UI");
	}

	static TSharedPtr<const FCompositeFont> LoadFont(const FString& FileName)
	{
		static TMap<FString, TSharedPtr<const FCompositeFont>> Cache;
		if (const TSharedPtr<const FCompositeFont>* Found = Cache.Find(FileName))
		{
			return *Found;
		}
		const FString Path = UIRoot() / TEXT("Fonts") / FileName;
		TSharedPtr<const FCompositeFont> Result;
		if (IFileManager::Get().FileExists(*Path))
		{
			TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>(FName(*FileName), Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			// Playing card suits come from the symbol font whatever the main face is.
			const FString SymbolPath = UIRoot() / TEXT("Fonts/Symbols.ttf");
			if (IFileManager::Get().FileExists(*SymbolPath))
			{
				FCompositeSubFont& Sub = Font->SubTypefaces.AddDefaulted_GetRef();
				Sub.CharacterRanges.Add(FInt32Range(0x2660, 0x2668));
				Sub.Typeface.AppendFont(TEXT("Symbols"), SymbolPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}
			Result = Font;
		}
		Cache.Add(FileName, Result);
		return Result;
	}

	static FSlateFontInfo MakeFont(const TCHAR* File, const TCHAR* FallbackStyle, int32 Size)
	{
		if (TSharedPtr<const FCompositeFont> Font = LoadFont(File))
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle(FallbackStyle, Size);
	}

	FSlateFontInfo Title(int32 Size) { return MakeFont(TEXT("BebasNeue-Regular.ttf"), TEXT("Bold"), Size); }
	FSlateFontInfo Heading(int32 Size) { return MakeFont(TEXT("BarlowCondensed-Bold.ttf"), TEXT("Bold"), Size); }
	FSlateFontInfo SubHeading(int32 Size) { return MakeFont(TEXT("BarlowCondensed-SemiBold.ttf"), TEXT("Bold"), Size); }
	FSlateFontInfo Body(int32 Size) { return MakeFont(TEXT("Barlow-Regular.ttf"), TEXT("Regular"), Size); }
	FSlateFontInfo BodyMedium(int32 Size) { return MakeFont(TEXT("Barlow-Medium.ttf"), TEXT("Regular"), Size); }
	FSlateFontInfo Story(int32 Size) { return MakeFont(TEXT("Cinzel-Regular.ttf"), TEXT("Regular"), Size); }
	FSlateFontInfo StoryBold(int32 Size) { return MakeFont(TEXT("Cinzel-Bold.ttf"), TEXT("Bold"), Size); }

	FLinearColor Ink() { return FLinearColor(0.004f, 0.004f, 0.006f, 1.f); }
	FLinearColor Panel() { return FLinearColor(0.01f, 0.011f, 0.014f, 0.82f); }
	FLinearColor PanelLight() { return FLinearColor(0.035f, 0.035f, 0.042f, 0.9f); }
	FLinearColor Line() { return FLinearColor(1.f, 1.f, 1.f, 0.09f); }
	FLinearColor Blood() { return FLinearColor(0.85f, 0.06f, 0.08f, 1.f); }
	FLinearColor Gold() { return FLinearColor(1.f, 0.74f, 0.32f, 1.f); }
	FLinearColor Acid() { return FLinearColor(0.55f, 1.f, 0.3f, 1.f); }
	FLinearColor Ice() { return FLinearColor(0.7f, 0.85f, 1.f, 1.f); }
	FLinearColor Text() { return FLinearColor(0.93f, 0.91f, 0.87f, 1.f); }
	FLinearColor TextDim() { return FLinearColor(0.62f, 0.6f, 0.57f, 1.f); }
	FLinearColor TextFaint() { return FLinearColor(0.38f, 0.37f, 0.36f, 1.f); }

	const FSlateBrush* PanelBrush()
	{
		static FSlateRoundedBoxBrush Brush(Panel(), 10.f, Line(), 1.f);
		return &Brush;
	}

	const FSlateBrush* PanelSolidBrush()
	{
		static FSlateRoundedBoxBrush Brush(FLinearColor(0.012f, 0.012f, 0.016f, 0.97f), 12.f, Line(), 1.f);
		return &Brush;
	}

	const FSlateBrush* CardBrush()
	{
		static FSlateRoundedBoxBrush Brush(PanelLight(), 8.f, FLinearColor(1.f, 1.f, 1.f, 0.06f), 1.f);
		return &Brush;
	}

	const FSlateBrush* WhiteBrush()
	{
		static FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	const FSlateBrush* RoundBrush(float Radius)
	{
		static TMap<int32, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
		const int32 Key = FMath::RoundToInt(Radius * 10.f);
		if (TSharedPtr<FSlateRoundedBoxBrush>* Found = Cache.Find(Key))
		{
			return Found->Get();
		}
		TSharedPtr<FSlateRoundedBoxBrush> Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::White, Radius);
		Cache.Add(Key, Brush);
		return Brush.Get();
	}

	const FSlateBrush* OutlineBrush(const FLinearColor& Color, float Radius, float Width)
	{
		static TMap<FString, TSharedPtr<FSlateRoundedBoxBrush>> Cache;
		const FString Key = FString::Printf(TEXT("%s_%.1f_%.1f"), *Color.ToString(), Radius, Width);
		if (TSharedPtr<FSlateRoundedBoxBrush>* Found = Cache.Find(Key))
		{
			return Found->Get();
		}
		TSharedPtr<FSlateRoundedBoxBrush> Brush = MakeShared<FSlateRoundedBoxBrush>(FLinearColor::Transparent, Radius, Color, Width);
		Cache.Add(Key, Brush);
		return Brush.Get();
	}

	const FSlateBrush* Image(const FString& FileName, const FVector2D& Size)
	{
		// UI images are imported as textures: UI/Images/Foo.png -> /Game/AliceInBotherland/UI/Images/T_UI_Foo.
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		const FString Key = FileName + Size.ToString();
		if (TSharedPtr<FSlateBrush>* Found = Cache.Find(Key))
		{
			return Found->Get();
		}
		TSharedPtr<FSlateBrush> Brush;
		const FString AssetPath = TEXT("/Game/AliceInBotherland/UI/Images/T_UI_") + FPaths::GetBaseFilename(FileName);
		if (UTexture2D* Texture = Cast<UTexture2D>(AIBAssets::Load(AssetPath)))
		{
			Texture->AddToRoot();
			Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Texture);
			Brush->ImageSize = FVector2f(Size);
			Brush->DrawAs = ESlateBrushDrawType::Image;
		}
		Cache.Add(Key, Brush);
		return Brush.Get();
	}

	const FSlateBrush* SuitBrush(EAIBSymbol Symbol, bool bGlow)
	{
		const TCHAR* Names[] = { TEXT(""), TEXT("Heart"), TEXT("Diamond"), TEXT("Club"), TEXT("Spade") };
		if (Symbol == EAIBSymbol::None)
		{
			return nullptr;
		}
		return Image(FString::Printf(TEXT("Suit_%s%s.png"), Names[(int32)Symbol], bGlow ? TEXT("_Glow") : TEXT("")));
	}

	const FSlateBrush* CrestBrush(int32 Community)
	{
		return Image(FString::Printf(TEXT("Crest_%d.png"), FMath::Clamp(Community, 0, 4)));
	}

	FText Clock(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60));
	}

	void PlayUISound(FName Cue)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (UGameInstance* GI = GEngine->GameViewport->GetGameInstance())
			{
				if (UAIBAudioSubsystem* Audio = GI->GetSubsystem<UAIBAudioSubsystem>())
				{
					Audio->PlaySfx(Cue, 0.8f);
				}
			}
		}
	}
}
