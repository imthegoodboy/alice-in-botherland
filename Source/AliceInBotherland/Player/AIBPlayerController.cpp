// Alice in Botherland - player controller: input, cameras, RPCs and the HUD

#include "Player/AIBPlayerController.h"
#include "Player/AIBCharacter.h"
#include "Player/AIBVehicle.h"
#include "Game/AIBGameMode.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Core/AIBGameInstance.h"
#include "Audio/AIBAudioSubsystem.h"
#include "Audio/AIBNarration.h"
#include "AliceInBotherland.h"
#include "UI/AIBStyle.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/VoiceInterface.h"
#include "VoipListenerSynthComponent.h"
#include "UObject/UObjectIterator.h"
#include "World/AIBLoreStones.h"
#include "Online/AIBSessionSubsystem.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBFleet.h"
#include "World/AIBNumberArena.h"
#include "World/AIBHotelArena.h"
#include "World/AIBGuardianSite.h"
#include "UI/SAIBHud.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"

#define LOCTEXT_NAMESPACE "AIBPlayerController"

AAIBPlayerController::AAIBPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
}

AAIBGameState* AAIBPlayerController::GetAIBGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<AAIBGameState>() : nullptr;
}

AAIBPlayerState* AAIBPlayerController::GetAIBPlayerState() const
{
	return GetPlayerState<AAIBPlayerState>();
}

int32 AAIBPlayerController::GetLocalSlot() const
{
	const AAIBPlayerState* PS = GetAIBPlayerState();
	return PS ? PS->Slot : INDEX_NONE;
}

bool AAIBPlayerController::IsHost() const
{
	const AAIBPlayerState* PS = GetAIBPlayerState();
	return PS && PS->bHost;
}

bool AAIBPlayerController::IsSpectating() const
{
	const AAIBPlayerState* PS = GetAIBPlayerState();
	const AAIBGameState* GS = GetAIBGameState();
	return GS && GS->bMatchInProgress && (!PS || !PS->IsActiveChampion());
}

EAIBSymbol AAIBPlayerController::GetVisibleSymbol(int32 Slot) const
{
	const EAIBSymbol* Found = VisibleSymbols.Find(Slot);
	return Found ? *Found : EAIBSymbol::None;
}

// ---------------------------------------------------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------------------------------------------------

void AAIBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	ApplySettings();

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	CineCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	if (CineCamera)
	{
		CineCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		CineCamera->GetCameraComponent()->SetFieldOfView(60.f);
		SetViewTarget(CineCamera);
	}

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Hud = SNew(SAIBHud).Owner(this);
		Viewport->AddViewportWidgetForPlayer(GetLocalPlayer(), Hud.ToSharedRef(), 10);
	}
	UpdateInputMode();
}

void AAIBPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Hud.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetForPlayer(GetLocalPlayer(), Hud.ToSharedRef());
		}
		Hud.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------------------------------

void AAIBPlayerController::BuildInput()
{
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_AIB"));
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	IA_Move = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	IA_Look = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Jump = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	IA_Sprint = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	IA_Interact = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	IA_Chat = MakeAction(TEXT("IA_Chat"), EInputActionValueType::Boolean);
	IA_Scoreboard = MakeAction(TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);
	IA_Pause = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean);
	IA_Throttle = MakeAction(TEXT("IA_Throttle"), EInputActionValueType::Axis1D);
	IA_Steer = MakeAction(TEXT("IA_Steer"), EInputActionValueType::Axis1D);
	IA_Handbrake = MakeAction(TEXT("IA_Handbrake"), EInputActionValueType::Boolean);
	IA_Reset = MakeAction(TEXT("IA_Reset"), EInputActionValueType::Boolean);
	IA_SpectateNext = MakeAction(TEXT("IA_SpectateNext"), EInputActionValueType::Boolean);
	IA_SpectatePrev = MakeAction(TEXT("IA_SpectatePrev"), EInputActionValueType::Boolean);
	IA_Talk = MakeAction(TEXT("IA_Talk"), EInputActionValueType::Boolean);
	IA_MicToggle = MakeAction(TEXT("IA_MicToggle"), EInputActionValueType::Boolean);

	auto Negate = [this]() { return NewObject<UInputModifierNegate>(Mapping); };
	auto SwizzleYX = [this]()
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Mapping);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		return Swizzle;
	};

	// WASD -> 2D move (X = right, Y = forward).
	Mapping->MapKey(IA_Move, EKeys::W).Modifiers.Add(SwizzleYX());
	{
		FEnhancedActionKeyMapping& S = Mapping->MapKey(IA_Move, EKeys::S);
		S.Modifiers.Add(SwizzleYX());
		S.Modifiers.Add(Negate());
	}
	Mapping->MapKey(IA_Move, EKeys::A).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Move, EKeys::D);
	Mapping->MapKey(IA_Move, EKeys::Gamepad_Left2D);

	Mapping->MapKey(IA_Look, EKeys::Mouse2D);
	Mapping->MapKey(IA_Look, EKeys::Gamepad_Right2D);

	Mapping->MapKey(IA_Jump, EKeys::SpaceBar);
	Mapping->MapKey(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	Mapping->MapKey(IA_Sprint, EKeys::LeftShift);
	Mapping->MapKey(IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	Mapping->MapKey(IA_Interact, EKeys::E);
	Mapping->MapKey(IA_Interact, EKeys::Gamepad_FaceButton_Left);
	Mapping->MapKey(IA_Chat, EKeys::Enter);
	Mapping->MapKey(IA_Chat, EKeys::T);
	Mapping->MapKey(IA_Scoreboard, EKeys::Tab);
	Mapping->MapKey(IA_Scoreboard, EKeys::Gamepad_Special_Left);
	Mapping->MapKey(IA_Pause, EKeys::Escape);
	Mapping->MapKey(IA_Pause, EKeys::Gamepad_Special_Right);

	Mapping->MapKey(IA_Throttle, EKeys::W);
	Mapping->MapKey(IA_Throttle, EKeys::Up);
	Mapping->MapKey(IA_Throttle, EKeys::S).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Throttle, EKeys::Down).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Throttle, EKeys::Gamepad_RightTriggerAxis);
	Mapping->MapKey(IA_Throttle, EKeys::Gamepad_LeftTriggerAxis).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Steer, EKeys::D);
	Mapping->MapKey(IA_Steer, EKeys::Right);
	Mapping->MapKey(IA_Steer, EKeys::A).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Steer, EKeys::Left).Modifiers.Add(Negate());
	Mapping->MapKey(IA_Steer, EKeys::Gamepad_LeftX);
	Mapping->MapKey(IA_Handbrake, EKeys::SpaceBar);
	Mapping->MapKey(IA_Handbrake, EKeys::Gamepad_FaceButton_Right);
	Mapping->MapKey(IA_Reset, EKeys::R);
	Mapping->MapKey(IA_Reset, EKeys::Gamepad_FaceButton_Top);
	Mapping->MapKey(IA_SpectateNext, EKeys::LeftMouseButton);
	Mapping->MapKey(IA_SpectateNext, EKeys::Gamepad_RightShoulder);
	Mapping->MapKey(IA_SpectatePrev, EKeys::RightMouseButton);
	Mapping->MapKey(IA_SpectatePrev, EKeys::Gamepad_LeftShoulder);
	Mapping->MapKey(IA_Talk, EKeys::V);
	Mapping->MapKey(IA_Talk, EKeys::Gamepad_DPad_Up);
	Mapping->MapKey(IA_MicToggle, EKeys::M);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(Mapping, 0);
	}
}

void AAIBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsLocalController() || !GetLocalPlayer())
	{
		return;
	}
	BuildInput();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AAIBPlayerController::OnMove);
	Input->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AAIBPlayerController::OnLook);
	Input->BindAction(IA_Jump, ETriggerEvent::Started, this, &AAIBPlayerController::OnJumpStarted);
	Input->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AAIBPlayerController::OnJumpCompleted);
	Input->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AAIBPlayerController::OnSprint);
	Input->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AAIBPlayerController::OnSprint);
	Input->BindAction(IA_Interact, ETriggerEvent::Started, this, &AAIBPlayerController::OnInteract);
	Input->BindAction(IA_Interact, ETriggerEvent::Completed, this, &AAIBPlayerController::OnInteract);
	Input->BindAction(IA_Chat, ETriggerEvent::Started, this, &AAIBPlayerController::OnChat);
	Input->BindAction(IA_Scoreboard, ETriggerEvent::Started, this, &AAIBPlayerController::OnScoreboard);
	Input->BindAction(IA_Scoreboard, ETriggerEvent::Completed, this, &AAIBPlayerController::OnScoreboard);
	Input->BindAction(IA_Pause, ETriggerEvent::Started, this, &AAIBPlayerController::OnPause);
	Input->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AAIBPlayerController::OnThrottle);
	Input->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &AAIBPlayerController::OnThrottle);
	Input->BindAction(IA_Steer, ETriggerEvent::Triggered, this, &AAIBPlayerController::OnSteer);
	Input->BindAction(IA_Steer, ETriggerEvent::Completed, this, &AAIBPlayerController::OnSteer);
	Input->BindAction(IA_Handbrake, ETriggerEvent::Started, this, &AAIBPlayerController::OnHandbrake);
	Input->BindAction(IA_Handbrake, ETriggerEvent::Completed, this, &AAIBPlayerController::OnHandbrake);
	Input->BindAction(IA_Reset, ETriggerEvent::Started, this, &AAIBPlayerController::OnResetVehicle);
	Input->BindAction(IA_SpectateNext, ETriggerEvent::Started, this, &AAIBPlayerController::OnSpectateNext);
	Input->BindAction(IA_SpectatePrev, ETriggerEvent::Started, this, &AAIBPlayerController::OnSpectatePrev);
	Input->BindAction(IA_Talk, ETriggerEvent::Started, this, &AAIBPlayerController::OnTalk);
	Input->BindAction(IA_Talk, ETriggerEvent::Completed, this, &AAIBPlayerController::OnTalk);
	Input->BindAction(IA_MicToggle, ETriggerEvent::Started, this, &AAIBPlayerController::OnMicToggle);
}

bool AAIBPlayerController::IsMovementAllowed() const
{
	const AAIBGameState* GS = GetAIBGameState();
	if (!GS || (Hud.IsValid() && Hud->BlocksGameInput()))
	{
		return false;
	}
	switch (GS->Phase)
	{
	case EAIBPhase::IslandExplore:
	case EAIBPhase::Stage1Briefing:
	case EAIBPhase::Stage2Briefing:
	case EAIBPhase::Stage2Communication:
	case EAIBPhase::Stage2Guess:
	case EAIBPhase::Stage2Results:
	case EAIBPhase::Stage3Race:
		return true;
	default:
		return false;
	}
}

void AAIBPlayerController::OnMove(const FInputActionValue& Value)
{
	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	if (!MyCharacter || !IsMovementAllowed())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	MyCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	MyCharacter->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AAIBPlayerController::OnLook(const FInputActionValue& Value)
{
	if (Hud.IsValid() && Hud->WantsCursor())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>() * MouseSensitivity;
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		Vehicle->LookAround(Axis.X * 2.f);
		return;
	}
	AddYawInput(Axis.X);
	AddPitchInput(bInvertY ? Axis.Y : -Axis.Y);
}

