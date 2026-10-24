// Alice in Botherland - a fallen champion's last words before the Guardian's judgement

#include "Audio/AIBLastWords.h"

namespace AIBLastWords
{
	namespace
	{
		struct FWords
		{
			FName Id;
			float Seconds;
			FText Text;
		};

		const TArray<FWords>& All()
		{
			static const TArray<FWords> Table = []
			{
				TArray<FWords> Result;
#define AIB_LAST_WORDS(Id, Seconds, Caption) Result.Add(FWords{ FName(TEXT(#Id)), Seconds, FText::FromString(TEXT(Caption)) });
#include "Audio/AIBLastWordsLines.inl"
#undef AIB_LAST_WORDS
				return Result;
			}();
			return Table;
		}
	}

	int32 Count()
	{
		return All().Num();
	}

	FName LineId(int32 Index)
	{
		return All().IsValidIndex(Index) ? All()[Index].Id : NAME_None;
	}

	FText Text(int32 Index)
	{
		return All().IsValidIndex(Index) ? All()[Index].Text : FText::GetEmpty();
	}

	float Seconds(int32 Index)
	{
		return All().IsValidIndex(Index) ? All()[Index].Seconds : 4.f;
	}
}
