// Alice in Botherland - time of day, fog and colour grading per scene zone

#pragma once

#include "CoreMinimal.h"
#include "World/AIBSetPiece.h"
#include "AIBEnvironmentDirector.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyLight;
class APostProcessVolume;

USTRUCT()
struct FAIBLook
{
	GENERATED_BODY()

	/** Sun elevation above the horizon (deg) and compass yaw the light travels towards. */
	float SunElevation = 30.f;
	float SunYaw = -40.f;
	float SunIntensity = 8.f;
	FLinearColor SunColor = FLinearColor(1.f, 0.93f, 0.85f);
	float FogDensity = 0.02f;
	FLinearColor FogColor = FLinearColor(0.45f, 0.5f, 0.55f);
	float Exposure = 0.f;
	float Saturation = 1.f;
	float Contrast = 1.05f;
	FLinearColor Tint = FLinearColor::White;
	float Vignette = 0.45f;
	float Bloom = 0.8f;
	float Grain = 0.15f;
	float SkyLight = 1.f;
};

UCLASS()
class ALICEINBOTHERLAND_API AAIBEnvironmentDirector : public AAIBSetPiece
{
	GENERATED_BODY()

public:
	AAIBEnvironmentDirector();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnZoneChanged(EAIBSceneZone NewZone) override;

	/** Optional override (e.g. ending cinematics push the sunset further). */
	void SetOverrideLook(const FAIBLook* Look);

	static FAIBLook LookForZone(EAIBSceneZone Zone);

protected:
	virtual void Build() override;

	void ApplyLook(const FAIBLook& Look);

	TWeakObjectPtr<ADirectionalLight> Sun;
	TWeakObjectPtr<AExponentialHeightFog> Fog;
	TWeakObjectPtr<ASkyLight> Sky;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> Grade;

	FAIBLook Current;
	FAIBLook Target;
	bool bHasOverride = false;
	FAIBLook Override;
	bool bSnapNext = true;
};