void AAIBPlayerController::OnJumpStarted()
{
	const AAIBGameState* GS = GetAIBGameState();
	if (GS && AIB::IsSkippablePhase(GS->Phase))
	{
		RequestSkip();
		return;
	}
	if (ACharacter* MyCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (IsMovementAllowed())
		{
			MyCharacter->Jump();
		}
	}
}

void AAIBPlayerController::OnJumpCompleted()
{
	if (ACharacter* MyCharacter = Cast<ACharacter>(GetPawn()))
	{
		MyCharacter->StopJumping();
	}
}

void AAIBPlayerController::OnSprint(const FInputActionValue& Value)
{
	if (AAIBCharacter* MyCharacter = Cast<AAIBCharacter>(GetPawn()))
	{
		MyCharacter->ServerSetSprinting(Value.Get<bool>());
	}
}

void AAIBPlayerController::OnTalk(const FInputActionValue& Value)
{
	bTalkHeld = Value.Get<bool>();
}

void AAIBPlayerController::OnMicToggle()
{
	if (Hud.IsValid() && Hud->BlocksGameInput())
	{
		return;
	}
	ToggleMicMute();
}

void AAIBPlayerController::ToggleMicMute()
{
	if (MicMode != 0)
	{
		MicModeBeforeMute = MicMode;
		SetMicMode(0);
	}
	else
	{
		SetMicMode(MicModeBeforeMute == 0 ? 1 : MicModeBeforeMute);
	}
}

void AAIBPlayerController::SetMicMode(int32 NewMode)
{
	UAIBGameInstance* GI = GetGameInstance<UAIBGameInstance>();
	if (!GI)
	{
		return;
	}
	FAIBSettings S = GI->GetSettings();
	S.MicMode = FMath::Clamp(NewMode, 0, 2);
	GI->SetSettings(S);
	MicMode = S.MicMode;
	const FText Message = MicMode == 0 ? LOCTEXT("MicToastOff", "Microphone OFF  (M to turn it on)")
		: MicMode == 2 ? LOCTEXT("MicToastOpen", "Microphone ON - open mic")
		: LOCTEXT("MicToastPtt", "Microphone ON - hold V to talk");
	Toasts.Add({ Message, (float)GetWorld()->GetTimeSeconds() });
	if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
	{
		Audio->PlayCue(TEXT("SFX_UI_Click"));
	}
}

bool AAIBPlayerController::IsMutedLocally(const AAIBPlayerState* PS) const
{
	return PS && PS->GetUniqueId().IsValid() && LocallyMuted.Contains(PS->GetUniqueId());
}

void AAIBPlayerController::ToggleMuteLocally(const AAIBPlayerState* PS)
{
	if (!PS || PS->bBot || !PS->GetUniqueId().IsValid() || PS == GetAIBPlayerState())
	{
		return;
	}
	const FUniqueNetIdRepl& Id = PS->GetUniqueId();
	if (LocallyMuted.Contains(Id))
	{
		LocallyMuted.Remove(Id);
		ServerUnmutePlayer(Id);
	}
	else
	{
		LocallyMuted.Add(Id);
		ServerMutePlayer(Id);
	}
}

int32 AAIBPlayerController::GetLoreFound() const
{
	const AAIBPlayerState* PS = GetAIBPlayerState();
	return LocalLoreFound | (PS ? PS->LoreFound : 0);
}

void AAIBPlayerController::OnInteract(const FInputActionValue& Value)
{
	const bool bHeld = Value.Get<bool>();
	if (bHeld && NearLore != INDEX_NONE)
	{
		// Read a marker stone: Alice tells this part of the island's past.
		const int32 Index = NearLore;
		LocalLoreFound |= 1 << Index;
		ServerDiscoverLore(Index);
		if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->PlaySfx(TEXT("SFX_Correct"), 0.6f);
			Audio->PlayVoice(AAIBLoreStones::LineFor(Index));
		}
		const int32 Found = int32(FMath::CountBits64(uint64(GetLoreFound())));
		Toasts.Add({ FText::Format(LOCTEXT("LoreFound", "Marker stone read  ({0} of {1})"), FText::AsNumber(Found), FText::AsNumber(AAIBLoreStones::Count)), float(GetWorld()->GetTimeSeconds()) });
		NearLore = INDEX_NONE;
		return;
	}
}

void AAIBPlayerController::OnChat()
{
	const AAIBGameState* GS = GetAIBGameState();
	if (GS && AIB::IsSkippablePhase(GS->Phase))
	{
		RequestSkip();
		return;
	}
	if (Hud.IsValid())
	{
		Hud->OpenChat();
	}
}

void AAIBPlayerController::OnScoreboard(const FInputActionValue& Value)
{
	if (Hud.IsValid())
	{
		Hud->SetScoreboardVisible(Value.Get<bool>());
	}
}

void AAIBPlayerController::OnPause()
{
	TogglePauseMenu();
}

void AAIBPlayerController::OnThrottle(const FInputActionValue& Value)
{
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		Vehicle->ApplyThrottle(IsMovementAllowed() ? Value.Get<float>() : 0.f);
	}
}

void AAIBPlayerController::OnSteer(const FInputActionValue& Value)
{
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		Vehicle->ApplySteering(IsMovementAllowed() ? Value.Get<float>() : 0.f);
	}
}

void AAIBPlayerController::OnHandbrake(const FInputActionValue& Value)
{
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		Vehicle->ApplyHandbrake(Value.Get<bool>());
	}
}

