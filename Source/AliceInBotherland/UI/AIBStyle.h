// Alice in Botherland - UI look: fonts, colours, brushes (pure Slate, files loaded at runtime)

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Core/AIBTypes.h"

namespace AIBStyle
{
	// Fonts (TTF files under Content/AliceInBotherland/UI/Fonts, engine Roboto as fallback).
	ALICEINBOTHERLAND_API FSlateFontInfo Title(int32 Size);      // Bebas Neue - big display numbers and titles
	ALICEINBOTHERLAND_API FSlateFontInfo Heading(int32 Size);    // Barlow Condensed Bold
	ALICEINBOTHERLAND_API FSlateFontInfo SubHeading(int32 Size); // Barlow Condensed SemiBold
	ALICEINBOTHERLAND_API FSlateFontInfo Body(int32 Size);       // Barlow Regular
	ALICEINBOTHERLAND_API FSlateFontInfo BodyMedium(int32 Size); // Barlow Medium
	ALICEINBOTHERLAND_API FSlateFontInfo Story(int32 Size);      // Cinzel Regular - narration
	ALICEINBOTHERLAND_API FSlateFontInfo StoryBold(int32 Size);  // Cinzel Bold
	ALICEINBOTHERLAND_API FSlateFontInfo IconFont(int32 Size);   // Material Icons (glyphs from AIBIcon)
	/** Same font with extra tracking (1/1000 em), for game-style labels. */
	ALICEINBOTHERLAND_API FSlateFontInfo Spaced(FSlateFontInfo Font, int32 LetterSpacing);

	// Palette
	ALICEINBOTHERLAND_API FLinearColor Ink();        // near-black background
	ALICEINBOTHERLAND_API FLinearColor Panel();      // translucent panel
	ALICEINBOTHERLAND_API FLinearColor PanelLight();
	ALICEINBOTHERLAND_API FLinearColor Line();       // thin borders
	ALICEINBOTHERLAND_API FLinearColor Blood();      // primary accent
	ALICEINBOTHERLAND_API FLinearColor Gold();
	ALICEINBOTHERLAND_API FLinearColor Acid();
	ALICEINBOTHERLAND_API FLinearColor Ice();
	ALICEINBOTHERLAND_API FLinearColor Text();
	ALICEINBOTHERLAND_API FLinearColor TextDim();
	ALICEINBOTHERLAND_API FLinearColor TextFaint();
	ALICEINBOTHERLAND_API FLinearColor Glass();      // dark translucent fill for game panels and buttons
	ALICEINBOTHERLAND_API FLinearColor Ember();      // warm highlight between Blood and Gold

	// Brushes (owned by the style, valid for the program lifetime)
	ALICEINBOTHERLAND_API const FSlateBrush* PanelBrush();
	ALICEINBOTHERLAND_API const FSlateBrush* PanelSolidBrush();
	ALICEINBOTHERLAND_API const FSlateBrush* CardBrush();
	ALICEINBOTHERLAND_API const FSlateBrush* WhiteBrush();
	ALICEINBOTHERLAND_API const FSlateBrush* RoundBrush(float Radius);
	ALICEINBOTHERLAND_API const FSlateBrush* OutlineBrush(const FLinearColor& Color, float Radius, float Width);
	/** PNG under UI/Images by file name (e.g. "Suit_Heart.png"); nullptr if missing. */
	ALICEINBOTHERLAND_API const FSlateBrush* Image(const FString& FileName, const FVector2D& Size = FVector2D(256.f, 256.f));
	ALICEINBOTHERLAND_API const FSlateBrush* SuitBrush(EAIBSymbol Symbol, bool bGlow = false);
	ALICEINBOTHERLAND_API const FSlateBrush* CrestBrush(int32 Community);
	/** Story / trial illustration by id (e.g. "S3_Island"), 16:9; nullptr if missing. */
	ALICEINBOTHERLAND_API const FSlateBrush* StoryImage(FName Id);

	ALICEINBOTHERLAND_API FString UIRoot();

	/** "1:05" style timer text. */
	ALICEINBOTHERLAND_API FText Clock(float Seconds);

	/** Plays a UI sound through the audio subsystem of the running game. */
	ALICEINBOTHERLAND_API void PlayUISound(FName Cue);
}

/** Material Icons code points (font UI/Fonts/MaterialIcons-Regular.ttf, Apache 2.0). */
namespace AIBIcon
{
	constexpr TCHAR Settings = 0xe8b8;
	constexpr TCHAR Play = 0xe037;
	constexpr TCHAR Trophy = 0xea23;
	constexpr TCHAR Book = 0xea19;
	constexpr TCHAR Info = 0xe88e;
	constexpr TCHAR Power = 0xe8ac;
	constexpr TCHAR Person = 0xe7fd;
	constexpr TCHAR Groups = 0xf233;
	constexpr TCHAR Robot = 0xf06c;
	constexpr TCHAR Login = 0xea77;
	constexpr TCHAR Add = 0xe145;
	constexpr TCHAR Copy = 0xe14d;
	constexpr TCHAR Mic = 0xe029;
	constexpr TCHAR MicOff = 0xe02b;
	constexpr TCHAR Logout = 0xe9ba;
	constexpr TCHAR Globe = 0xe80b;
	constexpr TCHAR Lan = 0xeb2f;
	constexpr TCHAR Check = 0xe86c;
	constexpr TCHAR Close = 0xe5cd;
	constexpr TCHAR Up = 0xe316;
	constexpr TCHAR Down = 0xe313;
	constexpr TCHAR Back = 0xe5c4;
	constexpr TCHAR Volume = 0xe050;
	constexpr TCHAR Tune = 0xe429;
	constexpr TCHAR Voice = 0xe91f;
	constexpr TCHAR Gamepad = 0xea28;
	constexpr TCHAR Movie = 0xe02c;
	constexpr TCHAR Refresh = 0xe5d5;
	constexpr TCHAR Restart = 0xf053;
	constexpr TCHAR Edit = 0xe3c9;
	constexpr TCHAR Chevron = 0xe5cc;
	constexpr TCHAR Hourglass = 0xe88b;
	constexpr TCHAR Bolt = 0xea0b;
	constexpr TCHAR Explore = 0xe87a;
	constexpr TCHAR Shield = 0xe9e0;
	constexpr TCHAR Keyboard = 0xe312;

	inline FText Glyph(TCHAR Code) { return FText::FromString(FString::Chr(Code)); }
}
