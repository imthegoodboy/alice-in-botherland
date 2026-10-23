// Alice in Botherland - AI champion (fills empty seats)

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIBBotController.generated.h"

class AAIBVehicle;
class AAIBCharacter;

UCLASS()
class ALICEINBOTHERLAND_API AAIBBotController : public AAIController
{
	GENERATED_BODY()

public:
	AAIBBotController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	/** Restricts wandering (island explore / hotel). Empty = stand still. */
	void SetWanderPoints(const TArray<FVector>& Points, const FBox& Bounds);
	void ClearWander();

	/** Skill 0..1 (driving accuracy, top speed). */
	float Skill = 0.7f;

protected:
	void TickWander(AAIBCharacter* Champion, float DeltaSeconds);
	void TickDrive(AAIBVehicle* Vehicle, float DeltaSeconds);

	TArray<FVector> WanderPoints;
	FBox WanderBounds;
	FVector WanderTarget = FVector::ZeroVector;
	float WanderPause = 0.f;
	bool bHasTarget = false;

	int32 PathIndex = 0;
	float StuckTimer = 0.f;
	float ReverseTimer = 0.f;
	float ReverseSteer = 0.f;
};