void AAIBPlayerController::OnResetVehicle()
{
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		Vehicle->ServerRequestReset();
	}
}

void AAIBPlayerController::OnSpectateNext()
{
	if (IsSpectating() && !(Hud.IsValid() && Hud->WantsCursor()))
	{
		ChooseSpectateTarget(1);
	}
}

void AAIBPlayerController::OnSpectatePrev()
{
	if (IsSpectating() && !(Hud.IsValid() && Hud->WantsCursor()))
	{
		ChooseSpectateTarget(-1);
	}
}

void AAIBPlayerController::ChooseSpectateTarget(int32 Direction)
{
	TArray<APawn*> Candidates;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* CandidatePawn = *It;
		const AAIBPlayerState* PS = CandidatePawn->GetPlayerState<AAIBPlayerState>();
		const bool bVisible = !CandidatePawn->IsHidden();
		if (PS && PS->IsActiveChampion() && bVisible && (CandidatePawn->IsA<AAIBCharacter>() || CandidatePawn->IsA<AAIBVehicle>()))
		{
			Candidates.Add(CandidatePawn);
		}
	}
	if (Candidates.Num() == 0)
	{
		SpectateTarget = nullptr;
		return;
	}
	const int32 Current = Candidates.IndexOfByKey(SpectateTarget.Get());
	const int32 Next = Current == INDEX_NONE ? 0 : (Current + Direction + Candidates.Num()) % Candidates.Num();
	SpectateTarget = Candidates[Next];
}

void AAIBPlayerController::RequestSkip()
{
	if (!bLocalSkip)
	{
		bLocalSkip = true;
		ServerSkipCinematic();
	}
}

void AAIBPlayerController::TogglePauseMenu()
{
	if (Hud.IsValid())
	{
		Hud->TogglePause();
	}
}

void AAIBPlayerController::LeaveMatch()
{
	UE_LOG(LogAIB, Log, TEXT("Leaving the match for the main menu (phase %s)"), GetAIBGameState() ? *UEnum::GetValueAsString(GetAIBGameState()->Phase) : TEXT("?"));
	if (GetNetMode() == NM_Client)
	{
		// Tell the host this was on purpose (no seat held, a quitter forfeits at once).
		ServerLeaving();
	}
	if (UAIBSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UAIBSessionSubsystem>())
	{
		Sessions->LeaveToMenu();
	}
}

void AAIBPlayerController::SendChatMessage(const FString& Text, int32 ToSlot, EAIBSymbol Claim)
{
	if (!Text.TrimStartAndEnd().IsEmpty())
	{
		ServerSendChat(Text, ToSlot, Claim);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Tick: cameras, zones, symbols, sounds, input mode
// ---------------------------------------------------------------------------------------------------------------------

void AAIBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController())
	{
		return;
	}
	const AAIBGameState* GS = GetAIBGameState();
	if (GS && GS->PhaseSerial != LastPhaseSerial)
	{
		LastPhaseSerial = GS->PhaseSerial;
		DraftGuess = EAIBSymbol::None;
		bLocalSkip = false;
		if (GS->Phase == EAIBPhase::StoryIntro && ChatLog.RemoveAll([](const FAIBChatMessage& Message) { return Message.bSystem; }) > 0 && Hud.IsValid())
		{
			// Lobby seat/leave notices are noise once the voyage starts.
			Hud->RebuildChat();
		}
	}
	ApplySettings();
	UpdateZone();
	UpdateCamera(DeltaTime);
	UpdateNarration();
	UpdateVoiceChat();
	UpdateDraftNumber();
	UpdateLore();
	UpdateSymbols();
	UpdateCountdownSounds();
	UpdateInputMode();

	const float Now = GetWorld()->GetTimeSeconds();
	Toasts.RemoveAll([Now](const FToast& Toast) { return Now - Toast.Time > 5.f; });
}

void AAIBPlayerController::UpdateZone()
{
	const AAIBGameState* GS = GetAIBGameState();
	EAIBSceneZone Zone = GS ? AIB::ZoneForPhase(GS->Phase) : EAIBSceneZone::Lobby;
	if (GS && GS->Phase == EAIBPhase::Results)
	{
		Zone = EAIBSceneZone::Guardian;
	}
	if (bZoneInit && Zone == LastZone)
	{
		return;
	}
	bZoneInit = true;
	LastZone = Zone;
	if (UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>())
	{
		WorldSys->SetLocalZone(Zone);
	}
	if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
	{
		Audio->SetAmbienceForZone(Zone);
	}
}

