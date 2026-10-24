// Alice in Botherland - server authority: Stage 1 (Number Game) and Stage 2 (Hidden Symbol)

#include "Game/AIBGameMode.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Game/AIBBotController.h"
#include "Player/AIBPlayerController.h"
#include "Player/AIBCharacter.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBNumberArena.h"
#include "World/AIBHotelArena.h"
#include "AliceInBotherland.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBTrials"

// ---------------------------------------------------------------------------------------------------------------------
// Stage 1 - The Number Game
// ---------------------------------------------------------------------------------------------------------------------

void AAIBGameMode::Stage1_Enter()
{
	AAIBGameState* GS = GetAIBGameState();
	GS->Stage1Round = 0;
	GS->bStage1SuddenDeath = false;
	GS->Stage1Participants.Reset();
	GS->Stage1AtRisk.Reset();
	Stage1TiedRounds = 0;
	GS->Stage1Last = FAIBStage1Round();
	SetBotWander({}, FBox());

	AAIBNumberArena* Arena = WorldSys() ? WorldSys()->GetNumberArena() : nullptr;
	for (AAIBPlayerState* PS : ActivePlayers())
	{
		if (Arena)
		{
			TeleportSlot(PS->Slot, Arena->GetStandTransform(PS->Slot));
		}
	}
	// Champions may walk around the chamber while Alice explains the rules; each round seats them again.
	SetAllMovementLocked(false);

	Cue(TEXT("SFX_Stinger_Title"));
	Cue(TEXT("MUS_Stage1"));
}

void AAIBGameMode::Stage1_BeginRound()
{
	AAIBGameState* GS = GetAIBGameState();
	const TArray<AAIBPlayerState*> Active = ActivePlayers();
	if (Active.Num() <= AIB::Stage1Advance)
	{
		// A forfeit already brought us down to four: nobody else needs to fall here.
		GS->bStage1SuddenDeath = false;
		GS->Stage1AtRisk.Reset();
		Announce(LOCTEXT("S1Skip", "THE NUMBER GAME ENDS"), LOCTEXT("S1SkipSub", "A champion already fell. Four remain."));
		FadeToPhase(EAIBPhase::Stage2Briefing);
		return;
	}

	GS->Stage1Round++;
	if (GS->bStage1SuddenDeath)
	{
		// Sudden death: everyone still picks (so the target keeps moving), but only the tied champions can fall.
		GS->Stage1AtRisk.RemoveAll([GS](int32 Slot)
		{
			const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
			return !PS || !PS->IsActiveChampion();
		});
		if (GS->Stage1AtRisk.Num() < 2)
		{
			GS->bStage1SuddenDeath = false;
			GS->Stage1AtRisk.Reset();
		}
	}
	GS->Stage1Participants.Reset();
	for (AAIBPlayerState* PS : Active)
	{
		GS->Stage1Participants.Add(PS->Slot);
		PS->bSubmitted = false;
		PS->PendingNumber = INDEX_NONE;
		PS->DraftNumber = INDEX_NONE;
	}
	GS->Stage1Last = FAIBStage1Round();
	GS->Stage1Last.Round = GS->Stage1Round;
	GS->Stage1Last.bSuddenDeath = GS->bStage1SuddenDeath;
	if (AAIBNumberArena* Arena = WorldSys() ? WorldSys()->GetNumberArena() : nullptr)
	{
		for (AAIBPlayerState* PS : Active)
		{
			TeleportSlot(PS->Slot, Arena->GetStandTransform(PS->Slot));
		}
	}
	SetAllMovementLocked(true);
	EnterPhase(EAIBPhase::Stage1RoundIntro);
}

void AAIBGameMode::SubmitNumber(AController* Controller, int32 Number)
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || GS->Phase != EAIBPhase::Stage1Selection || !GS->Stage1Participants.Contains(PS->Slot) || PS->bSubmitted || !PS->IsActiveChampion())
	{
		return;
	}
	PS->PendingNumber = FMath::Clamp(Number, 0, 100);
	PS->bSubmitted = true;
	PS->ForceNetUpdate();
}

void AAIBGameMode::SetDraftNumber(AController* Controller, int32 Number)
{
	AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (PS && !PS->bSubmitted && GetAIBGameState()->Phase == EAIBPhase::Stage1Selection)
	{
		PS->DraftNumber = FMath::Clamp(Number, 0, 100);
	}
}

