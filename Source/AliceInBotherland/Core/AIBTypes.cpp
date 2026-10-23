// Alice in Botherland - shared gameplay types

#include "Core/AIBTypes.h"

#define LOCTEXT_NAMESPACE "AIB"

namespace AIB
{
	const FAIBCommunityInfo& Community(int32 Slot)
	{
		static const FAIBCommunityInfo Communities[MaxPlayers] =
		{
			{
				LOCTEXT("C0Name", "The Tidewardens"),
				LOCTEXT("C0Motto", "The sea took everything. We learned to take it back."),
				LOCTEXT("C0Desc", "Net-weavers and divers of the northern drift. Three thousand souls on a raft-city of lashed hulls."),
				LOCTEXT("C0Ship", "The Lantern Tide"),
				FLinearColor(0.05f, 0.75f, 0.78f)
			},
			{
				LOCTEXT("C1Name", "The Ember Clan"),
				LOCTEXT("C1Motto", "As long as the furnace burns, we live."),
				LOCTEXT("C1Desc", "Refinery crews who keep the old engines running on salvaged oil. Loud, proud and running out of fuel."),
				LOCTEXT("C1Ship", "The Cinderwake"),
				FLinearColor(1.0f, 0.38f, 0.06f)
			},
			{
				LOCTEXT("C2Name", "The Salt Choir"),
				LOCTEXT("C2Motto", "We remember, so the world is not forgotten."),
				LOCTEXT("C2Desc", "Keepers of the last library and the old songs. Their ships carry books sealed in wax."),
				LOCTEXT("C2Ship", "The Hymn of Salt"),
				FLinearColor(0.95f, 0.78f, 0.25f)
			},
			{
				LOCTEXT("C3Name", "The Iron Drift"),
				LOCTEXT("C3Motto", "Every wreck is a promise."),
				LOCTEXT("C3Desc", "Salvagers who build ships from the bones of drowned cities. Their hulls are patched with road signs."),
				LOCTEXT("C3Ship", "The Rust Leviathan"),
				FLinearColor(0.35f, 0.55f, 1.0f)
			},
			{
				LOCTEXT("C4Name", "The Greenhollow"),
				LOCTEXT("C4Motto", "Carry the seed. Find the soil."),
				LOCTEXT("C4Desc", "Seed-keepers tending the last living gardens in their holds. They have not touched earth in a generation."),
				LOCTEXT("C4Ship", "The Seedbearer"),
				FLinearColor(0.35f, 0.9f, 0.3f)
			}
		};
		return Communities[FMath::Clamp(Slot, 0, MaxPlayers - 1)];
	}

	FText SymbolName(EAIBSymbol Symbol)
	{
		switch (Symbol)
		{
		case EAIBSymbol::Heart: return LOCTEXT("Heart", "Heart");
		case EAIBSymbol::Diamond: return LOCTEXT("Diamond", "Diamond");
		case EAIBSymbol::Club: return LOCTEXT("Club", "Club");
		case EAIBSymbol::Spade: return LOCTEXT("Spade", "Spade");
		default: return LOCTEXT("NoSymbol", "?");
		}
	}

	FString SymbolGlyph(EAIBSymbol Symbol)
	{
		switch (Symbol)
		{
		case EAIBSymbol::Heart: return TEXT("♥");
		case EAIBSymbol::Diamond: return TEXT("♦");
		case EAIBSymbol::Club: return TEXT("♣");
		case EAIBSymbol::Spade: return TEXT("♠");
		default: return TEXT("?");
		}
	}

	FLinearColor SymbolColor(EAIBSymbol Symbol)
	{
		switch (Symbol)
		{
		case EAIBSymbol::Heart: return FLinearColor(1.0f, 0.12f, 0.18f);
		case EAIBSymbol::Diamond: return FLinearColor(1.0f, 0.62f, 0.1f);
		case EAIBSymbol::Club: return FLinearColor(0.2f, 0.95f, 0.45f);
		case EAIBSymbol::Spade: return FLinearColor(0.35f, 0.6f, 1.0f);
		default: return FLinearColor::White;
		}
	}