void AAIBPlayerController::UpdateCamera(float DeltaTime)
{
	const AAIBGameState* GS = GetAIBGameState();
	UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>();
	if (!GS || !WorldSys || !CineCamera)
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	const float Elapsed = GS->GetPhaseElapsed();
	const int32 Slot = GetLocalSlot();
	const bool bSpectating = IsSpectating();
	APawn* MyPawn = GetPawn();

	// Pick a mode: a cinematic transform, follow a pawn, or our own pawn camera.
	bool bCine = false;
	FTransform Shot;
	float FOV = 60.f;
	AActor* Follow = nullptr;

	AAIBFleet* Fleet = WorldSys->GetFleet();
	AAIBNumberArena* Numbers = WorldSys->GetNumberArena();
	AAIBHotelArena* Hotel = WorldSys->GetHotel();
	AAIBGuardianSite* Guardian = WorldSys->GetGuardian();

	// The Guardian's judgement: close on the champion speaking their last words, then low and wide to watch the beam come down.
	auto FallShot = [&](FTransform& OutShot, float& OutFOV) -> bool
	{
		if (GS->Fallen.Num() == 0)
		{
			return false;
		}
		int32 Speaker = 0;
		for (int32 Index = 0; Index < GS->Fallen.Num(); ++Index)
		{
			if (Elapsed >= GS->Fallen[Index].SpeakAt - 0.4f)
			{
				Speaker = Index;
			}
		}
		const AAIBCharacter* Target = nullptr;
		for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
		{
			const AAIBPlayerState* PS = It->GetChampionState();
			if (PS && PS->Slot == GS->Fallen[Speaker].Slot)
			{
				Target = *It;
				break;
			}
		}
		if (!Target)
		{
			return false;
		}
		const FVector Base = Target->GetActorLocation();
		const FVector Fwd = Target->GetActorForwardVector().GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);
		const FVector Head = Base + FVector(0.f, 0.f, 58.f);
		const float Push = FMath::Clamp((Elapsed - GS->Fallen[Speaker].SpeakAt) / 5.f, 0.f, 1.f);
		// Slightly below eye level, so the bowed face is seen rather than the top of the head.
		const FVector CloseEye = Head + Fwd * FMath::Lerp(320.f, 250.f, Push) + Side * 60.f - FVector(0.f, 0.f, 48.f);
		const FVector CloseLook = Head - FVector(0.f, 0.f, 6.f);
		const FTransform Close(FRotationMatrix::MakeFromX(CloseLook - CloseEye).Rotator(), CloseEye);
		// Three-quarter front and low: the whole champion (and where they fall) with the sky above them.
		const FVector WideEye = Base + Side * 300.f + Fwd * 280.f + FVector(0.f, 0.f, 20.f);
		const FVector WideLook = Base + FVector(0.f, 0.f, 60.f);
		const FTransform Wide(FRotationMatrix::MakeFromX(WideLook - WideEye).Rotator(), WideEye);
		const float ToWide = FMath::SmoothStep(GS->FallStrikeAt - 2.f, GS->FallStrikeAt - 0.9f, Elapsed);
		OutShot.Blend(Close, Wide, ToWide);
		OutFOV = FMath::Lerp(36.f, 72.f, ToWide);
		return true;
	};

	auto SpectatePawn = [&]() -> AActor*
	{
		// Move on when the watched champion is gone, hidden, or has since been eliminated.
		const AAIBPlayerState* TargetPS = SpectateTarget.IsValid() ? SpectateTarget->GetPlayerState<AAIBPlayerState>() : nullptr;
		if (!SpectateTarget.IsValid() || SpectateTarget->IsHidden() || !TargetPS || !TargetPS->IsActiveChampion())
		{
			ChooseSpectateTarget(0);
		}
		return SpectateTarget.Get();
	};

	switch (GS->Phase)
	{
	case EAIBPhase::None:
	case EAIBPhase::Lobby:
	case EAIBPhase::LobbyCountdown:
	case EAIBPhase::StoryIntro:
		if (Fleet)
		{
			bCine = true;
			Shot = Fleet->GetLobbyCamera(Time);
			FOV = 55.f;
		}
		break;
	case EAIBPhase::ArrivalCinematic:
		if (Fleet)
		{
			bCine = true;
			Shot = Fleet->GetArrivalCamera(AIB::ArrivalSkipTo + Elapsed, FOV);
		}
		break;
	case EAIBPhase::IslandExplore:
	case EAIBPhase::Stage1Briefing:
	case EAIBPhase::Stage2Briefing:
	case EAIBPhase::Stage2Communication:
	case EAIBPhase::Stage2Guess:
		// Free movement (including while Alice explains a trial's rules).
		Follow = (MyPawn && !bSpectating) ? MyPawn : SpectatePawn();
		break;
	case EAIBPhase::Stage1Elimination:
		if (FallShot(Shot, FOV))
		{
			bCine = true;
		}
		else if (Numbers)
		{
			bCine = true;
			Shot = Numbers->GetOverviewCamera(Time);
		}
		break;
	case EAIBPhase::Stage1RoundIntro:
	case EAIBPhase::Stage1Selection:
	case EAIBPhase::Stage1Results:
		if (Numbers)
		{
			bCine = true;
			const bool bSeated = Slot != INDEX_NONE && GS->Stage1Participants.Contains(Slot);
			Shot = bSeated ? Numbers->GetSeatCamera(Slot) : Numbers->GetOverviewCamera(Time);
			FOV = 62.f;
		}
		break;
	case EAIBPhase::Stage1Reveal:
		if (Numbers)
		{
			bCine = true;
			Shot = Numbers->GetRevealCamera(Elapsed);
			FOV = 50.f;
		}
		break;
	case EAIBPhase::Stage2Results:
	case EAIBPhase::Stage2Elimination:
		if (GS->Phase == EAIBPhase::Stage2Elimination && FallShot(Shot, FOV))
		{
			bCine = true;
		}
		else if (Hotel)
		{
			bCine = true;
			Shot = Hotel->GetOverviewCamera(Time);
			FOV = 65.f;
		}
		break;
	case EAIBPhase::Stage3Briefing:
	case EAIBPhase::Stage3Countdown:
	case EAIBPhase::Stage3Race:
		Follow = (MyPawn && MyPawn->IsA<AAIBVehicle>()) ? MyPawn : SpectatePawn();
		break;
	case EAIBPhase::Stage3Finish:
		if (Guardian)
		{
			bCine = true;
			Shot = Guardian->GetArrivalCamera(Elapsed);
			FOV = 55.f;
		}
		break;
	case EAIBPhase::FinalBriefing:
		if (Guardian)
		{
			bCine = true;
			Shot = Guardian->GetFinalWideCamera(Time);
			FOV = 50.f;
		}
		break;
	case EAIBPhase::FinalChoice:
	case EAIBPhase::FinalResolve:
		if (Guardian)
		{
			bCine = true;
			const int32 Chooser = FMath::Max(0, GS->ChooserIndex);
			// Alternate: close-up on the chooser, then the wide shot while they deliberate.
			const bool bWide = GS->Phase == EAIBPhase::FinalChoice && FMath::Fmod(Elapsed, 10.f) > 6.f;
			Shot = bWide ? Guardian->GetFinalWideCamera(Time) : Guardian->GetChooserCamera(Chooser, Elapsed);
			FOV = bWide ? 50.f : 40.f;
		}
		break;
	case EAIBPhase::EndingCinematic:
	case EAIBPhase::Epilogue:
		if (Guardian)
		{
			bCine = true;
			const bool bKing = GS->Ending == EAIBEnding::KingFirst || GS->Ending == EAIBEnding::SacrificeThenKing || GS->Ending == EAIBEnding::SoloKing;
			Shot = Guardian->GetEndingCamera(bKing, Elapsed, FOV);
		}
		break;
	case EAIBPhase::Results:
		if (Guardian)
		{
			bCine = true;
			Shot = Guardian->GetEndingCamera(true, 18.f + FMath::Sin(Time * 0.05f), FOV);
		}
		break;
	default:
		break;
	}

	if (bCine)
	{
		CineCamera->SetActorTransform(Shot);
		CineCamera->GetCameraComponent()->SetFieldOfView(FOV);
		if (GetViewTarget() != CineCamera)
		{
			SetViewTarget(CineCamera);
		}
	}
	else if (Follow)
	{
		if (GetViewTarget() != Follow)
		{
			SetViewTarget(Follow);
		}
	}
	else if (MyPawn && GetViewTarget() != MyPawn)
	{
		SetViewTarget(MyPawn);
	}
}

