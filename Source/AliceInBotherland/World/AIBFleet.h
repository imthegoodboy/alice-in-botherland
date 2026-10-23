// Alice in Botherland - the five community ships

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBFleet.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AAIBGameState;

/**
 * Five ships. Their pose is a pure function of the replicated phase clock, so every client sees the same
 * voyage without replicating transforms: offshore in the lobby, sailing in during the arrival cinematic,
 * moored in the cove afterwards, and sailing away for communities that leave in the ending.
 */
UCLASS()
class ALICEINBOTHERLAND_API AAIBFleet : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBFleet();

	virtual void Tick(float DeltaSeconds) override;

	FTransform GetShipTransform(int32 Slot) const;
	FVector GetDockedLocation(int32 Slot) const;
	FVector GetOffshoreLocation(int32 Slot) const;
	/** Camera framing the whole fleet at sea (main menu / lobby). */
	FTransform GetLobbyCamera(float Time) const;
	/** Arrival cinematic camera for a given time (0..ArrivalCinematic). */
	FTransform GetArrivalCamera(float Time, float& OutFOV) const;
	float GetShipLength(int32 Slot) const;

	/** The ship at the head of the single-file column (the camera follows it in). */
	int32 GetLeadShip() const;

protected:
	virtual void Build() override;

	/** One ship's way in: single file up to and through the channel, then out to its mooring (sampled, cm). */
	struct FRoute
	{
		TArray<FVector> Points;
		TArray<float> Distance;
		float Length() const { return Distance.Num() ? Distance.Last() : 0.f; }
	};
	void BuildRoutes();
	/** Point and heading S cm along the route (before the start it continues straight back out to sea). */
	FVector SampleRoute(int32 Slot, float S, FVector& OutDir) const;
	/** How far along its route a ship is for the current phase; bDeparting while it sails back out. */
	float RouteDistance(int32 Slot, const AAIBGameState* GS, bool& bDeparting) const;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> ShipRoots;

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> Lanterns;

	TArray<float> ShipLengths;

	TArray<FRoute> Routes;
	/** Convoy distance covered during the arrival (every ship moves the same distance, so the column keeps its gaps). */
	float ConvoyLength = 0.f;
	/** Departure: per-ship head start so the ships leave last-in first-out and never meet in the channel. */
	TArray<float> DepartDelay;
	float DepartLength = 0.f;
};
