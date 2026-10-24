// Alice in Botherland - a fallen champion's last words before the Guardian's judgement

#pragma once

#include "CoreMinimal.h"

/**
 * Lines a champion says when the Guardian judges them (voiced by the champion, LW_* in Audio/VO). Texts and spoken
 * lengths come from the generated AIBLastWordsLines.inl (Tools/gen_last_words.py).
 */
namespace AIBLastWords
{
	int32 Count();
	FName LineId(int32 Index);
	FText Text(int32 Index);
	/** Spoken length in seconds. */
	float Seconds(int32 Index);
}
