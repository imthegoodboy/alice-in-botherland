// Alice in Botherland - Stage 2 arena: the abandoned Grand Meridian Hotel lobby

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBHotelArena.generated.h"

class UPointLightComponent;

UCLASS()
class ALICEINBOTHERLAND_API AAIBHotelArena : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBHotelArena();

	virtual void Tick(float DeltaSeconds) override;

	/** Spawn for the Index-th finalist candidate (0..3) around the lobby centre. */
	FTransform GetSpawn(int32 Index, int32 Count) const;
	/** Points bots wander between while "talking". */
	const TArray<FVector>& GetWanderPoints() const { return WanderPoints; }
	FTransform GetOverviewCamera(float Time) const;
	/** Keeps characters inside the hall. */
	FBox GetWalkBounds() const;

	static constexpr float HalfX = 1800.f;
	static constexpr float HalfY = 1400.f;
	static constexpr float Height = 1000.f;

protected:
	virtual void Build() override;

	void PlaceProp(const FString& Path, const FVector& Location, const FRotator& Rotation, float TargetSize, UMaterialInterface* FallbackMat, const FVector& FallbackSize);

	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> FlickerLights;
	TArray<float> FlickerBase;
	TArray<FVector> WanderPoints;
};