void AAIBPlayerController::UpdateSymbols()
{
	const AAIBGameState* GS = GetAIBGameState();
	const bool bSymbolPhase = GS && (GS->Phase == EAIBPhase::Stage2Communication || GS->Phase == EAIBPhase::Stage2Guess);
	const int32 MySlot = GetLocalSlot();
	for (TActorIterator<AAIBCharacter> It(GetWorld()); It; ++It)
	{
		const int32 Slot = It->GetSlot();
		// Never show our own symbol even if something odd arrives.
		const EAIBSymbol Symbol = (bSymbolPhase && Slot != MySlot) ? GetVisibleSymbol(Slot) : EAIBSymbol::None;
		It->ShowLocalSymbol(Symbol);
	}
}

void AAIBPlayerController::UpdateCountdownSounds()
{
	const AAIBGameState* GS = GetAIBGameState();
	if (!GS)
	{
		return;
	}
	const bool bTicking = GS->Phase == EAIBPhase::LobbyCountdown || GS->Phase == EAIBPhase::Stage1Selection || GS->Phase == EAIBPhase::Stage2Communication
		|| GS->Phase == EAIBPhase::Stage2Guess || GS->Phase == EAIBPhase::FinalChoice || GS->Phase == EAIBPhase::Stage3Race
		|| GS->Phase == EAIBPhase::IslandExplore || GS->Phase == EAIBPhase::Stage3Countdown;
	const int32 Second = FMath::CeilToInt(GS->GetPhaseRemaining());
	if (!bTicking || Second == LastTickSecond)
	{
		LastTickSecond = Second;
		return;
	}
	LastTickSecond = Second;
	UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>();
	if (!Audio || Second <= 0)
	{
		return;
	}
	if (GS->Phase == EAIBPhase::Stage3Countdown)
	{
		Audio->PlayCue(TEXT("SFX_Beep_Count"));
	}
	else if (Second <= 5)
	{
		Audio->PlayCue(TEXT("SFX_TickFinal"));
	}
	else if (Second <= 10 && GS->Phase != EAIBPhase::IslandExplore && GS->Phase != EAIBPhase::Stage3Race)
	{
		Audio->PlayCue(TEXT("SFX_Tick"));
	}
}

