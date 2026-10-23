// Alice in Botherland - main menu mode (same island map, no pawns)

#include "Game/AIBMenuGameMode.h"
#include "Game/AIBGameState.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBFleet.h"
#include "World/AIBGuardianSite.h"
#include "Audio/AIBAudioSubsystem.h"
#include "UI/SAIBMainMenu.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/SpectatorPawn.h"

AAIBMenuGameMode::AAIBMenuGameMode()
{
	GameStateClass = AAIBGameState::StaticClass();
	PlayerControllerClass = AAIBMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	SpectatorClass = ASpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = false;
}

AAIBMenuPlayerController::AAIBMenuPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = true;
}

void AAIBMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	MenuCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	if (MenuCamera)
	{
		MenuCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		MenuCamera->GetCameraComponent()->SetFieldOfView(55.f);
		SetViewTarget(MenuCamera);
	}

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Menu = SNew(SAIBMainMenu).Owner(this);
		Viewport->AddViewportWidgetForPlayer(GetLocalPlayer(), Menu.ToSharedRef(), 10);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Menu);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	if (UAIBAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UAIBAudioSubsystem>())
	{
		Audio->PlayMusic(TEXT("MUS_Menu"));
		Audio->SetAmbienceForZone(EAIBSceneZone::Sea);
	}
	if (UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>())
	{
		WorldSys->SetLocalZone(EAIBSceneZone::Sea);
	}
}

void AAIBMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Menu.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetForPlayer(GetLocalPlayer(), Menu.ToSharedRef());
		}
		Menu.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AAIBMenuPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>();
	if (!MenuCamera || !WorldSys)
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	FTransform Shot = MenuCamera->GetActorTransform();
	if (AAIBFleet* Fleet = WorldSys->GetFleet())
	{
		Shot = Fleet->GetLobbyCamera(Time);
	}
	MenuCamera->SetActorTransform(Shot);
	if (GetViewTarget() != MenuCamera)
	{
		SetViewTarget(MenuCamera);
	}
}
