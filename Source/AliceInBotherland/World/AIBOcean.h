// Alice in Botherland - the acidic ocean and the island's shoreline

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBOcean.generated.h"

class UProceduralMeshComponent;

/**
 * Ocean surface (dense grid near the island so the material can displace waves, coarse out to the horizon)
 * plus a sloped "skirt" around the landscape border so the island meets the sea instead of ending abruptly.
 */
UCLASS()
class ALICEINBOTHERLAND_API AAIBOcean : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBOcean();

	float GetSeaLevel() const { return GetActorLocation().Z; }

protected:
	virtual void Build() override;

	void BuildSurface();
	void BuildSkirt();
	void ScatterShoreRocks(const TArray<FTransform>& ShoreSpots);

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Surface;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Skirt;
};
