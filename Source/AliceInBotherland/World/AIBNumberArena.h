// Alice in Botherland - Stage 1 arena: the Room of Numbers

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBNumberArena.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ALICEINBOTHERLAND_API AAIBNumberArena : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBNumberArena();

	virtual void Tick(float DeltaSeconds) override;

	/** Where the champion of Slot stands (facing the table). */
	FTransform GetStandTransform(int32 Slot) const;
	/** Over-the-shoulder view from behind Slot's podium. */
	FTransform GetSeatCamera(int32 Slot) const;
	/** Slow orbit above the table. */
	FTransform GetOverviewCamera(float Time) const;
	/** Low close-up on the hologram for the reveal. */
	FTransform GetRevealCamera(float Time) const;

	static constexpr float StandRadius = 345.f;

protected:
	virtual void Build() override;

	FVector SlotDirection(int32 Slot) const;

	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PodiumGlow;
	UPROPERTY() TArray<TObjectPtr<USpotLightComponent>> PodiumLights;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> PodiumNumbers;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> PodiumNames;
	UPROPERTY() TObjectPtr<UTextRenderComponent> HoloMain;
	UPROPERTY() TObjectPtr<UTextRenderComponent> HoloSub;
	UPROPERTY() TObjectPtr<UTextRenderComponent> WallTitle;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> HoloRings;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HoloMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> InlayMaterial;
};
