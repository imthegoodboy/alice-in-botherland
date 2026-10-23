// Alice in Botherland - the narrator (Alice): her lines and the narrated sequences built from them

#include "Audio/AIBNarration.h"

#define LOCTEXT_NAMESPACE "AIBNarration"

namespace AIBNarration
{
	namespace
	{
		const TMap<FName, FLine>& Lines()
		{
			static const TMap<FName, FLine> Table = []
			{
				TMap<FName, FLine> Result;
#define AIB_NARRATION_LINE(Id, Seconds, Caption) Result.Add(FName(TEXT(#Id)), FLine{ FName(TEXT(#Id)), Seconds, FText::FromString(TEXT(Caption)) });
#include "Audio/AIBNarrationLines.inl"
#undef AIB_NARRATION_LINE
				return Result;
			}();
			return Table;
		}

		FSequence Build(std::initializer_list<FBeat> Beats)
		{
			FSequence Sequence;
			Sequence.Beats = Beats;
			float Cursor = LeadIn;
			for (const FBeat& Beat : Sequence.Beats)
			{
				Sequence.Starts.Add(Cursor);
				const FLine* Line = FindLine(Beat.Line);
				Cursor += (Line ? Line->Duration : 3.f) + Gap;
			}
			Sequence.Length = Cursor - Gap + Tail;
			return Sequence;
		}

		const FSequence& Empty()
		{
			static const FSequence None;
			return None;
		}
	}

	const FLine* FindLine(FName Id)
	{
		return Lines().Find(Id);
	}

	bool IsNarratorLine(FName Id)
	{
		return Id.ToString().StartsWith(TEXT("NAR_"));
	}

