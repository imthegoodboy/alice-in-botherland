// Alice in Botherland - online lobbies: host, browse, join, invite (Steam or LAN)

#include "Online/AIBSessionSubsystem.h"
#include "Core/AIBGameInstance.h"
#include "AliceInBotherland.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBSessions"

namespace
{
	const FName GameKey(TEXT("AIB_GAME"));
	const FString GameValue(TEXT("ALICE_IN_BOTHERLAND_V1"));
}

void UAIBSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (IOnlineSessionPtr Sessions = GetSessions())
	{
		CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAIBSessionSubsystem::OnCreateComplete));
		FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAIBSessionSubsystem::OnFindComplete));
		JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAIBSessionSubsystem::OnJoinComplete));
		DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAIBSessionSubsystem::OnDestroyComplete));
		InviteHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UAIBSessionSubsystem::OnInviteAccepted));
	}
	UE_LOG(LogAIB, Log, TEXT("Online platform: %s"), *GetPlatformName());
}

void UAIBSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr UAIBSessionSubsystem::GetSessions() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

FString UAIBSessionSubsystem::GetPlatformName() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetSubsystemName().ToString() : TEXT("NONE");
}

bool UAIBSessionSubsystem::HasOnlinePlatform() const
{
	const FString Name = GetPlatformName();
	return !Name.Equals(TEXT("NULL"), ESearchCase::IgnoreCase) && !Name.Equals(TEXT("NONE"), ESearchCase::IgnoreCase);
}

FString UAIBSessionSubsystem::GetLocalAddress() const
{
	bool bCanBind = false;
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBind);
	return Addr->IsValid() ? Addr->ToString(false) : TEXT("?");
}

FString UAIBSessionSubsystem::TravelOptions() const
{
	// The engine replaces "Name" with the online subsystem's nickname (a machine id on LAN), so the champion name
	// also travels as "Champion", which the game mode applies at login.
	UAIBGameInstance* GI = Cast<UAIBGameInstance>(GetGameInstance());
	const FString Name = GI ? GI->GetPlayerName() : TEXT("Nomad");
	return FString::Printf(TEXT("?Name=%s?Champion=%s"), *Name, *Name);
}

void UAIBSessionSubsystem::TravelToHost(const FString& Options)
{
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(UAIBGameInstance::IslandMap), true,
		FString::Printf(TEXT("listen?game=Match?Bots=%d%s%s%s"), PendingBots, *TravelOptions(), bPendingAutoReady ? TEXT("?AutoReady") : TEXT(""), *Options));
}

void UAIBSessionSubsystem::HostLobby(bool bLAN, int32 Bots, bool bAutoReady)
{
	PendingBots = FMath::Clamp(Bots, 0, 4);
	bPendingLAN = bLAN;
	bPendingAutoReady = bAutoReady;
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		Status = LOCTEXT("NoOSS", "Online services unavailable - hosting a direct-IP lobby.");
		TravelToHost(TEXT(""));
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bHostAfterDestroy = true;
		Sessions->DestroySession(NAME_GameSession);
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 5;
	Settings.NumPrivateConnections = 0;
	Settings.bIsLANMatch = bLAN || !HasOnlinePlatform();
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowInvites = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bUsesStats = false;
	Settings.Set(GameKey, GameValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SETTING_MAPNAME, FString(TEXT("L_Island")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	bBusy = true;
	Status = LOCTEXT("Creating", "Creating lobby...");
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		bBusy = false;
		Status = LOCTEXT("CreateFailed", "Could not create an online lobby - hosting a direct-IP lobby.");
		TravelToHost(TEXT(""));
	}
}

void UAIBSessionSubsystem::OnCreateComplete(FName SessionName, bool bSuccess)
{
	bBusy = false;
	UE_LOG(LogAIB, Log, TEXT("Create session %s: %d"), *SessionName.ToString(), bSuccess);
	Status = bSuccess ? LOCTEXT("Created", "Lobby created.") : LOCTEXT("CreateFailed2", "Online lobby failed - hosting a direct-IP lobby.");
	TravelToHost(TEXT(""));
}

void UAIBSessionSubsystem::FindLobbies(bool bLAN)
{
	IOnlineSessionPtr Sessions = GetSessions();
	Results.Reset();
	if (!Sessions.IsValid())
	{
		Status = LOCTEXT("NoOSSFind", "Online services unavailable. Use Join by IP.");
		return;
	}
	Search = MakeShared<FOnlineSessionSearch>();
	Search->bIsLanQuery = bLAN || !HasOnlinePlatform();
	Search->MaxSearchResults = 100;
	Search->PingBucketSize = 50;
	Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	bSearching = true;
	Status = LOCTEXT("Searching", "Searching for lobbies...");
	if (!Sessions->FindSessions(0, Search.ToSharedRef()))
	{
		bSearching = false;
		Status = LOCTEXT("SearchFailed", "Search failed.");
	}
}

void UAIBSessionSubsystem::OnFindComplete(bool bSuccess)
{
	bSearching = false;
	Results.Reset();
	if (!Search.IsValid())
	{
		return;
	}
	for (int32 Index = 0; Index < Search->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = Search->SearchResults[Index];
		FString Value;
		// Only our game (Steam's shared dev app id lists every test game in the world).
		if (!Result.Session.SessionSettings.Get(GameKey, Value) || Value != GameValue)
		{
			continue;
		}
		FAIBLobbyInfo Info;
		Info.HostName = Result.Session.OwningUserName;
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.Players = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
		Info.Ping = Result.PingInMs;
		Info.SearchIndex = Index;
		Results.Add(Info);
	}
	Status = Results.Num() > 0
		? FText::Format(LOCTEXT("Found", "{0} lobbies found."), FText::AsNumber(Results.Num()))
		: LOCTEXT("NoneFound", "No lobbies found. Ask a friend to host, or join by IP.");
}

void UAIBSessionSubsystem::JoinLobby(int32 ResultIndex)
{
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid() || !Search.IsValid() || !Search->SearchResults.IsValidIndex(ResultIndex))
	{
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		PendingJoin = Search->SearchResults[ResultIndex];
		Sessions->DestroySession(NAME_GameSession);
		return;
	}
	bBusy = true;
	Status = LOCTEXT("Joining", "Joining lobby...");
	Sessions->JoinSession(0, NAME_GameSession, Search->SearchResults[ResultIndex]);
}

