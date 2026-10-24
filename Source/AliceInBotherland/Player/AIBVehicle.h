// Alice in Botherland - Stage 3 race vehicle (Chaos offroad car)

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "ChaosVehicleWheel.h"
#include "AIBVehicle.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UAudioComponent;
class UPointLightComponent;
class UChaosWheeledVehicleMovementComponent;

UCLASS()
class ALICEINBOTHERLAND_API UAIBWheelFront : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UAIBWheelFront();
};

UCLASS()
class ALICEINBOTHERLAND_API UAIBWheelRear : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UAIBWheelRear();
};

UCLASS()
class ALICEINBOTHERLAND_API AAIBVehicle : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AAIBVehicle();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetDriverSlot(int32 Slot);
	int32 GetDriverSlot() const { return DriverSlot; }

	void ApplyThrottle(float Value);
	void ApplySteering(float Value);
	void ApplyBrake(float Value);
	void ApplyHandbrake(bool bOn);
	void LookAround(float Yaw);

	/** Server: puts the car back on its wheels at a transform (stuck, flipped or in the acid). */
	void ResetTo(const FTransform& Transform);

	/** Requests a reset from the owning client. */
	UFUNCTION(Server, Reliable)
	void ServerRequestReset();

	float GetSpeedKmh() const;
	bool IsUpsideDown() const;

	UChaosWheeledVehicleMovementComponent* GetChaos() const;

	/** Engine inputs locked (countdown / finished). */
	void SetInputLocked(bool bLocked);

protected:
	UFUNCTION()
	void OnRep_DriverSlot();

	/**
	 * The Chaos movement component detaches the wheel bodies from the chassis when it creates its physics state,
	 * but only if the skeletal mesh bodies already exist. With deferred body creation that step is skipped and the
	 * chassis stays welded to the kinematic wheel bodies (the car hangs frozen). Redo it once the bodies exist.
	 */
	bool EnsureWheelBodiesDetached();
	bool bWheelBodiesDetached = false;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> Chassis;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> TireFL;
	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> TireFR;
	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> TireRL;
	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> TireRR;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UAudioComponent> EngineAudio;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UPointLightComponent> UnderGlow;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY(ReplicatedUsing = OnRep_DriverSlot)
	int32 DriverSlot = INDEX_NONE;

	UPROPERTY(Replicated)
	bool bInputLocked = true;

	float StuckTime = 0.f;
	/** Server time of the last requested reset (R): one every few seconds. */
	float LastResetRequest = -100.f;
	float LookYaw = 0.f;
};
