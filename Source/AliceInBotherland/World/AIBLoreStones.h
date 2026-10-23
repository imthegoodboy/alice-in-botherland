// Alice in Botherland - marker stones to discover while exploring the island

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBLoreStones.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * Five old marker stones between the beach and the town. Walking up to one and pressing E plays Alice's line about
 * the island's past. Each stone sends up a soft pillar of light (seen from afar) until the local player has read it.
 * Purely decorative (no collision): the race route passes close by later.
 */
UCLASS()
class ALICEINBOTHERLAND_API AAIBLoreStones : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBLoreStones();

	virtual void Tick(float DeltaSeconds) override;

	static constexpr int32 Count = 5;
	/** How close (cm, horizontal) a champion must be to read a stone. */
	static constexpr float ReadRadius = 450.f;

	FVector GetStoneLocation(int32 Index) const { return Stones.IsValidIndex(Index) ? Stones[Index] : FVector::ZeroVector; }
	/** Narrator line for a stone. */
	static FName LineFor(int32 Index);

protected:
	virtual void Build() override;

	TArray<FVector> Stones;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Pillars;

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> Lights;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RuneGlow;
};