void UAIBSessionSubsystem::OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bBusy = false;
	IOnlineSessionPtr Sessions = GetSessions();
	FString Connect;
	if (Result != EOnJoinSessionCompleteResult::Success || !Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, Connect))
	{
		Status = LOCTEXT("JoinFailed", "Could not join that lobby (full or closed).");
		return;
	}
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel(Connect + TravelOptions(), ETravelType::TRAVEL_Absolute);
	}
}

void UAIBSessionSubsystem::JoinByAddress(const FString& Address)
{
	FString Clean = Address.TrimStartAndEnd();
	if (Clean.IsEmpty())
	{
		return;
	}
	Status = FText::Format(LOCTEXT("Connecting", "Connecting to {0}..."), FText::FromString(Clean));
	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel(Clean + TravelOptions(), ETravelType::TRAVEL_Absolute);
	}
}

bool UAIBSessionSubsystem::ShowInviteUI()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS || !HasOnlinePlatform())
	{
		return false;
	}
	IOnlineExternalUIPtr UI = OSS->GetExternalUIInterface();
	return UI.IsValid() && UI->ShowInviteUI(0, NAME_GameSession);
}

void UAIBSessionSubsystem::LeaveToMenu()
{
	IOnlineSessionPtr Sessions = GetSessions();
	if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		bLeaveAfterDestroy = true;
		Sessions->DestroySession(NAME_GameSession);
		return;
	}
	if (UAIBGameInstance* GI = Cast<UAIBGameInstance>(GetGameInstance()))
	{
		GI->OpenMainMenu();
	}
}

void UAIBSessionSubsystem::OnDestroyComplete(FName SessionName, bool bSuccess)
{
	if (bHostAfterDestroy)
	{
		bHostAfterDestroy = false;
		HostLobby(bPendingLAN, PendingBots, bPendingAutoReady);
		return;
	}
	if (PendingJoin.IsSet())
	{
		const FOnlineSessionSearchResult Result = PendingJoin.GetValue();
		PendingJoin.Reset();
		bBusy = true;
		GetSessions()->JoinSession(0, NAME_GameSession, Result);
		return;
	}
	if (bLeaveAfterDestroy)
	{
		bLeaveAfterDestroy = false;
		if (UAIBGameInstance* GI = Cast<UAIBGameInstance>(GetGameInstance()))
		{
			GI->OpenMainMenu();
		}
	}
}

void UAIBSessionSubsystem::OnInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		return;
	}
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		PendingJoin = InviteResult;
		Sessions->DestroySession(NAME_GameSession);
		return;
	}
	bBusy = true;
	Status = LOCTEXT("JoiningInvite", "Joining your friend's lobby...");
	Sessions->JoinSession(0, NAME_GameSession, InviteResult);
}

#undef LOCTEXT_NAMESPACE
