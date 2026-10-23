// Alice in Botherland - time of day, fog and colour grading per scene zone

#include "World/AIBEnvironmentDirector.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AAIBEnvironmentDirector::AAIBEnvironmentDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

FAIBLook AAIBEnvironmentDirector::LookForZone(EAIBSceneZone Zone)
{
	FAIBLook L;
	switch (Zone)
	{
	case EAIBSceneZone::Lobby:
	case EAIBSceneZone::Sea:
		// Late golden hour at sea: copper sun behind the fleet lighting the island's face, a light green haze over
		// the acid water. (A sun much lower than this streaks the water with island-long shadows.)
		L.SunElevation = 16.f; L.SunYaw = 55.f; L.SunIntensity = 7.f;
		L.SunColor = FLinearColor(1.f, 0.62f, 0.36f);
		L.FogDensity = 0.016f; L.FogColor = FLinearColor(0.4f, 0.43f, 0.35f);
		L.Saturation = 0.92f; L.Contrast = 1.12f; L.Tint = FLinearColor(1.f, 0.97f, 0.92f);
		L.Vignette = 0.55f; L.Bloom = 1.2f; L.Exposure = 0.3f;
		break;
	case EAIBSceneZone::Island:
		L.SunElevation = 24.f; L.SunYaw = 40.f; L.SunIntensity = 9.f;
		L.SunColor = FLinearColor(1.f, 0.85f, 0.7f);
		L.FogDensity = 0.022f; L.FogColor = FLinearColor(0.55f, 0.5f, 0.42f);
		L.Saturation = 0.95f; L.Contrast = 1.08f;
		break;
	case EAIBSceneZone::NumberRoom:
		L.SunElevation = 40.f; L.SunYaw = 0.f; L.SunIntensity = 2.f;
		L.FogDensity = 0.01f; L.FogColor = FLinearColor(0.05f, 0.05f, 0.06f);
		L.Saturation = 0.85f; L.Contrast = 1.2f; L.Tint = FLinearColor(0.95f, 0.97f, 1.f);
		L.Vignette = 0.75f; L.Bloom = 1.4f; L.Grain = 0.3f; L.SkyLight = 0.15f; L.Exposure = 0.6f;
		break;
	case EAIBSceneZone::Hotel:
		// Low sun pouring through the north windows (light travels towards -Y).
		L.SunElevation = 26.f; L.SunYaw = -90.f; L.SunIntensity = 10.f;
		L.SunColor = FLinearColor(1.f, 0.78f, 0.55f);
		L.FogDensity = 0.03f; L.FogColor = FLinearColor(0.5f, 0.45f, 0.38f);
		L.Saturation = 0.8f; L.Contrast = 1.1f; L.Tint = FLinearColor(1.f, 0.93f, 0.82f);
		L.Vignette = 0.6f; L.Bloom = 1.1f; L.Grain = 0.25f; L.SkyLight = 0.5f; L.Exposure = 0.5f;
		break;
	case EAIBSceneZone::Race:
		L.SunElevation = 38.f; L.SunYaw = 20.f; L.SunIntensity = 10.f;
		L.SunColor = FLinearColor(1.f, 0.9f, 0.78f);
		L.FogDensity = 0.018f; L.FogColor = FLinearColor(0.6f, 0.52f, 0.42f);
		L.Saturation = 1.05f; L.Contrast = 1.1f; L.Vignette = 0.35f;
		break;
	case EAIBSceneZone::Guardian:
		// Blood-red sunset behind the statue.
		L.SunElevation = 4.f; L.SunYaw = -135.f; L.SunIntensity = 7.f;
		L.SunColor = FLinearColor(1.f, 0.4f, 0.2f);
		L.FogDensity = 0.04f; L.FogColor = FLinearColor(0.45f, 0.3f, 0.35f);
		L.Saturation = 1.0f; L.Contrast = 1.15f; L.Tint = FLinearColor(1.f, 0.92f, 0.9f);
		L.Vignette = 0.6f; L.Bloom = 1.5f; L.Exposure = 0.4f;
		break;
	case EAIBSceneZone::Results:
	default:
		L.SunElevation = 3.f; L.SunYaw = 80.f; L.SunIntensity = 5.f;
		L.SunColor = FLinearColor(1.f, 0.5f, 0.35f);
		L.FogDensity = 0.04f; L.FogColor = FLinearColor(0.32f, 0.3f, 0.4f);
		L.Saturation = 0.9f; L.Vignette = 0.65f; L.Bloom = 1.3f; L.Exposure = 0.3f;
		break;
	}
	return L;
}

