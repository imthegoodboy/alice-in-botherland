// Alice in Botherland - life on the island: gulls, tumbleweeds, roadside wrecks, signposts and campfire sparks

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBIslandLife.generated.h"

class UStaticMeshComponent;

/**
 * Small things that make the island feel inhabited (and abandoned): gulls circling the cove, the town and the
 * Guardian's beam, tumbleweeds rolling across the road, a dead power line, wrecked cars and an old outpost along
 * the route, signposts, a beached wreck and sparks over the signal fire. Purely decorative, built locally and only
 * shown on the island. Everything stays well clear of the race road (the course places its own obstacles).
 */
UCLASS()
class ALICEINBOTHERLAND_API AAIBIslandLife : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBIslandLife();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void Build() override;

	void BuildGulls();
	void BuildTumbleweeds();
	void BuildRoadside();
	void BuildSignposts();
	void BuildBeach();

	/** Point beside the race road: Index along AIBRacePath, Offset cm to the left (+) or right (-). */
	FVector RoadSide(int32 Index, float Offset) const;
	/** Unit direction of the road at a path point. */
	FVector2D RoadDir(int32 Index) const;

	struct FGullFlight
	{
		FVector Center = FVector::ZeroVector;
		float Radius = 2000.f;
		float Speed = 900.f;
		float Phase = 0.f;
		float FlapRate = 2.5f;
		float Bob = 150.f;
		bool bClockwise = false;
	};
	TArray<FGullFlight> GullFlights;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> GullBodies;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> GullWingsL;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> GullWingsR;

	struct FTumble
	{
		FVector2D Start = FVector2D::ZeroVector;
		float Travel = 0.f;
		float Speed = 300.f;
		float Radius = 40.f;
		float Distance = 6000.f;
	};
	TArray<FTumble> Tumbles;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TumbleMeshes;
	FVector2D WindDir = FVector2D(1.f, 0.f);

	struct FSpark
	{
		FVector Drift = FVector::ZeroVector;
		float Born = 0.f;
		float Life = 1.5f;
		float Size = 0.04f;
	};
	TArray<FSpark> Sparks;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SparkMeshes;
	FVector FireLocation = FVector::ZeroVector;
};