void AAIBGameMode::Stage1_Resolve()
{
	AAIBGameState* GS = GetAIBGameState();
	FAIBStage1Round Round;
	Round.Round = GS->Stage1Round;
	Round.bSuddenDeath = GS->bStage1SuddenDeath;

	float Sum = 0.f;
	for (int32 Slot : GS->Stage1Participants)
	{
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		if (!PS || !PS->IsActiveChampion())
		{
			// Left mid-round: their number doesn't count.
			continue;
		}
		FAIBNumberEntry Entry;
		Entry.Slot = Slot;
		if (PS->PendingNumber != INDEX_NONE)
		{
			Entry.Number = PS->PendingNumber;
		}
		else if (PS->DraftNumber != INDEX_NONE)
		{
			// Dialled in but never sent: the dial counts.
			Entry.Number = PS->DraftNumber;
		}
		else
		{
			// Rule shown before the match: no number at all = random number.
			Entry.Number = Random.RandRange(0, 100);
			Entry.bAuto = true;
		}
		Sum += Entry.Number;
		Round.Entries.Add(Entry);
	}
	if (Round.Entries.Num() == 0)
	{
		FadeToPhase(EAIBPhase::Stage2Briefing);
		return;
	}
	Round.Average = Sum / Round.Entries.Num();
	Round.Target = Round.Average * 0.8f;

	// In sudden death only the tied champions can be the farthest.
	auto CanFall = [GS](int32 Slot) { return !GS->bStage1SuddenDeath || GS->Stage1AtRisk.Contains(Slot); };
	float MinDist = TNumericLimits<float>::Max();
	float MaxDist = -1.f;
	for (FAIBNumberEntry& Entry : Round.Entries)
	{
		Entry.Distance = FMath::Abs(Entry.Number - Round.Target);
		MinDist = FMath::Min(MinDist, Entry.Distance);
		if (CanFall(Entry.Slot))
		{
			MaxDist = FMath::Max(MaxDist, Entry.Distance);
		}
	}
	TArray<int32> Farthest;
	for (FAIBNumberEntry& Entry : Round.Entries)
	{
		Entry.bWinner = FMath::IsNearlyEqual(Entry.Distance, MinDist, 0.001f);
		Entry.bFarthest = CanFall(Entry.Slot) && FMath::IsNearlyEqual(Entry.Distance, MaxDist, 0.001f);
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Entry.Slot);
		if (Entry.bWinner && PS)
		{
			PS->Stage1Wins++;
			LogEvent(EAIBMatchEventKind::RoundWin, Entry.Slot, Round.Round);
		}
		else if (PS)
		{
			PS->Lives = FMath::Max(0, PS->Lives - 1);
		}
		if (Entry.bFarthest)
		{
			Farthest.Add(Entry.Slot);
		}
	}
	if (Farthest.Num() == 1)
	{
		Round.EliminatedSlot = Farthest[0];
	}
	else
	{
		Round.TiedSlots = Farthest;
	}
	GS->Stage1Last = Round;
	GS->ForceNetUpdate();
	Cue(TEXT("SFX_NumberRoll"));
	EnterPhase(EAIBPhase::Stage1Reveal);
}

