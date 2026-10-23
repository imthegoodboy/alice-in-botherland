// Alice in Botherland - player controller: input, cameras, RPCs and the HUD

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/AIBTypes.h"
#include "InputActionValue.h"
#include "AIBPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ACameraActor;
class AAIBGameState;
class AAIBPlayerState;
class SAIBHud;

UCLASS()
class ALICEINBOTHERLAND_API AAIBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAIBPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	// ---------------- Server RPCs ----------------
	UFUNCTION(Server, Reliable) void ServerSetReady(bool bReady);
	UFUNCTION(Server, Reliable) void ServerAddBot();
	UFUNCTION(Server, Reliable) void ServerFillBots();
	UFUNCTION(Server, Reliable) void ServerRemoveSeat(int32 Slot);
	UFUNCTION(Server, Reliable) void ServerStartMatch();
	UFUNCTION(Server, Reliable) void ServerSetAITestMode(bool bOn);
	UFUNCTION(Server, Reliable) void ServerDiscoverLore(int32 Index);
	UFUNCTION(Server, Reliable) void ServerSetTalking(bool bTalking);
	UFUNCTION(Server, Reliable) void ServerSkipCinematic();
	UFUNCTION(Server, Reliable) void ServerReturnToLobby();
	UFUNCTION(Server, Reliable) void ServerSubmitNumber(int32 Number);
	UFUNCTION(Server, Reliable) void ServerSubmitGuess(EAIBSymbol Guess);
	UFUNCTION(Server, Reliable) void ServerSendChat(const FString& Text, int32 ToSlot, EAIBSymbol Claim);
	UFUNCTION(Server, Reliable) void ServerFinalSelect(EAIBFinalChoice Choice);
	UFUNCTION(Server, Reliable) void ServerFinalConfirm(bool bConfirm);
	UFUNCTION(Server, Reliable) void ServerSetSwitchInteract(bool bHolding);

	// ---------------- Client RPCs ----------------
	UFUNCTION(Client, Reliable) void ClientReceiveChat(const FAIBChatMessage& Message);
	UFUNCTION(Client, Reliable) void ClientReceiveSymbols(const TArray<FAIBSymbolView>& Views);
	UFUNCTION(Client, Reliable) void ClientNotify(const FText& Message, FName CueId);
	UFUNCTION(Client, Reliable) void ClientRecordMatch(const FAIBMatchRecord& Record);
	UFUNCTION(Client, Reliable) void ClientFaceRotation(FRotator Rotation);

	// ---------------- Local state for the HUD ----------------
	AAIBGameState* GetAIBGameState() const;
	AAIBPlayerState* GetAIBPlayerState() const;
	int32 GetLocalSlot() const;
	bool IsHost() const;
	bool IsSpectating() const;
	const TArray<FAIBChatMessage>& GetChatLog() const { return ChatLog; }
	EAIBSymbol GetVisibleSymbol(int32 Slot) const;
	const TMap<int32, EAIBSymbol>& GetVisibleSymbols() const { return VisibleSymbols; }
	APawn* GetSpectateTarget() const { return SpectateTarget.Get(); }
	bool HasLocallySkipped() const { return bLocalSkip; }
	/** Marker stone the champion is standing at (INDEX_NONE if none) - the HUD shows the read prompt. */
	int32 GetNearLore() const { return NearLore; }
	/** Marker stones read this match (server state merged with the ones just read locally). */
	int32 GetLoreFound() const;
	/** Voice chat: microphone currently sending. */
	bool IsSpeaking() const { return bSpeaking; }

	/** Number currently dialled in on the selector (not yet sent). */
	int32 DraftNumber = 50;
	EAIBSymbol DraftGuess = EAIBSymbol::None;
	bool bHoldingInteract = false;

	/** Toasts shown on screen (text + local time). */
	struct FToast { FText Text; float Time; };
	TArray<FToast> Toasts;

	void TogglePauseMenu();
	void LeaveMatch();
	void SendChatMessage(const FString& Text, int32 ToSlot = INDEX_NONE, EAIBSymbol Claim = EAIBSymbol::None);
	void RequestSkip();

	float MouseSensitivity = 1.f;
	bool bInvertY = false;
	float FieldOfView = 80.f;
	int32 MicMode = 1;

protected:
	void BuildInput();
	void UpdateCamera(float DeltaTime);
	void UpdateZone();
	void UpdateSymbols();
	void UpdateCountdownSounds();
	void UpdateInputMode();
	/** Plays Alice's lines for the current phase in step with the phase clock. */
	void UpdateNarration();
	void UpdateVoiceChat();
	void UpdateLore();
	void ApplySettings();
	void ChooseSpectateTarget(int32 Direction);

	// Input handlers
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpCompleted();
	void OnSprint(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	void OnChat();
	void OnScoreboard(const FInputActionValue& Value);
	void OnPause();
	void OnThrottle(const FInputActionValue& Value);
	void OnSteer(const FInputActionValue& Value);
	void OnHandbrake(const FInputActionValue& Value);
	void OnResetVehicle();
	void OnSpectateNext();
	void OnSpectatePrev();
	void OnTalk(const FInputActionValue& Value);

	bool IsMovementAllowed() const;

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> IA_Move;
	UPROPERTY() TObjectPtr<UInputAction> IA_Look;
	UPROPERTY() TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY() TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY() TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY() TObjectPtr<UInputAction> IA_Chat;
	UPROPERTY() TObjectPtr<UInputAction> IA_Scoreboard;
	UPROPERTY() TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY() TObjectPtr<UInputAction> IA_Throttle;
	UPROPERTY() TObjectPtr<UInputAction> IA_Steer;
	UPROPERTY() TObjectPtr<UInputAction> IA_Handbrake;
	UPROPERTY() TObjectPtr<UInputAction> IA_Reset;
	UPROPERTY() TObjectPtr<UInputAction> IA_SpectateNext;
	UPROPERTY() TObjectPtr<UInputAction> IA_SpectatePrev;
	UPROPERTY() TObjectPtr<UInputAction> IA_Talk;

	UPROPERTY() TObjectPtr<ACameraActor> CineCamera;

	TSharedPtr<SAIBHud> Hud;
	TArray<FAIBChatMessage> ChatLog;
	TMap<int32, EAIBSymbol> VisibleSymbols;
	TWeakObjectPtr<APawn> SpectateTarget;

	EAIBSceneZone LastZone = EAIBSceneZone::Lobby;
	bool bZoneInit = false;
	bool bCursorMode = false;
	bool bInputModeInit = false;
	bool bLocalSkip = false;
	int32 LastTickSecond = -1;
	int32 LastPhaseSerial = -1;
	int32 LastShot = -1;
	int32 NarrationSerial = -1;
	int32 NarrationBeat = INDEX_NONE;
	int32 NearLore = INDEX_NONE;
	int32 LocalLoreFound = 0;
	bool bTalkHeld = false;
	bool bSpeaking = false;
	bool bLobbyWelcomed = false;
	bool bVoiceRegistered = false;
};
