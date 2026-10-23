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