	EAIBSymbol ParseSymbol(const FString& Text)
	{
		const FString Lower = Text.ToLower();
		// Look for the last mentioned symbol so "not heart, it's spade" reads as spade.
		int32 Best = INDEX_NONE;
		EAIBSymbol Result = EAIBSymbol::None;
		auto Check = [&](const TCHAR* Word, EAIBSymbol Sym)
		{
			const int32 Index = Lower.Find(Word, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (Index != INDEX_NONE && Index > Best)
			{
				Best = Index;
				Result = Sym;
			}
		};
		Check(TEXT("heart"), EAIBSymbol::Heart);
		Check(TEXT("♥"), EAIBSymbol::Heart);
		Check(TEXT("diamond"), EAIBSymbol::Diamond);
		Check(TEXT("♦"), EAIBSymbol::Diamond);
		Check(TEXT("club"), EAIBSymbol::Club);
		Check(TEXT("♣"), EAIBSymbol::Club);
		Check(TEXT("spade"), EAIBSymbol::Spade);
		Check(TEXT("♠"), EAIBSymbol::Spade);
		return Result;
	}

	FText PhaseTitle(EAIBPhase Phase)
	{
		switch (Phase)
		{
		case EAIBPhase::Lobby: return LOCTEXT("PhLobby", "Lobby");
		case EAIBPhase::LobbyCountdown: return LOCTEXT("PhLobbyCount", "The trials begin in");
		case EAIBPhase::StoryIntro: return LOCTEXT("PhStory", "The story so far");
		case EAIBPhase::ArrivalCinematic: return LOCTEXT("PhArrival", "Arrival");
		case EAIBPhase::IslandExplore: return LOCTEXT("PhExplore", "The first trial begins in");
		case EAIBPhase::Stage1Briefing: return LOCTEXT("PhS1Brief", "Stage 1 - The Number Game");
		case EAIBPhase::Stage1RoundIntro: return LOCTEXT("PhS1Intro", "Round starting");
		case EAIBPhase::Stage1Selection: return LOCTEXT("PhS1Select", "Choose your number");
		case EAIBPhase::Stage1Reveal: return LOCTEXT("PhS1Reveal", "Calculating target");
		case EAIBPhase::Stage1Results: return LOCTEXT("PhS1Results", "Results");
		case EAIBPhase::Stage1Elimination: return LOCTEXT("PhS1Elim", "Elimination");
		case EAIBPhase::Stage2Briefing: return LOCTEXT("PhS2Brief", "Stage 2 - The Hidden Symbol");
		case EAIBPhase::Stage2Communication: return LOCTEXT("PhS2Comm", "Talk. Trust. Deceive.");
		case EAIBPhase::Stage2Guess: return LOCTEXT("PhS2Guess", "Name your symbol");
		case EAIBPhase::Stage2Results: return LOCTEXT("PhS2Results", "Results");
		case EAIBPhase::Stage2Elimination: return LOCTEXT("PhS2Elim", "Elimination");
		case EAIBPhase::Stage3Briefing: return LOCTEXT("PhS3Brief", "Stage 3 - Race to the Guardian");
		case EAIBPhase::Stage3Countdown: return LOCTEXT("PhS3Count", "Get ready");
		case EAIBPhase::Stage3Race: return LOCTEXT("PhS3Race", "Reach the Guardian");
		case EAIBPhase::Stage3Finish: return LOCTEXT("PhS3Finish", "The switch is claimed");
		case EAIBPhase::FinalBriefing: return LOCTEXT("PhFinalBrief", "The Guardian's Choice");
		case EAIBPhase::FinalChoice: return LOCTEXT("PhFinalChoice", "Make your choice");
		case EAIBPhase::FinalResolve: return LOCTEXT("PhFinalResolve", "The choice is made");
		case EAIBPhase::EndingCinematic: return LOCTEXT("PhEnding", "Ending");
		case EAIBPhase::Results: return LOCTEXT("PhResults", "Results");
		default: return FText::GetEmpty();
		}
	}

	EAIBSceneZone ZoneForPhase(EAIBPhase Phase)
	{
		switch (Phase)
		{
		case EAIBPhase::None:
		case EAIBPhase::Lobby:
		case EAIBPhase::LobbyCountdown:
			return EAIBSceneZone::Lobby;
		case EAIBPhase::StoryIntro:
		case EAIBPhase::ArrivalCinematic:
			return EAIBSceneZone::Sea;
		case EAIBPhase::IslandExplore:
			return EAIBSceneZone::Island;
		case EAIBPhase::Stage1Briefing:
		case EAIBPhase::Stage1RoundIntro:
		case EAIBPhase::Stage1Selection:
		case EAIBPhase::Stage1Reveal:
		case EAIBPhase::Stage1Results:
		case EAIBPhase::Stage1Elimination:
			return EAIBSceneZone::NumberRoom;
		case EAIBPhase::Stage2Briefing:
		case EAIBPhase::Stage2Communication:
		case EAIBPhase::Stage2Guess:
		case EAIBPhase::Stage2Results:
		case EAIBPhase::Stage2Elimination:
			return EAIBSceneZone::Hotel;
		case EAIBPhase::Stage3Briefing:
		case EAIBPhase::Stage3Countdown:
		case EAIBPhase::Stage3Race:
		case EAIBPhase::Stage3Finish:
			return EAIBSceneZone::Race;
		case EAIBPhase::FinalBriefing:
		case EAIBPhase::FinalChoice:
		case EAIBPhase::FinalResolve:
		case EAIBPhase::EndingCinematic:
			return EAIBSceneZone::Guardian;
		case EAIBPhase::Results:
		default:
			return EAIBSceneZone::Results;
		}
	}

	bool IsInputPhase(EAIBPhase Phase)
	{
		return Phase == EAIBPhase::Stage1Selection || Phase == EAIBPhase::Stage2Guess || Phase == EAIBPhase::FinalChoice;
	}

	FText EndingTitle(EAIBEnding Ending)
	{
		switch (Ending)
		{
		case EAIBEnding::KingFirst: return LOCTEXT("EndKing", "The Crowned Exile");
		case EAIBEnding::SacrificeThenKing: return LOCTEXT("EndSacKing", "The Gift and the Crown");
		case EAIBEnding::DoubleSacrifice: return LOCTEXT("EndDouble", "The Double Sacrifice");
		case EAIBEnding::SoloSacrifice: return LOCTEXT("EndSolo", "The Last Sacrifice");
		case EAIBEnding::SoloKing: return LOCTEXT("EndSoloKing", "The Last One Standing");
		default: return FText::GetEmpty();
		}
	}

	FString RandomBotName(FRandomStream& Stream, const TArray<FString>& Taken)
	{
		static const TCHAR* Names[] =
		{
			TEXT("Kestrel"), TEXT("Marrow"), TEXT("Juniper"), TEXT("Sable"), TEXT("Wren"), TEXT("Corin"),
			TEXT("Ilse"), TEXT("Tamsin"), TEXT("Rook"), TEXT("Nadia"), TEXT("Oskar"), TEXT("Halloran"),
			TEXT("Mireille"), TEXT("Brannock"), TEXT("Sorrel"), TEXT("Vesna"), TEXT("Quill"), TEXT("Dagny"),
			TEXT("Lior"), TEXT("Thessaly"), TEXT("Ash"), TEXT("Calder"), TEXT("Ondine"), TEXT("Fenwick")
		};
		const int32 Count = UE_ARRAY_COUNT(Names);
		for (int32 Attempt = 0; Attempt < 64; ++Attempt)
		{
			const FString Candidate = Names[Stream.RandRange(0, Count - 1)];
			if (!Taken.Contains(Candidate))
			{
				return Candidate;
			}
		}
		return FString::Printf(TEXT("Nomad %d"), Stream.RandRange(10, 99));
	}
}

#undef LOCTEXT_NAMESPACE