void AAIBPlayerController::UpdateInputMode()
{
	const bool bWantsCursor = Hud.IsValid() && Hud->WantsCursor();
	if (bInputModeInit && bWantsCursor == bCursorMode)
	{
		return;
	}
	bInputModeInit = true;
	bCursorMode = bWantsCursor;
	if (bWantsCursor)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void AAIBPlayerController::ApplySettings()
{
	UAIBGameInstance* GI = GetGameInstance<UAIBGameInstance>();
	if (!GI)
	{
		return;
	}
	const FAIBSettings& S = GI->GetSettings();
	MouseSensitivity = S.MouseSensitivity;
	bInvertY = S.bInvertY;
	FieldOfView = FMath::Clamp(S.FieldOfView, 60.f, 110.f);
	MicMode = S.MicMode;
	VoiceChatVolume = FMath::Clamp(S.VoiceChatVolume, 0.f, 2.f);
#if !UE_BUILD_SHIPPING
	// Development: -AIBMic=2 forces an open microphone (voice chat testing).
	FParse::Value(FCommandLine::Get(), TEXT("AIBMic="), MicMode);
#endif
	if (AAIBCharacter* MyCharacter = Cast<AAIBCharacter>(GetPawn()))
	{
		if (UCameraComponent* Camera = MyCharacter->GetFollowCamera())
		{
			Camera->SetFieldOfView(FieldOfView);
		}
	}
}

void AAIBPlayerController::UpdateNarration()
{
	const AAIBGameState* GS = GetAIBGameState();
	UAIBAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>() : nullptr;
	if (!GS || !Audio)
	{
		return;
	}
	if (GS->PhaseSerial != NarrationSerial)
	{
		// A new phase: anything Alice was still saying about the previous one stops.
		if (NarrationSerial != -1 && NarrationBeat != INDEX_NONE)
		{
			Audio->StopNarration();
		}
		NarrationSerial = GS->PhaseSerial;
		NarrationBeat = INDEX_NONE;
		if (GS->Phase == EAIBPhase::Results)
		{
			Audio->PlayVoice(TEXT("NAR_Results"));
		}
		// Load the coming narration (voice and pictures) before it is needed: a stall mid-story would put Alice out
		// of step with her captions. The story is loaded while the room counts down.
		const EAIBNarration Coming = GS->Phase == EAIBPhase::LobbyCountdown ? EAIBNarration::Story
			: GS->Phase == EAIBPhase::EndingCinematic ? AIBNarration::EpilogueFor(GS->Ending) : AIBNarration::ForPhase(GS->Phase, GS->Ending);
		TArray<FName> ComingLines;
		for (const AIBNarration::FBeat& Beat : AIBNarration::Get(Coming).Beats)
		{
			ComingLines.Add(Beat.Line);
			AIBStyle::StoryImage(Beat.Image);
		}
		Audio->PreloadVoices(ComingLines);
	}
	// Alice greets each champion once, when they first arrive at the gathering.
	if (!bLobbyWelcomed && GS->Phase == EAIBPhase::Lobby && !GS->bAITestMode && GetLocalSlot() != INDEX_NONE && GS->GetPhaseElapsed() > 1.5f)
	{
		bLobbyWelcomed = true;
		Audio->PlayVoice(TEXT("NAR_Lobby_Welcome"));
	}
	const EAIBNarration Which = AIBNarration::ForPhase(GS->Phase, GS->Ending);
	// The sudden-death race gets a short countdown banner, not the whole briefing again.
	if (Which == EAIBNarration::None || (GS->Phase == EAIBPhase::Stage3Briefing && GS->bRaceSuddenDeath))
	{
		return;
	}
	const float Elapsed = GS->GetPhaseElapsed();
	const int32 Beat = AIBNarration::BeatAt(Which, Elapsed);
	if (Beat == INDEX_NONE || Beat == NarrationBeat)
	{
		return;
	}
	NarrationBeat = Beat;
	const AIBNarration::FSequence& Sequence = AIBNarration::Get(Which);
	// Start the line where the clock says it should be, so the voice matches the caption even after a stall. Someone
	// joining near the end of a line waits for the next one.
	const AIBNarration::FLine* Line = AIBNarration::FindLine(Sequence.Beats[Beat].Line);
	const float Offset = Elapsed - Sequence.Starts[Beat];
	if (Offset < (Line ? Line->Duration * 0.6f : 1.f))
	{
		Audio->PlayVoice(Sequence.Beats[Beat].Line, Offset > 0.15f ? Offset : 0.f);
	}
}

void AAIBPlayerController::UpdateVoiceChat()
{
	// Champions who joined by room code have no online session, so the engine never registers their microphone:
	// register it here (once). Registering opens the microphone, so restore what the settings ask for afterwards.
	if (!bVoiceRegistered)
	{
		const ULocalPlayer* LP = GetLocalPlayer();
		IOnlineVoicePtr VoiceInterface = LP ? Online::GetVoiceInterface(GetWorld()) : nullptr;
		if (VoiceInterface.IsValid())
		{
			bVoiceRegistered = true;
			VoiceInterface->RegisterLocalTalker(LP->GetControllerId());
			ToggleSpeaking(bSpeaking);
		}
	}
	const bool bWant = MicMode == 2 || (MicMode == 1 && bTalkHeld);
	if (bWant != bSpeaking)
	{
		bSpeaking = bWant;
		ToggleSpeaking(bWant);
		ServerSetTalking(bWant);
	}
	// Other champions' voices play through VoIP synth components that come and go with the talkers.
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (Now >= NextVoipVolumeTime)
	{
		NextVoipVolumeTime = Now + 0.5f;
		for (TObjectIterator<UVoipListenerSynthComponent> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld())
			{
				It->SetVolumeMultiplier(VoiceChatVolume);
			}
		}
	}
}

void AAIBPlayerController::UpdateDraftNumber()
{
	const AAIBGameState* GS = GetAIBGameState();
	const AAIBPlayerState* PS = GetAIBPlayerState();
	if (!GS || !PS || GS->Phase != EAIBPhase::Stage1Selection || PS->bSubmitted)
	{
		return;
	}
	if (DraftPhaseSerial != GS->PhaseSerial)
	{
		// Only a number the champion actually dialled counts, not the dial's resting value.
		DraftPhaseSerial = GS->PhaseSerial;
		DraftBaseline = DraftNumber;
		DraftSent = DraftNumber;
	}
	const float Now = GetWorld()->GetRealTimeSeconds();
	if (DraftNumber != DraftSent && Now >= NextDraftSendTime)
	{
		DraftSent = DraftNumber;
		NextDraftSendTime = Now + 0.25f;
		ServerSetDraftNumber(DraftNumber);
	}
}