void AAIBGameMode::Stage1_AfterResults()
{
	AAIBGameState* GS = GetAIBGameState();
	const FAIBStage1Round& Round = GS->Stage1Last;
	if (GS->CountActive() <= AIB::Stage1Advance)
	{
		// Someone left during the round: that was this trial's fall.
		Stage1_BeginRound();
		return;
	}
	// Sudden death that keeps tying (everyone picking the same number) is settled by lot after a few rounds.
	const bool bLot = Round.EliminatedSlot == INDEX_NONE && GS->bStage1SuddenDeath && ++Stage1TiedRounds >= 3 && Round.TiedSlots.Num() > 0;
	const int32 LoserSlot = bLot ? Round.TiedSlots[Random.RandRange(0, Round.TiedSlots.Num() - 1)] : Round.EliminatedSlot;
	if (LoserSlot != INDEX_NONE)
	{
		AAIBPlayerState* Loser = GS->GetPlayerBySlot(LoserSlot);
		Eliminate(Loser, 1, false, true);
		GS->bStage1SuddenDeath = false;
		GS->Stage1AtRisk.Reset();
		Stage1TiedRounds = 0;
		if (bLot)
		{
			SystemChat(LOCTEXT("S1ElimLot", "Tied again - the Guardian cast lots."));
		}
		EnterPhase(EAIBPhase::Stage1Elimination, BeginJudgement(1, true));
		return;
	}

	// Tie for farthest: those players are at risk in a sudden-death round that everyone plays.
	GS->bStage1SuddenDeath = true;
	GS->Stage1AtRisk = Round.TiedSlots;
	LogEvent(EAIBMatchEventKind::SuddenDeath, INDEX_NONE, 1);
	FString Names;
	for (int32 Slot : Round.TiedSlots)
	{
		if (const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot))
		{
			Names += (Names.IsEmpty() ? TEXT("") : TEXT("  vs  ")) + PS->GetPlayerName();
		}
	}
	Announce(LOCTEXT("S1Sudden", "SUDDEN DEATH"), FText::Format(LOCTEXT("S1SuddenSub", "{0} at risk - everyone picks again"), FText::FromString(Names)), FLinearColor(1.f, 0.4f, 0.1f));
	Cue(TEXT("VO_Stage1_SuddenDeath"));
	Stage1_BeginRound();
}

// ---------------------------------------------------------------------------------------------------------------------
// Stage 2 - The Hidden Symbol
// ---------------------------------------------------------------------------------------------------------------------

void AAIBGameMode::Stage2_Enter()
{
	AAIBGameState* GS = GetAIBGameState();
	GS->Stage2Round = 0;
	GS->bStage2SuddenDeath = false;
	GS->Stage2Participants.Reset();
	GS->Stage2Last.Reset();
	Stage2Qualified.Reset();
	Stage2Contenders.Reset();
	Stage2SpotsLeft = AIB::Stage2Advance;

	const TArray<AAIBPlayerState*> Active = ActivePlayers();
	AAIBHotelArena* Hotel = WorldSys() ? WorldSys()->GetHotel() : nullptr;
	for (int32 Index = 0; Index < Active.Num(); ++Index)
	{
		AAIBPlayerState* PS = Active[Index];
		PS->Stage2Points = 0;
		Stage2Contenders.Add(PS->Slot);
		if (Hotel)
		{
			TeleportSlot(PS->Slot, Hotel->GetSpawn(Index, Active.Num()));
		}
	}
	SetAllMovementLocked(false);
	if (Hotel)
	{
		SetBotWander(Hotel->GetWanderPoints(), Hotel->GetWalkBounds());
	}

	Cue(TEXT("SFX_Stinger_Title"));
	Cue(TEXT("MUS_Stage2"));
}

void AAIBGameMode::Stage2_BeginRound()
{
	AAIBGameState* GS = GetAIBGameState();
	if (!GS->bStage2SuddenDeath && GS->CountActive() <= AIB::Stage2Advance)
	{
		Announce(LOCTEXT("S2Skip", "THE HIDDEN SYMBOL ENDS"), LOCTEXT("S2SkipSub", "Only two champions remain."));
		FadeToPhase(EAIBPhase::Stage3Briefing);
		return;
	}
	GS->Stage2Round++;
	GS->Stage2Participants.Reset();
	for (int32 Slot : Stage2Contenders)
	{
		const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		if (PS && PS->IsActiveChampion())
		{
			GS->Stage2Participants.Add(Slot);
		}
	}
	GS->Stage2Last.Reset();
	GS->Stage2Ledger.Reset();
	Stage2Claims.Reset();
	for (int32 Slot : GS->Stage2Participants)
	{
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		// Independent draw (repeats allowed) so nobody can deduce their own symbol from the others.
		PS->HiddenSymbol = static_cast<EAIBSymbol>(Random.RandRange(1, 4));
		PS->PendingGuess = EAIBSymbol::None;
		PS->bSubmitted = false;
	}
	Stage2_SendSymbolViews();

	if (GS->bStage2SuddenDeath)
	{
		Announce(LOCTEXT("S2Sudden", "TIE FOR THE FINAL - SUDDEN DEATH ROUND"), FText::Format(LOCTEXT("S2SuddenSub", "{0}. 30 seconds to talk."), FText::FromString(Stage2TieNames)), FLinearColor(1.f, 0.6f, 0.2f));
	}
	else
	{
		Announce(FText::Format(LOCTEXT("S2Round", "SYMBOL ROUND {0} OF {1}"), FText::AsNumber(GS->Stage2Round), FText::AsNumber(AIB::Stage2Rounds)),
			LOCTEXT("S2RoundSub", "30 seconds to talk. Use the chat or the quick-tell buttons."), FLinearColor(0.6f, 0.75f, 1.f));
	}
	EnterPhase(EAIBPhase::Stage2Communication);
}

