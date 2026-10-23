// Alice in Botherland - main menu mode (same island map, no pawns)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"
#include "AIBMenuGameMode.generated.h"

class ACameraActor;
class SAIBMainMenu;

UCLASS()
class ALICEINBOTHERLAND_API AAIBMenuGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AAIBMenuGameMode();

	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
};

UCLASS()
class ALICEINBOTHERLAND_API AAIBMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAIBMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Camera path for the prologue (0..1) or idle drift (<0). */
	float PrologueAlpha = -1.f;

protected:
	UPROPERTY()
	TObjectPtr<ACameraActor> MenuCamera;

	TSharedPtr<SAIBMainMenu> Menu;
};