	const FSequence& Get(EAIBNarration Which)
	{
		static const FText Fall = LOCTEXT("CardFall", "THE FALL OF EARTH");
		static const FText Search = LOCTEXT("CardSearch", "THE SEARCH FOR SURVIVAL");
		static const FText Island = LOCTEXT("CardIsland", "THE LAST ISLAND");
		static const FText Guardian = LOCTEXT("CardGuardian", "THE GUARDIAN'S TRIALS");
		static const FSequence Story = Build({
			{ TEXT("NAR_Story_01"), TEXT("S1_DyingEarth"), Fall },
			{ TEXT("NAR_Story_02"), TEXT("S1_AcidSea"), Fall },
			{ TEXT("NAR_Story_03"), TEXT("S1_Hunger"), Fall },
			{ TEXT("NAR_Story_04"), TEXT("S2_Fleets"), Search },
			{ TEXT("NAR_Story_05"), TEXT("S2_Search"), Search },
			{ TEXT("NAR_Story_06"), TEXT("S3_Island"), Island },
			{ TEXT("NAR_Story_07"), TEXT("S3_Island"), Island },
			{ TEXT("NAR_Story_08"), TEXT("S4_Guardian"), Guardian },
			{ TEXT("NAR_Story_09"), TEXT("S4_Guardian"), Guardian } });

		static const FText Welcome = LOCTEXT("CardWelcome", "WELCOME TO THE LAST ISLAND");
		static const FSequence IslandWelcome = Build({
			{ TEXT("NAR_Island_01"), TEXT("I_Welcome"), Welcome },
			{ TEXT("NAR_Island_02"), TEXT("I_Welcome"), Welcome },
			{ TEXT("NAR_Island_03"), TEXT("I_Welcome"), Welcome } });

		static const FText T1 = LOCTEXT("CardT1", "TRIAL I  -  THE NUMBER GAME");
		static const FSequence Stage1 = Build({
			{ TEXT("NAR_S1_Welcome"), TEXT("T1_Numbers"), T1 },
			{ TEXT("NAR_S1_Rule1"), TEXT("T1_Numbers"), T1 },
			{ TEXT("NAR_S1_Rule2"), TEXT("T1_Numbers"), T1 },
			{ TEXT("NAR_S1_Rule3"), TEXT("T1_Numbers"), T1 },
			{ TEXT("NAR_S1_Rule4"), TEXT("T1_Numbers"), T1 },
			{ TEXT("NAR_S1_Tip"), TEXT("T1_Numbers"), T1 } });

		static const FText T2 = LOCTEXT("CardT2", "TRIAL II  -  THE HIDDEN SYMBOL");
		static const FSequence Stage2 = Build({
			{ TEXT("NAR_S2_Welcome"), TEXT("T2_Symbols"), T2 },
			{ TEXT("NAR_S2_Rule1"), TEXT("T2_Symbols"), T2 },
			{ TEXT("NAR_S2_Rule2"), TEXT("T2_Symbols"), T2 },
			{ TEXT("NAR_S2_Rule3"), TEXT("T2_Symbols"), T2 },
			{ TEXT("NAR_S2_Rule4"), TEXT("T2_Symbols"), T2 } });

		static const FText T3 = LOCTEXT("CardT3", "TRIAL III  -  THE RACE TO THE GUARDIAN");
		static const FSequence Stage3 = Build({
			{ TEXT("NAR_S3_Welcome"), TEXT("T3_Race"), T3 },
			{ TEXT("NAR_S3_Rule1"), TEXT("T3_Race"), T3 },
			{ TEXT("NAR_S3_Rule2"), TEXT("T3_Race"), T3 },
			{ TEXT("NAR_S3_Rule3"), TEXT("T3_Race"), T3 } });

		static const FText T4 = LOCTEXT("CardT4", "THE GUARDIAN'S CHOICE");
		static const FSequence Final = Build({
			{ TEXT("NAR_Final_Welcome"), TEXT("T4_Choice"), T4 },
			{ TEXT("NAR_Final_Rule1"), TEXT("T4_Choice"), T4 },
			{ TEXT("NAR_Final_Rule2"), TEXT("T4_Choice"), T4 },
			{ TEXT("NAR_Final_Rule3"), TEXT("T4_Choice"), T4 },
			{ TEXT("NAR_Final_Rule4"), TEXT("T4_Choice"), T4 } });

		switch (Which)
		{
		case EAIBNarration::Story: return Story;
		case EAIBNarration::Island: return IslandWelcome;
		case EAIBNarration::Stage1: return Stage1;
		case EAIBNarration::Stage2: return Stage2;
		case EAIBNarration::Stage3: return Stage3;
		case EAIBNarration::Final: return Final;
		default: return Empty();
		}
	}

	float Length(EAIBNarration Which)
	{
		return Get(Which).Length;
	}

	int32 BeatAt(EAIBNarration Which, float Elapsed)
	{
		const FSequence& Sequence = Get(Which);
		int32 Result = INDEX_NONE;
		for (int32 Index = 0; Index < Sequence.Starts.Num(); ++Index)
		{
			if (Elapsed >= Sequence.Starts[Index])
			{
				Result = Index;
			}
		}
		return Result;
	}

	bool Contains(EAIBNarration Which, FName Line)
	{
		return Get(Which).Beats.ContainsByPredicate([Line](const FBeat& Beat) { return Beat.Line == Line; });
	}

	EAIBNarration ForPhase(EAIBPhase Phase)
	{
		switch (Phase)
		{
		case EAIBPhase::StoryIntro: return EAIBNarration::Story;
		case EAIBPhase::IslandExplore: return EAIBNarration::Island;
		case EAIBPhase::Stage1Briefing: return EAIBNarration::Stage1;
		case EAIBPhase::Stage2Briefing: return EAIBNarration::Stage2;
		case EAIBPhase::Stage3Briefing: return EAIBNarration::Stage3;
		case EAIBPhase::FinalBriefing: return EAIBNarration::Final;
		default: return EAIBNarration::None;
		}
	}

	FString ImagePath(FName Image)
	{
		return FString::Printf(TEXT("/Game/AliceInBotherland/UI/Story/T_%s"), *Image.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
