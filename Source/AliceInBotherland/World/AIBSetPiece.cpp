// Alice in Botherland - base class for self-building set pieces

#include "World/AIBSetPiece.h"
#include "Core/AIBAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

AAIBSetPiece::AAIBSetPiece()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AAIBSetPiece::BeginPlay()
{
	Super::BeginPlay();
	if (!bBuilt)
	{
		bBuilt = true;
		Build();
	}
}

void AAIBSetPiece::OnZoneChanged(EAIBSceneZone NewZone)
{
	if (VisibleInZones.Num() == 0)
	{
		return;
	}
	const bool bVisible = VisibleInZones.Contains(NewZone);
	SetActorHiddenInGame(!bVisible);
	TInlineComponentArray<USceneComponent*> Components(this);
	for (USceneComponent* Comp : Components)
	{
		if (Comp != Root)
		{
			Comp->SetVisibility(bVisible);
		}
	}
}

float AAIBSetPiece::GroundZ(const UWorld* World, const FVector2D& XY, float Fallback)
{
	if (!World)
	{
		return Fallback;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AIBGround), false);
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	const FVector Start(XY.X, XY.Y, 50000.f);
	const FVector End(XY.X, XY.Y, -20000.f);
	if (World->LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return Fallback;
}

USceneComponent* AAIBSetPiece::MakeScene(const FName& Name, const FTransform& Relative, USceneComponent* Parent)
{
	USceneComponent* Comp = NewObject<USceneComponent>(this, Name);
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetupAttachment(Parent ? Parent : Root.Get());
	Comp->SetRelativeTransform(Relative);
	Comp->RegisterComponent();
	return Comp;
}

UStaticMeshComponent* AAIBSetPiece::AddMesh(UStaticMesh* Mesh, const FTransform& Relative, UMaterialInterface* Material, bool bCollision, USceneComponent* Parent)
{
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Mesh_%d"), ComponentCounter++));
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetupAttachment(Parent ? Parent : Root.Get());
	Comp->SetStaticMesh(Mesh);
	Comp->SetRelativeTransform(Relative);
	if (Material)
	{
		for (int32 Index = 0; Index < Comp->GetNumMaterials(); ++Index)
		{
			Comp->SetMaterial(Index, Material);
		}
	}
	Comp->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Comp->SetCanEverAffectNavigation(false);
	Comp->RegisterComponent();
	return Comp;
}

UStaticMeshComponent* AAIBSetPiece::AddBox(const FVector& Center, const FVector& Size, UMaterialInterface* Material, const FRotator& Rotation, bool bCollision, USceneComponent* Parent)
{
	// Engine cube is 100 cm with the pivot at the centre.
	return AddMesh(AIBAssets::Cube(), FTransform(Rotation, Center, Size / 100.f), Material, bCollision, Parent);
}

UPointLightComponent* AAIBSetPiece::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bShadows, USceneComponent* Parent)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("Point_%d"), ComponentCounter++));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Parent ? Parent : Root.Get());
	Light->SetRelativeLocation(Location);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(Intensity * AIB::LightScale);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	Light->SetCastShadows(bShadows);
	Light->SetSourceRadius(10.f);
	Light->RegisterComponent();
	return Light;
}

USpotLightComponent* AAIBSetPiece::AddSpotLight(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, float Intensity, float Radius, float OuterAngle, bool bShadows, USceneComponent* Parent)
{
	USpotLightComponent* Light = NewObject<USpotLightComponent>(this, *FString::Printf(TEXT("Spot_%d"), ComponentCounter++));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Parent ? Parent : Root.Get());
	Light->SetRelativeLocationAndRotation(Location, Rotation);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(Intensity * AIB::LightScale);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	Light->SetOuterConeAngle(OuterAngle);
	Light->SetInnerConeAngle(OuterAngle * 0.6f);
	Light->SetCastShadows(bShadows);
	Light->SetVolumetricScatteringIntensity(1.5f);
	Light->RegisterComponent();
	return Light;
}

URectLightComponent* AAIBSetPiece::AddRectLight(const FVector& Location, const FRotator& Rotation, const FLinearColor& Color, float Intensity, float Width, float Height, float Radius, USceneComponent* Parent)
{
	URectLightComponent* Light = NewObject<URectLightComponent>(this, *FString::Printf(TEXT("Rect_%d"), ComponentCounter++));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Parent ? Parent : Root.Get());
	Light->SetRelativeLocationAndRotation(Location, Rotation);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(Intensity * AIB::RectLightScale);
	Light->SetLightColor(Color);
	Light->SetSourceWidth(Width);
	Light->SetSourceHeight(Height);
	Light->SetAttenuationRadius(Radius);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	return Light;
}

UMaterialInterface* AAIBSetPiece::TextMaterial() const
{
	return AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("M_AIB_TextGlow"),
		AIBAssets::Material(TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque")));
}

UTextRenderComponent* AAIBSetPiece::AddText(const FVector& Location, const FRotator& Rotation, const FText& Text, const FLinearColor& Color, float WorldSize, USceneComponent* Parent)
{
	UTextRenderComponent* Comp = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("Text_%d"), ComponentCounter++));
	Comp->SetMobility(EComponentMobility::Movable);
	Comp->SetupAttachment(Parent ? Parent : Root.Get());
	Comp->SetRelativeLocationAndRotation(Location, Rotation);
	Comp->SetText(Text);
	Comp->SetTextRenderColor(Color.ToFColor(true));
	Comp->SetWorldSize(WorldSize);
	Comp->SetHorizontalAlignment(EHTA_Center);
	Comp->SetVerticalAlignment(EVRTA_TextCenter);
	if (UMaterialInterface* Mat = TextMaterial())
	{
		Comp->SetTextMaterial(Mat);
	}
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->RegisterComponent();
	return Comp;
}
