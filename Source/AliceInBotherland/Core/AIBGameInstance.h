// Alice in Botherland - game instance: profile, settings, travel errors, loading screen

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "Core/AIBSaveGame.h"
#include "AIBGameInstance.generated.h"

UCLASS()
class ALICEINBOTHERLAND_API UAIBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

	static const TCHAR* IslandMap;

	UAIBSaveGame* GetProfile();
	const FAIBSettings& GetSettings();
	void SetSettings(const FAIBSettings& NewSettings);
	void ApplySettings();
	void SaveProfile();
	void RecordMatch(const FAIBMatchRecord& Record);

	FString GetPlayerName();

	/** Message to show on the main menu after a failed join / disconnect. */
	FText PendingMenuMessage;

	/** Opens the main menu (island map with the menu game mode). */
	void OpenMainMenu();

protected:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* LoadedWorld);

	UPROPERTY()
	TObjectPtr<UAIBSaveGame> Profile;

	FDelegateHandle PreLoadHandle;
	FDelegateHandle PostLoadHandle;
};
