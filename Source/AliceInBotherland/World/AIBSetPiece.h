// Alice in Botherland - base class for self-building set pieces

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/AIBTypes.h"
#include "AIBSetPiece.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;
class UTextRenderComponent;

/**
 * A gameplay set piece (arena, ocean, fleet...). Spawned locally on every machine by UAIBWorldSubsystem
 * (not replicated) and assembled from components in Build(). Set pieces that live away from the island hide
 * themselves when the local camera is in another zone.
 */
UCLASS(Abstract)
class ALICEINBOTHERLAND_API AAIBSetPiece : public AActor
{
	GENERATED_BODY()

public:
	AAIBSetPiece();

	virtual void BeginPlay() override;

	/** Zones in which this set piece must be visible. Empty = always visible. */
	TArray<EAIBSceneZone> VisibleInZones;

	/** Called by the world subsystem whenever the local scene zone changes. */
	virtual void OnZoneChanged(EAIBSceneZone NewZone);

	bool IsBuilt() const { return bBuilt; }

	/** Ground height under a world XY (landscape / static geometry), or Fallback. */
	static float GroundZ(const UWorld* World, const FVector2D& XY, float Fallback = 0.f);

protected:
	virtual void Build() {}

	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, const FTransform& Relative, UMaterialInterface* Material = nullptr, bool bCollision = true, USceneComponent* Parent = nullptr);
	/** Box built from the engine cube (1 m) scaled to Size (cm). */
	UStaticMeshComponent* AddBox(const FVector& Center, const FVector& Size, UMaterialInterface* Material, const FRotator& Rotation = FRotator::ZeroRotator, bool bCollision = true, USceneComponent* Parent = nullptr);
	UPointLightComponent* AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bShadows = false, USceneComponent* Parent = nullptr);
	USpotLightComponent* AddSpotLight(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, float Intensity, float Radius, float OuterAngle, bool bShadows = true, USceneComponent* Parent = nullptr);
	URectLightComponent* AddRectLight(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, float Intensity, float Width, float Height, float Radius, USceneComponent* Parent = nullptr);
	UTextRenderComponent* AddText(const FVector& Location, const FRotator& Rotation, const FText& Text, const FLinearColor& Color, float WorldSize, USceneComponent* Parent = nullptr);

	/** Material for text that glows (game material when available, engine default otherwise). */
	UMaterialInterface* TextMaterial() const;

	USceneComponent* MakeScene(const FName& Name, const FTransform& Relative, USceneComponent* Parent = nullptr);

	UPROPERTY(VisibleAnywhere, Category = "AIB")
	TObjectPtr<USceneComponent> Root;

	int32 ComponentCounter = 0;
	bool bBuilt = false;
};