void AAIBEnvironmentDirector::Build()
{
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		Sun = *It;
		break;
	}
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		Fog = *It;
		break;
	}
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		Sky = *It;
		break;
	}
	if (Sun.IsValid())
	{
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	}
	if (Fog.IsValid())
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetVolumetricFog(true);
		FogComp->SetVolumetricFogScatteringDistribution(0.6f);
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Grade = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, Params);
	if (Grade)
	{
		Grade->bUnbound = true;
		Grade->Priority = 50.f;
		Grade->BlendWeight = 1.f;
	}

	Current = LookForZone(EAIBSceneZone::Lobby);
	Target = Current;
	ApplyLook(Current);
}

void AAIBEnvironmentDirector::OnZoneChanged(EAIBSceneZone NewZone)
{
	Target = LookForZone(NewZone);
}

void AAIBEnvironmentDirector::SetOverrideLook(const FAIBLook* Look)
{
	bHasOverride = Look != nullptr;
	if (Look)
	{
		Override = *Look;
	}
}

void AAIBEnvironmentDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FAIBLook& Goal = bHasOverride ? Override : Target;
	const float A = bSnapNext ? 1.f : FMath::Clamp(DeltaSeconds * 1.2f, 0.f, 1.f);
	bSnapNext = false;

	auto Lerp = [A](float& Value, float To) { Value = FMath::Lerp(Value, To, A); };
	auto LerpC = [A](FLinearColor& Value, const FLinearColor& To) { Value = FMath::Lerp(Value, To, A); };
	Lerp(Current.SunElevation, Goal.SunElevation);
	Current.SunYaw = FMath::Lerp(Current.SunYaw, Current.SunYaw + FMath::FindDeltaAngleDegrees(Current.SunYaw, Goal.SunYaw), A);
	Lerp(Current.SunIntensity, Goal.SunIntensity);
	LerpC(Current.SunColor, Goal.SunColor);
	Lerp(Current.FogDensity, Goal.FogDensity);
	LerpC(Current.FogColor, Goal.FogColor);
	Lerp(Current.Exposure, Goal.Exposure);
	Lerp(Current.Saturation, Goal.Saturation);
	Lerp(Current.Contrast, Goal.Contrast);
	LerpC(Current.Tint, Goal.Tint);
	Lerp(Current.Vignette, Goal.Vignette);
	Lerp(Current.Bloom, Goal.Bloom);
	Lerp(Current.Grain, Goal.Grain);
	Lerp(Current.SkyLight, Goal.SkyLight);
	ApplyLook(Current);
}

void AAIBEnvironmentDirector::ApplyLook(const FAIBLook& Look)
{
	if (ADirectionalLight* SunActor = Sun.Get())
	{
		SunActor->SetActorRotation(FRotator(-Look.SunElevation, Look.SunYaw, 0.f));
		if (UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(SunActor->GetLightComponent()))
		{
			Comp->SetIntensity(Look.SunIntensity);
			Comp->SetLightColor(Look.SunColor);
		}
	}
	if (AExponentialHeightFog* FogActor = Fog.Get())
	{
		FogActor->GetComponent()->SetFogDensity(Look.FogDensity);
		FogActor->GetComponent()->SetFogInscatteringColor(Look.FogColor);
	}
	if (ASkyLight* SkyActor = Sky.Get())
	{
		SkyActor->GetLightComponent()->SetIntensity(Look.SkyLight);
	}
	if (Grade)
	{
		FPostProcessSettings& S = Grade->Settings;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = Look.Exposure;
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(Look.Saturation, Look.Saturation, Look.Saturation, 1.f);
		S.bOverride_ColorContrast = true;
		S.ColorContrast = FVector4(Look.Contrast, Look.Contrast, Look.Contrast, 1.f);
		S.bOverride_ColorGain = true;
		S.ColorGain = FVector4(Look.Tint.R, Look.Tint.G, Look.Tint.B, 1.f);
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = Look.Vignette;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = Look.Bloom;
		S.bOverride_FilmGrainIntensity = true;
		S.FilmGrainIntensity = Look.Grain;
		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = 0.35f;
	}
}
