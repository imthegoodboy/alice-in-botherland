// Alice in Botherland - online lobbies: host, browse, join, invite (Steam or LAN)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "AIBSessionSubsystem.generated.h"

struct FAIBLobbyInfo
{
	FString HostName;
	int32 Players = 0;
	int32 MaxPlayers = 5;
	int32 Ping = 0;
	int32 SearchIndex = INDEX_NONE;
	bool bInProgress = false;
};

UCLASS()
class ALICEINBOTHERLAND_API UAIBSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Creates a session and opens the island as a listen server. Bots pre-fill seats (solo practice = 4). bAutoReady readies the host too (quick play). */
	void HostLobby(bool bLAN, int32 Bots, bool bAutoReady = false);
	void FindLobbies(bool bLAN);
	void JoinLobby(int32 ResultIndex);
	void JoinByAddress(const FString& Address);
	/** Opens the platform invite overlay. Returns false when the platform has none (LAN / no Steam). */
	bool ShowInviteUI();
	/** Destroys the session (if any) and returns to the main menu. */
	void LeaveToMenu();

	bool IsSearching() const { return bSearching; }
	bool IsBusy() const { return bBusy; }
	const TArray<FAIBLobbyInfo>& GetResults() const { return Results; }
	FText GetStatus() const { return Status; }
	FString GetPlatformName() const;
	bool HasOnlinePlatform() const;
	FString GetLocalAddress() const;

private:
	IOnlineSessionPtr GetSessions() const;
	void TravelToHost(const FString& Options);
	void OnCreateComplete(FName SessionName, bool bSuccess);
	void OnFindComplete(bool bSuccess);
	void OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroyComplete(FName SessionName, bool bSuccess);
	void OnInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	FString TravelOptions() const;

	TSharedPtr<FOnlineSessionSearch> Search;
	TArray<FAIBLobbyInfo> Results;
	FText Status;
	bool bSearching = false;
	bool bBusy = false;
	bool bPendingLAN = false;
	int32 PendingBots = 0;
	bool bPendingAutoReady = false;
	bool bLeaveAfterDestroy = false;
	bool bHostAfterDestroy = false;
	TOptional<FOnlineSessionSearchResult> PendingJoin;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle InviteHandle;
};
