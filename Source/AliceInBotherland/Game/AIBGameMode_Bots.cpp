// Alice in Botherland - server authority: AI champion decisions

#include "Game/AIBGameMode.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Game/AIBBotController.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBBots"

namespace
{
	bool IsLiveBot(const AAIBPlayerState* PS)
	{
		return PS && PS->bBot && !PS->bDisconnected && PS->IsActiveChampion();
	}

	int32 TrustKey(int32 Bot, int32 Other) { return Bot * 8 + Other; }
}

void AAIBGameMode::SetBotWander(const TArray<FVector>& Points, const FBox& Bounds)
{
	for (TActorIterator<AAIBBotController> It(GetWorld()); It; ++It)
	{
		const AAIBPlayerState* PS = It->GetPlayerState<AAIBPlayerState>();
		if (Points.Num() > 0 && IsLiveBot(PS))
		{
			It->SetWanderPoints(Points, Bounds);
		}
		else
		{
			It->ClearWander();
		}
	}
}

void AAIBGameMode::ScheduleBotActions(float WindowStart, float WindowEnd)
{
	BotActTime.Reset();
	BotChatBudget.Reset();
	for (AAIBPlayerState* PS : ActivePlayers())
	{
		if (IsLiveBot(PS))
		{
			BotActTime.Add(PS->Slot, Now() + Random.FRandRange(WindowStart, WindowEnd));
			BotChatBudget.Add(PS->Slot, Random.RandRange(2, 4));
		}
	}
}

void AAIBGameMode::TickBots(float DeltaSeconds)
{
	AAIBGameState* GS = GetAIBGameState();
	const float T = Now();
	for (AAIBPlayerState* PS : GS->GetSeatedPlayers())
	{
		if (!IsLiveBot(PS))
		{
			continue;
		}
		const float* ActAt = BotActTime.Find(PS->Slot);
		if (!ActAt || T < *ActAt)
		{
			continue;
		}
		AController* Bot = Cast<AController>(PS->GetOwner());
		switch (GS->Phase)
		{
		case EAIBPhase::Stage1Selection:
			if (!PS->bSubmitted && GS->Stage1Participants.Contains(PS->Slot))
			{
				SubmitNumber(Bot, BotPickNumber(PS));
			}
			BotActTime.Remove(PS->Slot);
			break;
		case EAIBPhase::Stage2Communication:
		{
			int32& Budget = BotChatBudget.FindOrAdd(PS->Slot);
			if (Budget > 0 && GS->Stage2Participants.Contains(PS->Slot))
			{
				BotChat(PS);
				--Budget;
				BotActTime.Add(PS->Slot, T + Random.FRandRange(3.5f, 9.f));
			}
			else
			{
				BotActTime.Remove(PS->Slot);
			}
			break;
		}
		case EAIBPhase::Stage2Guess:
			if (!PS->bSubmitted && GS->Stage2Participants.Contains(PS->Slot))
			{
				SubmitGuess(Bot, BotGuess(PS));
			}
			BotActTime.Remove(PS->Slot);
			break;
		default:
			BotActTime.Remove(PS->Slot);
			break;
		}
	}
}

int32 AAIBGameMode::BotPickNumber(const AAIBPlayerState* Bot) const
{
	const AAIBGameState* GS = GetAIBGameState();
	if (GS->bStage1SuddenDeath)
	{
		// Few players left: the target collapses towards zero, so go low (with some nerves).
		return FMath::Clamp(FMath::RoundToInt(FMath::Abs(Random.FRandRange(-6.f, 14.f))), 0, 100);
	}
	// Level-k reasoning: level 0 guesses ~50, each level multiplies by 0.8. Temper shifts how deep a bot thinks.
	float Weights[5] = { 0.12f, 0.28f, 0.3f, 0.2f, 0.1f };
	const float Shift = (Bot->BotTemper - 0.5f) * 0.3f;
	Weights[0] = FMath::Max(0.02f, Weights[0] + Shift);
	Weights[4] = FMath::Max(0.02f, Weights[4] - Shift);
	float Total = 0.f;
	for (float W : Weights)
	{
		Total += W;
	}
	float Pick = Random.FRandRange(0.f, Total);
	int32 Level = 0;
	for (; Level < 4; ++Level)
	{
		Pick -= Weights[Level];
		if (Pick <= 0.f)
		{
			break;
		}
	}
	float Guess = 50.f * FMath::Pow(0.8f, float(Level));
	if (Level == 0)
	{
		Guess = Random.FRandRange(20.f, 80.f);
	}
	Guess += Random.FRandRange(-6.f, 6.f);
	return FMath::Clamp(FMath::RoundToInt(Guess), 0, 100);
}

