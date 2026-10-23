// Alice in Botherland - Stage 3 race route from the beach to the Guardian

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBRaceCourse.generated.h"

UCLASS()
class ALICEINBOTHERLAND_API AAIBRaceCourse : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	/** Start transform for finalist 0/1. Sudden death starts further along the route. */
	FTransform GetStart(int32 FinalistIndex, bool bSuddenDeath) const;
	/** Ground-snapped route points from the start line to the Guardian switch. */
	const TArray<FVector>& GetPath() const { return Path; }
	/** Index of the first path point to follow for a sudden-death start. */
	int32 GetSuddenDeathStartIndex() const { return SuddenDeathIndex; }
	/** Remaining route length in metres from Location to the end. */
	float RemainingDistance(const FVector& Location) const;
	/** Nearest path index at or after a hint. */
	int32 FindNearestIndex(const FVector& Location, int32 Hint = 0) const;
	/** A safe reset transform near the route for a stuck / drowned vehicle. */
	FTransform GetResetTransform(const FVector& Location) const;

protected:
	virtual void Build() override;

	void BuildPath();

	TArray<FVector> Path;
	TArray<float> CumulativeLength;
	int32 SuddenDeathIndex = 0;
};
