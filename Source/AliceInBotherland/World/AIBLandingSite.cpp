// Alice in Botherland - pier and beach where the champions come ashore

#include "World/AIBLandingSite.h"
#include "World/AIBLayout.h"
#include "Core/AIBAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

FTransform AAIBLandingSite::GetDisembarkSpawn(int32 Slot) const
{
	const FRotator Facing(0.f, AIBLayout::LandingYaw, 0.f);
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	const FVector Forward = Facing.Vector();
	const FVector2D Base = AIBLayout::PierStart + FVector2D(Forward.X, Forward.Y) * 900.f;
	const FVector2D XY = Base + FVector2D(Right.X, Right.Y) * ((Slot - 2) * 260.f);
	const float Z = GroundZ(GetWorld(), XY, 0.f) + 100.f;
	return FTransform(Facing, FVector(XY.X, XY.Y, Z));
}

void AAIBLandingSite::Build()
{
	UWorld* World = GetWorld();
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);

	UMaterialInterface* Wood = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_DarkWood"));
	if (!Wood)
	{
		UMaterialInstanceDynamic* WoodMID = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(WoodMID, FLinearColor(0.16f, 0.09f, 0.05f));
		Wood = WoodMID;
	}
	UMaterialInterface* Rust = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_RustMetal"), Wood);

	// ---------------- Pier ----------------
	const FVector2D Start = AIBLayout::PierStart;
	const FRotator PierRot(0.f, AIBLayout::PierYaw, 0.f);
	const FVector Dir = PierRot.Vector();
	const FVector Side = FRotationMatrix(PierRot).GetUnitAxis(EAxis::Y);
	const float StartGround = GroundZ(World, Start, 0.f);
	DeckZ = FMath::Max(StartGround + 30.f, AIBLayout::SeaLevel + 260.f);
	PierDeckStart = FVector(Start.X, Start.Y, DeckZ);

	const float Width = 420.f;
	const float PlankDepth = 30.f;
	const int32 NumPlanks = FMath::CeilToInt(AIBLayout::PierLength / 34.f);
	FRandomStream Stream(77);
	for (int32 Index = 0; Index < NumPlanks; ++Index)
	{
		// Some missing planks for a ruined look, but never two in a row.
		if (Index > 6 && Index % 11 == 5)
		{
			continue;
		}
		const FVector Center = PierDeckStart + Dir * (Index * 34.f + 17.f) + FVector(0.f, 0.f, Stream.FRandRange(-2.f, 2.f));
		const FRotator Tilt(Stream.FRandRange(-0.8f, 0.8f), AIBLayout::PierYaw, Stream.FRandRange(-0.6f, 0.6f));
		AddBox(Center, FVector(30.f, Width * Stream.FRandRange(0.93f, 1.f), 8.f), Wood, Tilt);
	}
	// Stringers and posts.
	for (int32 SideIndex = -1; SideIndex <= 1; SideIndex += 2)
	{
		const FVector Offset = Side * (Width * 0.5f - 15.f) * SideIndex;
		const FVector Mid = PierDeckStart + Dir * AIBLayout::PierLength * 0.5f + Offset - FVector(0.f, 0.f, PlankDepth);
		AddBox(Mid, FVector(AIBLayout::PierLength, 24.f, 30.f), Wood, PierRot);
		for (float D = 0.f; D <= AIBLayout::PierLength; D += 450.f)
		{
			const FVector Top = PierDeckStart + Dir * D + Offset;
			const float Bottom = AIBLayout::SeaLevel - 400.f;
			const float Height = Top.Z - Bottom + 110.f;
			AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(Top.X, Top.Y, Bottom + Height * 0.5f), FVector(0.32f, 0.32f, Height / 100.f)), Wood);
		}
		// Rope rail.
		AddBox(PierDeckStart + Dir * AIBLayout::PierLength * 0.5f + Offset + FVector(0.f, 0.f, 95.f), FVector(AIBLayout::PierLength, 6.f, 6.f), Rust, PierRot, false);
	}
	// Lantern posts along the pier.
	for (float D = 600.f; D <= AIBLayout::PierLength; D += 1500.f)
	{
		const FVector Post = PierDeckStart + Dir * D + Side * (Width * 0.5f);
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, Post + FVector(0.f, 0.f, 120.f), FVector(0.12f, 0.12f, 2.4f)), Rust);
		UMaterialInstanceDynamic* Flame = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.55f, 0.18f), 30.f);
		AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, Post + FVector(0.f, 0.f, 250.f), FVector(0.18f)), Flame, false);
		AddPointLight(Post + FVector(0.f, 0.f, 250.f), FLinearColor(1.f, 0.6f, 0.25f), 300.f, 1800.f, false);
	}

	// ---------------- Beach camp ----------------
	const FString PropPaths[] =
	{
		AIBAssets::Paths::Env + TEXT("Props/SM_WoodenCrate_01"),
		AIBAssets::Paths::Env + TEXT("Props/SM_WoodenCrate_02"),
		AIBAssets::Paths::Env + TEXT("Props/SM_WoodenBarrels_01")
	};
	const FRotator Inland(0.f, AIBLayout::LandingYaw, 0.f);
	const FVector InlandDir = Inland.Vector();
	const FVector InlandSide = FRotationMatrix(Inland).GetUnitAxis(EAxis::Y);
	for (int32 Index = 0; Index < 7; ++Index)
	{
		UStaticMesh* Mesh = AIBAssets::Mesh(PropPaths[Index % 3]);
		const FVector2D XY = Start + FVector2D(InlandSide.X, InlandSide.Y) * (Index % 2 == 0 ? 1.f : -1.f) * Stream.FRandRange(700.f, 1300.f)
			+ FVector2D(InlandDir.X, InlandDir.Y) * Stream.FRandRange(-200.f, 800.f);
		const FVector Loc(XY.X, XY.Y, GroundZ(World, XY, 0.f));
		if (Mesh)
		{
			AddMesh(Mesh, FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Loc, FVector(1.f)));
		}
		else
		{
			AddBox(Loc + FVector(0.f, 0.f, 45.f), FVector(90.f), Wood, FRotator(0.f, Stream.FRandRange(0.f, 90.f), 0.f));
		}
	}

	// Community banners in a crescent behind the landing spots.
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		const FTransform Spawn = GetDisembarkSpawn(Slot);
		const FVector Base = Spawn.GetLocation() + InlandDir * 650.f - FVector(0.f, 0.f, 100.f);
		const FVector2D BaseXY(Base.X, Base.Y);
		const FVector Ground(Base.X, Base.Y, GroundZ(World, BaseXY, Base.Z));
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 230.f), FVector(0.08f, 0.08f, 4.6f)), Rust);
		const FLinearColor Color = AIB::Community(Slot).Color;
		UMaterialInstanceDynamic* Cloth = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Cloth, Color * 0.6f);
		AddBox(Ground + FVector(0.f, 0.f, 380.f) + InlandSide * 45.f, FVector(4.f, 90.f, 150.f), Cloth, Inland, false);
		UMaterialInstanceDynamic* Glow = AIBAssets::MakeEmissive(this, Color, 15.f);
		AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 470.f), FVector(0.14f)), Glow, false);
	}

	// Signal fire on the beach.
	const FVector2D FireXY = Start + FVector2D(InlandDir.X, InlandDir.Y) * 1500.f;
	const FVector Fire(FireXY.X, FireXY.Y, GroundZ(World, FireXY, 0.f));
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float Angle = Index * 60.f;
		const FVector Offset = FRotator(0.f, Angle, 0.f).Vector() * 55.f;
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator(60.f, Angle, 0.f), Fire + Offset + FVector(0.f, 0.f, 25.f), FVector(0.12f, 0.12f, 1.1f)), Wood, false);
	}
	UMaterialInstanceDynamic* Embers = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.35f, 0.05f), 60.f);
	AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, Fire + FVector(0.f, 0.f, 30.f), FVector(0.55f, 0.55f, 0.3f)), Embers, false);
	AddPointLight(Fire + FVector(0.f, 0.f, 120.f), FLinearColor(1.f, 0.48f, 0.15f), 2500.f, 2600.f, true);
}