void AAIBGameMode::BotChat(AAIBPlayerState* Bot)
{
	AAIBGameState* GS = GetAIBGameState();
	TArray<AAIBPlayerState*> Others;
	for (int32 Slot : GS->Stage2Participants)
	{
		AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		if (PS && PS != Bot)
		{
			Others.Add(PS);
		}
	}
	if (Others.Num() == 0)
	{
		return;
	}
	AController* BotController = Cast<AController>(Bot->GetOwner());

	// One in five messages is just atmosphere.
	if (Random.FRand() < 0.18f)
	{
		static const TCHAR* Flavour[] =
		{
			TEXT("Someone in here is lying. I can feel it."),
			TEXT("Tell me mine and I'll tell you yours."),
			TEXT("Don't trust the quiet ones."),
			TEXT("My people are counting on me. Be honest."),
			TEXT("Everyone keep your voice down. The walls listen.")
		};
		SubmitChat(BotController, Flavour[Random.RandRange(0, UE_ARRAY_COUNT(Flavour) - 1)], INDEX_NONE, EAIBSymbol::None);
		return;
	}

	AAIBPlayerState* Target = Others[Random.RandRange(0, Others.Num() - 1)];
	// Honesty: calm bots mostly tell the truth; hot-headed ones lie more, especially to the points leader.
	float Honesty = FMath::Lerp(0.9f, 0.45f, Bot->BotTemper);
	if (Target->Stage2Points > Bot->Stage2Points)
	{
		Honesty -= 0.2f;
	}
	if (GetAIBGameState()->bStage2SuddenDeath)
	{
		// Head to head only one can advance: helping the rival is a bad bet, so most claims are bluffs.
		Honesty = FMath::Lerp(0.45f, 0.15f, Bot->BotTemper);
	}
	EAIBSymbol Claim = Target->HiddenSymbol;
	if (Random.FRand() > Honesty)
	{
		do
		{
			Claim = static_cast<EAIBSymbol>(Random.RandRange(1, 4));
		}
		while (Claim == Target->HiddenSymbol);
	}
	const FString Name = Target->GetPlayerName();
	const FString Sym = AIB::SymbolName(Claim).ToString();
	const FString Glyph = AIB::SymbolGlyph(Claim);
	FString Line;
	switch (Random.RandRange(0, 4))
	{
	case 0: Line = FString::Printf(TEXT("%s, you carry the %s %s."), *Name, *Sym, *Glyph); break;
	case 1: Line = FString::Printf(TEXT("%s - yours is a %s. Trust me."), *Name, *Sym); break;
	case 2: Line = FString::Printf(TEXT("I see a %s above you, %s."), *Sym, *Name); break;
	case 3: Line = FString::Printf(TEXT("%s: %s %s. Say %s."), *Name, *Glyph, *Sym, *Sym); break;
	default: Line = FString::Printf(TEXT("Listen %s. %s. Don't let anyone tell you otherwise."), *Name, *Sym); break;
	}
	SubmitChat(BotController, Line, Target->Slot, Claim);
}

EAIBSymbol AAIBGameMode::BotGuess(const AAIBPlayerState* Bot) const
{
	TMap<EAIBSymbol, float> Votes;
	TArray<TPair<int32, EAIBSymbol>> Claims;
	Stage2Claims.MultiFind(Bot->Slot, Claims);
	for (const TPair<int32, EAIBSymbol>& Claim : Claims)
	{
		const float* Trust = BotTrust.Find(TrustKey(Bot->Slot, Claim.Key));
		Votes.FindOrAdd(Claim.Value) += Trust ? *Trust : 0.6f;
	}
	EAIBSymbol Best = EAIBSymbol::None;
	float BestScore = 0.f;
	for (const TPair<EAIBSymbol, float>& Vote : Votes)
	{
		if (Vote.Value > BestScore)
		{
			BestScore = Vote.Value;
			Best = Vote.Key;
		}
	}
	if (Best == EAIBSymbol::None || BestScore < 0.25f)
	{
		Best = static_cast<EAIBSymbol>(Random.RandRange(1, 4));
	}
	return Best;
}

void AAIBGameMode::UpdateBotTrust()
{
	const AAIBGameState* GS = GetAIBGameState();
	for (const TPair<int32, TPair<int32, EAIBSymbol>>& Entry : Stage2Claims)
	{
		const AAIBPlayerState* Receiver = GS->GetPlayerBySlot(Entry.Key);
		if (!Receiver || !Receiver->bBot)
		{
			continue;
		}
		float& Trust = BotTrust.FindOrAdd(TrustKey(Receiver->Slot, Entry.Value.Key), 0.6f);
		Trust = Entry.Value.Value == Receiver->HiddenSymbol ? FMath::Min(1.f, Trust + 0.2f) : FMath::Max(0.f, Trust - 0.35f);
	}
}

#undef LOCTEXT_NAMESPACE