void AAIBGameMode::Stage2_SendSymbolViews(AAIBPlayerController* OnlyFor)
{
	AAIBGameState* GS = GetAIBGameState();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AAIBPlayerController* PC = Cast<AAIBPlayerController>(It->Get());
		if (!PC || (OnlyFor && PC != OnlyFor))
		{
			continue;
		}
		const AAIBPlayerState* Viewer = PC->GetPlayerState<AAIBPlayerState>();
		const int32 ViewerSlot = Viewer ? Viewer->Slot : INDEX_NONE;
		const bool bViewerCompeting = GS->Stage2Participants.Contains(ViewerSlot);
		TArray<FAIBSymbolView> Views;
		for (int32 Slot : GS->Stage2Participants)
		{
			// The server never sends a champion their own symbol.
			if (bViewerCompeting && Slot == ViewerSlot)
			{
				continue;
			}
			if (const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot))
			{
				FAIBSymbolView View;
				View.Slot = Slot;
				View.Symbol = PS->HiddenSymbol;
				Views.Add(View);
			}
		}
		PC->ClientReceiveSymbols(Views);
	}
}

void AAIBGameMode::SubmitGuess(AController* Controller, EAIBSymbol Guess)
{
	AAIBGameState* GS = GetAIBGameState();
	AAIBPlayerState* PS = Controller ? Controller->GetPlayerState<AAIBPlayerState>() : nullptr;
	if (!PS || GS->Phase != EAIBPhase::Stage2Guess || !GS->Stage2Participants.Contains(PS->Slot) || PS->bSubmitted || Guess == EAIBSymbol::None)
	{
		return;
	}
	PS->PendingGuess = Guess;
	PS->bSubmitted = true;
	PS->ForceNetUpdate();
}

void AAIBGameMode::Stage2_Resolve()
{
	AAIBGameState* GS = GetAIBGameState();
	GS->Stage2Last.Reset();
	for (int32 Slot : GS->Stage2Participants)
	{
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		if (!PS)
		{
			continue;
		}
		FAIBSymbolResult Result;
		Result.Slot = Slot;
		Result.Actual = PS->HiddenSymbol;
		Result.Guess = PS->PendingGuess;
		Result.bCorrect = PS->PendingGuess != EAIBSymbol::None && PS->PendingGuess == PS->HiddenSymbol;
		if (Result.bCorrect)
		{
			PS->Stage2Points++;
		}
		GS->Stage2Last.Add(Result);
	}
	// The Liar's Ledger: every claim (the last one each champion made to each other), now that it can't help anyone.
	TMap<int32, int32> LastClaim;
	TArray<FAIBClaimRecord> Ledger;
	for (const TPair<int32, TPair<int32, EAIBSymbol>>& Entry : Stage2Claims)
	{
		const AAIBPlayerState* Receiver = GS->GetPlayerBySlot(Entry.Key);
		AAIBPlayerState* Claimer = GS->GetPlayerBySlot(Entry.Value.Key);
		if (!Receiver || !Claimer || !GS->Stage2Participants.Contains(Entry.Key))
		{
			continue;
		}
		FAIBClaimRecord Record;
		Record.FromSlot = Entry.Value.Key;
		Record.ToSlot = Entry.Key;
		Record.Claimed = Entry.Value.Value;
		Record.bTrue = Record.Claimed == Receiver->HiddenSymbol;
		Record.bBelieved = Receiver->PendingGuess == Record.Claimed;
		const int32 Key = Record.FromSlot * 8 + Record.ToSlot;
		if (const int32* Existing = LastClaim.Find(Key))
		{
			Ledger[*Existing] = Record;
		}
		else
		{
			LastClaim.Add(Key, Ledger.Add(Record));
		}
	}
	GS->Stage2Ledger = Ledger;
	UpdateBotTrust();
	GS->ForceNetUpdate();
	Cue(TEXT("SFX_Whoosh_Reveal"));
	EnterPhase(EAIBPhase::Stage2Results);
}

