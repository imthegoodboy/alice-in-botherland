// Alice in Botherland - champion character

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/AIBTypes.h"
#include "AIBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class AAIBPlayerState;

UCLASS()
class ALICEINBOTHERLAND_API AAIBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAIBCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	AAIBPlayerState* GetChampionState() const;
	int32 GetSlot() const;

	/** Locks/unlocks player-driven movement (server authoritative, replicated through the movement mode). */
	void SetMovementLocked(bool bLocked);

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	/** Shows a symbol floating above this character on THIS machine only (others' symbols, never your own). */
	void ShowLocalSymbol(EAIBSymbol Symbol);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSacrifice();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastCrown();

	/** Puts the character back into a normal state (after a reset to the lobby). */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRestore();

	/** Hides the character while it's driving (race). Replicated. */
	void SetHiddenForRace(bool bHide);

	bool IsSprinting() const { return bSprinting; }

	static constexpr float WalkSpeed = 380.f;
	static constexpr float RunSpeed = 640.f;

protected:
	void ApplyCommunityLook();

	UFUNCTION()
	void OnRep_Sprinting();

	UFUNCTION()
	void OnRep_HiddenForRace();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Floating suit symbol (client-local visibility). */
	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UStaticMeshComponent> SymbolMesh;

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<UPointLightComponent> SymbolLight;

	UPROPERTY(ReplicatedUsing = OnRep_Sprinting)
	bool bSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_HiddenForRace)
	bool bHiddenForRace = false;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> JacketMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SymbolMID;


	/** Transient effect pieces spawned by the FX multicasts. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> EffectComponents;

	EAIBSymbol LocalSymbol = EAIBSymbol::None;
	int32 AppliedSlot = INDEX_NONE;
	float EffectStartTime = -1.f;
	enum class EEffect : uint8 { None, Eliminated, Sacrifice, Crown } ActiveEffect = EEffect::None;
};
