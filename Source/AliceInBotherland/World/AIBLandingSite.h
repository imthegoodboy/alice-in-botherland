// Alice in Botherland - pier and beach where the champions come ashore

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBLandingSite.generated.h"

UCLASS()
class ALICEINBOTHERLAND_API AAIBLandingSite : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	/** Where champion Slot stands after disembarking (ground snapped, facing inland). */
	FTransform GetDisembarkSpawn(int32 Slot) const;
	/** Safe point to put a champion back on land (acid / out of bounds). */
	FTransform GetRespawn(int32 Slot) const { return GetDisembarkSpawn(Slot); }

protected:
	virtual void Build() override;

	FVector PierDeckStart = FVector::ZeroVector;
	float DeckZ = 0.f;
};
