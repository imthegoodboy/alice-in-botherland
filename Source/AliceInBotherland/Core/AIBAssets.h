// Alice in Botherland - runtime asset lookup with graceful fallbacks

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;
class UObject;

namespace AIBAssets
{
	/** Engine primitive fallbacks. */
	ALICEINBOTHERLAND_API UStaticMesh* Cube();
	ALICEINBOTHERLAND_API UStaticMesh* Cylinder();
	ALICEINBOTHERLAND_API UStaticMesh* Sphere();
	ALICEINBOTHERLAND_API UStaticMesh* Plane();
	ALICEINBOTHERLAND_API UStaticMesh* Cone();

	/** Loads "/Game/Path/Asset" (object name appended automatically). Returns nullptr when missing (no error spam). */
	ALICEINBOTHERLAND_API UObject* Load(const FString& PackagePath);
	ALICEINBOTHERLAND_API UStaticMesh* Mesh(const FString& PackagePath, UStaticMesh* Fallback = nullptr);
	ALICEINBOTHERLAND_API UMaterialInterface* Material(const FString& PackagePath, UMaterialInterface* Fallback = nullptr);
	ALICEINBOTHERLAND_API USoundBase* Sound(const FString& PackagePath);

	/** Default lit material with a "Color" parameter (engine BasicShapeMaterial). */
	ALICEINBOTHERLAND_API UMaterialInterface* BasicMaterial();

	/** Creates a dynamic emissive material (game master material if imported, else engine fallback). */
	ALICEINBOTHERLAND_API UMaterialInstanceDynamic* MakeEmissive(UObject* Outer, const FLinearColor& Color, float Intensity);
	ALICEINBOTHERLAND_API UMaterialInstanceDynamic* MakeHologram(UObject* Outer, const FLinearColor& Color, float Opacity);
	ALICEINBOTHERLAND_API UMaterialInstanceDynamic* MakeBeam(UObject* Outer, const FLinearColor& Color, float Intensity);
	ALICEINBOTHERLAND_API UMaterialInstanceDynamic* MakeTinted(UObject* Outer, const FString& MaterialPath, const FLinearColor& Tint);

	/** Sets a colour on whichever colour parameter the material exposes (Color, Tint, BaseColor). */
	ALICEINBOTHERLAND_API void SetColor(UMaterialInstanceDynamic* MID, const FLinearColor& Color);

	/** Game material library paths (created by Tools/Editor scripts). */
	namespace Paths
	{
		inline const FString Materials = TEXT("/Game/AliceInBotherland/Materials/");
		inline const FString Env = TEXT("/Game/AliceInBotherland/Environment/");
		inline const FString VFX = TEXT("/Game/AliceInBotherland/VFX/");
		inline const FString Audio = TEXT("/Game/AliceInBotherland/Audio/");
	}
}
