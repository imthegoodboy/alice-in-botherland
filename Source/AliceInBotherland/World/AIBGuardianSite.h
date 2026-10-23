// Alice in Botherland - the Guardian: race finish, final chamber and ending stage

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBGuardianSite.generated.h"

class UPointLightComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class ALICEINBOTHERLAND_API AAIBGuardianSite : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBGuardianSite();

	virtual void Tick(float DeltaSeconds) override;

	FVector GetSwitchLocation() const;
	static constexpr float SwitchRadius = 750.f;

	/** Where finalist Index (0 = first chooser) stands in the final chamber. */
	FTransform GetFinalStand(int32 Index) const;
	FTransform GetThroneSeat() const;
	/** Wide shot of both finalists before the statue. */
	FTransform GetFinalWideCamera(float Time) const;
	/** Close shot on a finalist. */
	FTransform GetChooserCamera(int32 Index, float Time) const;
	/** Ending cinematic camera by ending type and time. */
	FTransform GetEndingCamera(bool bKing, float Time, float& OutFOV) const;
	/** Race finish view looking down the road. */
	FTransform GetArrivalCamera(float Time) const;

protected:
	virtual void Build() override;

	FVector Local(const FVector& V) const { return GetActorTransform().TransformPosition(V); }

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BeamMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SwitchGlow;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeGlow;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Braziers;
	UPROPERTY() TObjectPtr<UPointLightComponent> SwitchLight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam;

	float StatueHeight = 1600.f;
	float AltarTop = 90.f;
};