void AAIBPlayerController::UpdateLore()
{
	NearLore = INDEX_NONE;
	const AAIBGameState* GS = GetAIBGameState();
	const APawn* MyPawn = GetPawn();
	const UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>();
	const AAIBLoreStones* Stones = WorldSys ? WorldSys->GetLoreStones() : nullptr;
	if (GS && GS->Phase == EAIBPhase::Lobby)
	{
		LocalLoreFound = 0;
	}
	if (!GS || GS->Phase != EAIBPhase::IslandExplore || !MyPawn || !Stones)
	{
		return;
	}
	const int32 Found = GetLoreFound();
	for (int32 Index = 0; Index < AAIBLoreStones::Count; ++Index)
	{
		if (!(Found & (1 << Index)) && FVector::Dist2D(MyPawn->GetActorLocation(), Stones->GetStoneLocation(Index)) < AAIBLoreStones::ReadRadius)
		{
			NearLore = Index;
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// RPCs
// ---------------------------------------------------------------------------------------------------------------------

static AAIBGameMode* AIBMode(const AActor* Context)
{
	return Context->GetWorld()->GetAuthGameMode<AAIBGameMode>();
}

void AAIBPlayerController::ServerSetReady_Implementation(bool bReady) { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestReady(this, bReady); }
void AAIBPlayerController::ServerAddBot_Implementation() { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestAddBot(this); }
void AAIBPlayerController::ServerFillBots_Implementation() { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestFillBots(this); }
void AAIBPlayerController::ServerRemoveSeat_Implementation(int32 Slot) { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestRemoveSeat(this, Slot); }
void AAIBPlayerController::ServerStartMatch_Implementation() { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestStart(this); }
void AAIBPlayerController::ServerSetAITestMode_Implementation(bool bOn) { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestSetAITestMode(this, bOn); }
void AAIBPlayerController::ServerDiscoverLore_Implementation(int32 Index) { if (AAIBGameMode* GM = AIBMode(this)) GM->DiscoverLore(this, Index); }
void AAIBPlayerController::ServerSetTalking_Implementation(bool bTalking) { if (AAIBGameMode* GM = AIBMode(this)) GM->SetTalking(this, bTalking); }
void AAIBPlayerController::ServerLeaving_Implementation() { bLeftForGood = true; }
void AAIBPlayerController::ServerSetDraftNumber_Implementation(int32 Number) { if (AAIBGameMode* GM = AIBMode(this)) GM->SetDraftNumber(this, Number); }
void AAIBPlayerController::ServerSkipCinematic_Implementation() { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestSkipCinematic(this); }
void AAIBPlayerController::ServerReturnToLobby_Implementation() { if (AAIBGameMode* GM = AIBMode(this)) GM->RequestReturnToLobby(this); }
void AAIBPlayerController::ServerSubmitNumber_Implementation(int32 Number) { if (AAIBGameMode* GM = AIBMode(this)) GM->SubmitNumber(this, Number); }
void AAIBPlayerController::ServerSubmitGuess_Implementation(EAIBSymbol Guess) { if (AAIBGameMode* GM = AIBMode(this)) GM->SubmitGuess(this, Guess); }
void AAIBPlayerController::ServerSendChat_Implementation(const FString& Text, int32 ToSlot, EAIBSymbol Claim) { if (AAIBGameMode* GM = AIBMode(this)) GM->SubmitChat(this, Text, ToSlot, Claim); }
void AAIBPlayerController::ServerFinalSelect_Implementation(EAIBFinalChoice Choice) { if (AAIBGameMode* GM = AIBMode(this)) GM->SubmitFinalSelect(this, Choice); }
void AAIBPlayerController::ServerFinalConfirm_Implementation(bool bConfirm) { if (AAIBGameMode* GM = AIBMode(this)) GM->SubmitFinalConfirm(this, bConfirm); }

void AAIBPlayerController::ClientReceiveChat_Implementation(const FAIBChatMessage& Message)
{
	if (!Message.bSystem && Message.FromSlot != INDEX_NONE)
	{
		const AAIBGameState* GS = GetAIBGameState();
		if (GS && IsMutedLocally(GS->GetPlayerBySlot(Message.FromSlot)))
		{
			return;
		}
	}
	ChatLog.Add(Message);
	if (ChatLog.Num() > 80)
	{
		ChatLog.RemoveAt(0, ChatLog.Num() - 80);
	}
	if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
	{
		Audio->PlayCue(TEXT("SFX_ChatPop"));
	}
	if (Hud.IsValid())
	{
		Hud->OnChatMessage(Message);
	}
}

void AAIBPlayerController::ClientReceiveSymbols_Implementation(const TArray<FAIBSymbolView>& Views)
{
	VisibleSymbols.Reset();
	const int32 MySlot = GetLocalSlot();
	for (const FAIBSymbolView& View : Views)
	{
		if (View.Slot != MySlot)
		{
			VisibleSymbols.Add(View.Slot, View.Symbol);
		}
	}
}

void AAIBPlayerController::ClientNotify_Implementation(const FText& Message, FName CueId)
{
	Toasts.Add({ Message, (float)GetWorld()->GetTimeSeconds() });
	if (!CueId.IsNone())
	{
		if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
		{
			Audio->PlayCue(CueId);
		}
	}
}

void AAIBPlayerController::ClientRecordMatch_Implementation(const FAIBMatchRecord& Record)
{
	if (UAIBGameInstance* GI = GetGameInstance<UAIBGameInstance>())
	{
		FAIBMatchRecord Local = Record;
		const AAIBPlayerState* PS = GetAIBPlayerState();
		for (FAIBMatchPlayerRecord& Entry : Local.Players)
		{
			Entry.bLocal = PS && Entry.Community == PS->Slot && Entry.Name == PS->GetPlayerName();
		}
		GI->RecordMatch(Local);
	}
}

void AAIBPlayerController::ClientFaceRotation_Implementation(FRotator Rotation)
{
	SetControlRotation(Rotation);
}

#undef LOCTEXT_NAMESPACE
