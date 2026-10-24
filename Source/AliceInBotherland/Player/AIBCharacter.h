// Alice in Botherland - champion character

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/AIBTypes.h"
#include "Player/AIBPoses.h"
#include "AIBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class AAIBPlayerState;
class UPoseableMeshComponent;

/** The story pose a champion is in (replicated; StartTime is server time so everyone sees it in step). */
USTRUCT()
struct FAIBPosePlay
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Pose = 0;

	UPROPERTY()
	float StartTime = 0.f;

	UPROPERTY()
	uint8 Serial = 0;
};

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

	/** Out of the trials without a scene (left the island): the body drops where it stands. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	/** The Guardian's judgement: a red beam from the sky strikes the champion and they collapse. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJudgement();

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

	/** Server: puts the champion into a story pose for everyone to see (held until replaced or stopped). */
	void PlayPose(EAIBPose Pose);
	void StopPose();
	EAIBPose GetPose() const { return static_cast<EAIBPose>(PoseState.Pose); }

	static constexpr float WalkSpeed = 380.f;
	static constexpr float RunSpeed = 640.f;

protected:
	void ApplyCommunityLook();

	UFUNCTION()
	void OnRep_Sprinting();

	UFUNCTION()
	void OnRep_HiddenForRace();

	UFUNCTION()
	void OnRep_Pose();

	/** Blends the pose in and out on a pose copy of the animated mesh (every machine, every frame). */
	void UpdatePose(float DeltaSeconds);
	/** Drops the pose layer at once (ragdoll, sacrifice light, restore). */
	void ClearPoseVisual();
	/** The body goes limp and falls (ragdoll), pushed by Impulse. */
	void Collapse(const FVector& Impulse);
	/** Beam, impact and light of the judgement over time. */
	void TickJudgement(float T);
	float ServerNow() const;

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

	UPROPERTY(ReplicatedUsing = OnRep_Pose)
	FAIBPosePlay PoseState;

	/** Copy of the body that story poses are applied to (shown instead of the animated mesh while one plays). */
	UPROPERTY()
	TObjectPtr<UPoseableMeshComponent> PoseMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SymbolMID;


	/** Transient effect pieces spawned by the FX multicasts. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> EffectComponents;

	/** The judgement's pieces: outer beam, bright core, ground ring, light. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> JudgeBeam;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> JudgeCore;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> JudgeRing;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> JudgeLight;
	bool bJudgeCollapsed = false;

	EAIBSymbol LocalSymbol = EAIBSymbol::None;
	int32 AppliedSlot = INDEX_NONE;
	float EffectStartTime = -1.f;
	enum class EEffect : uint8 { None, Eliminated, Judged, Sacrifice, Crown } ActiveEffect = EEffect::None;

	EAIBPose ShownPose = EAIBPose::None;
	float ShownPoseStart = 0.f;
	float PoseWeight = 0.f;
	bool bPoseHidMesh = false;
};