void AAIBGameMode::Stage2_AfterResults()
{
	AAIBGameState* GS = GetAIBGameState();
	if (!GS->bStage2SuddenDeath && GS->Stage2Round < AIB::Stage2Rounds)
	{
		Stage2_BeginRound();
		return;
	}

	// Rank the contenders for the remaining spots.
	TArray<AAIBPlayerState*> Ranked;
	for (int32 Slot : Stage2Contenders)
	{
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		if (PS && PS->IsActiveChampion())
		{
			Ranked.Add(PS);
		}
	}
	Ranked.Sort([](const AAIBPlayerState& A, const AAIBPlayerState& B) { return A.Stage2Points > B.Stage2Points; });

	bool bResolved = true;
	if (Ranked.Num() > Stage2SpotsLeft && Stage2SpotsLeft > 0)
	{
		const int32 Cutoff = Ranked[Stage2SpotsLeft - 1]->Stage2Points;
		TArray<AAIBPlayerState*> Above, Tied, Below;
		for (AAIBPlayerState* PS : Ranked)
		{
			(PS->Stage2Points > Cutoff ? Above : PS->Stage2Points == Cutoff ? Tied : Below).Add(PS);
		}
		const int32 TiedSpots = Stage2SpotsLeft - Above.Num();
		for (AAIBPlayerState* PS : Above)
		{
			Stage2Qualified.AddUnique(PS->Slot);
		}
		for (AAIBPlayerState* PS : Below)
		{
			Eliminate(PS, 2, false, true);
		}
		const bool bTooManySuddenDeaths = GS->Stage2Round >= AIB::Stage2Rounds + 4;
		if (Tied.Num() > TiedSpots && !bTooManySuddenDeaths)
		{
			Stage2SpotsLeft = TiedSpots;
			Stage2Contenders.Reset();
			FString Names;
			for (AAIBPlayerState* PS : Tied)
			{
				Stage2Contenders.Add(PS->Slot);
				Names += (Names.IsEmpty() ? TEXT("") : TEXT("  vs  ")) + PS->GetPlayerName();
			}
			GS->bStage2SuddenDeath = true;
			bResolved = false;
			Stage2TieNames = Names;
			LogEvent(EAIBMatchEventKind::SuddenDeath, INDEX_NONE, 2);
			SystemChat(FText::Format(LOCTEXT("S2TieChat", "Tie for the final: {0}. One more symbol round between them."), FText::FromString(Names)));
			if (PendingFallen.Num() > 0)
			{
				// Those below the tie are judged first; the sudden-death round follows the scene.
				EnterPhase(EAIBPhase::Stage2Elimination, BeginJudgement(2, false));
			}
			else
			{
				Stage2_BeginRound();
			}
			return;
		}
		// Still tied after many rounds: the Guardian decides by lot.
		while (Tied.Num() > TiedSpots)
		{
			Eliminate(Tied[Random.RandRange(0, Tied.Num() - 1)], 2, false, true);
			Tied.RemoveAll([](const AAIBPlayerState* PS) { return !PS->IsActiveChampion(); });
		}
		for (AAIBPlayerState* PS : Tied)
		{
			Stage2Qualified.AddUnique(PS->Slot);
		}
	}
	else
	{
		for (AAIBPlayerState* PS : Ranked)
		{
			Stage2Qualified.AddUnique(PS->Slot);
		}
	}

	if (bResolved)
	{
		// Everyone not qualified is out.
		for (AAIBPlayerState* PS : ActivePlayers())
		{
			if (!Stage2Qualified.Contains(PS->Slot))
			{
				Eliminate(PS, 2, false, true);
			}
		}
		GS->bStage2SuddenDeath = false;
		// Clear private symbol views.
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AAIBPlayerController* PC = Cast<AAIBPlayerController>(It->Get()))
			{
				PC->ClientReceiveSymbols({});
			}
		}
		EnterPhase(EAIBPhase::Stage2Elimination, BeginJudgement(2, true));
	}
}

#undef LOCTEXT_NAMESPACE
