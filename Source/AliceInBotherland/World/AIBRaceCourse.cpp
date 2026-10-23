// Alice in Botherland - Stage 3 race route from the beach to the Guardian

#include "World/AIBRaceCourse.h"
#include "World/AIBLayout.h"
#include "World/AIBRacePath.h"
#include "Core/AIBAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBRace"

void AAIBRaceCourse::BuildPath()
{
	UWorld* World = GetWorld();
	TArray<FVector2D> Control;
	for (const FVector2D& Point : AIBRacePath::Points)
	{
		Control.Add(Point);
	}
	if (Control.Num() < 2)
	{
		Control = { AIBLayout::RaceStart, AIBLayout::Guardian };
	}

	// Resample every ~15 m along the control polyline and snap to the ground.
	Path.Reset();
	const float Step = 1500.f;
	for (int32 Index = 0; Index < Control.Num() - 1; ++Index)
	{
		const FVector2D A = Control[Index];
		const FVector2D B = Control[Index + 1];
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(A, B) / Step));
		for (int32 S = 0; S < Steps; ++S)
		{
			const FVector2D P = FMath::Lerp(A, B, float(S) / Steps);
			Path.Add(FVector(P.X, P.Y, GroundZ(World, P, 0.f)));
		}
	}
	const FVector2D Last = Control.Last();
	Path.Add(FVector(Last.X, Last.Y, GroundZ(World, Last, 0.f)));

	CumulativeLength.SetNum(Path.Num());
	CumulativeLength[0] = 0.f;
	for (int32 Index = 1; Index < Path.Num(); ++Index)
	{
		CumulativeLength[Index] = CumulativeLength[Index - 1] + FVector::Dist(Path[Index - 1], Path[Index]);
	}
	// Sudden death starts at ~55% of the route (shorter road).
	const float Target = CumulativeLength.Last() * 0.55f;
	SuddenDeathIndex = 0;
	while (SuddenDeathIndex < Path.Num() - 2 && CumulativeLength[SuddenDeathIndex] < Target)
	{
		++SuddenDeathIndex;
	}
}

FTransform AAIBRaceCourse::GetStart(int32 FinalistIndex, bool bSuddenDeath) const
{
	if (Path.Num() < 2)
	{
		return GetActorTransform();
	}
	const int32 Index = bSuddenDeath ? SuddenDeathIndex : 0;
	const FVector Base = Path[Index];
	const FVector Ahead = Path[FMath::Min(Index + 2, Path.Num() - 1)];
	const FRotator Facing = (Ahead - Base).GetSafeNormal2D().Rotation();
	const FVector Side = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	const FVector2D XY = FVector2D(Base + Side * (FinalistIndex == 0 ? -420.f : 420.f));
	const float Z = GroundZ(GetWorld(), XY, Base.Z) + 70.f;
	return FTransform(Facing, FVector(XY.X, XY.Y, Z));
}

int32 AAIBRaceCourse::FindNearestIndex(const FVector& Location, int32 Hint) const
{
	int32 Best = FMath::Clamp(Hint, 0, FMath::Max(0, Path.Num() - 1));
	float BestDist = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Path.Num(); ++Index)
	{
		const float Dist = FVector::DistSquared2D(Path[Index], Location);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = Index;
		}
	}
	return Best;
}

float AAIBRaceCourse::RemainingDistance(const FVector& Location) const
{
	if (Path.Num() < 2)
	{
		return FVector::Dist2D(Location, FVector(AIBLayout::Guardian, 0.f)) / 100.f;
	}
	const int32 Nearest = FindNearestIndex(Location);
	const float Along = CumulativeLength.Last() - CumulativeLength[Nearest];
	return (Along + FVector::Dist2D(Location, Path[Nearest])) / 100.f;
}

FTransform AAIBRaceCourse::GetResetTransform(const FVector& Location) const
{
	if (Path.Num() < 2)
	{
		return GetActorTransform();
	}
	const int32 Nearest = FMath::Clamp(FindNearestIndex(Location), 0, Path.Num() - 2);
	const FVector Base = Path[Nearest];
	const FRotator Facing = (Path[Nearest + 1] - Base).GetSafeNormal2D().Rotation();
	return FTransform(Facing, Base + FVector(0.f, 0.f, 150.f));
}

void AAIBRaceCourse::Build()
{
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	BuildPath();
	if (Path.Num() < 2)
	{
		return;
	}

	UMaterialInterface* Rust = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_RustMetal"));
	if (!Rust)
	{
		UMaterialInstanceDynamic* Fallback = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Fallback, FLinearColor(0.2f, 0.1f, 0.05f));
		Rust = Fallback;
	}

	// Start gate: two pylons and a lintel with a warning light bar.
	{
		const FTransform Start = GetStart(0, false);
		const FRotator Facing = Start.Rotator();
		// The gate stands 15 m down the road so the cars drive through it at the start.
		const FVector2D GateXY = FVector2D((Start.GetLocation() + GetStart(1, false).GetLocation()) * 0.5f + Facing.Vector() * 1500.f);
		const FVector Center(GateXY.X, GateXY.Y, GroundZ(GetWorld(), GateXY, Start.GetLocation().Z));
		const FVector Side = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
		for (int32 SideIndex = -1; SideIndex <= 1; SideIndex += 2)
		{
			AddBox(Center + Side * SideIndex * 900.f + FVector(0.f, 0.f, 300.f), FVector(80.f, 80.f, 700.f), Rust, Facing);
		}
		AddBox(Center + FVector(0.f, 0.f, 650.f), FVector(60.f, 1900.f, 80.f), Rust, Facing);
		UMaterialInstanceDynamic* Bar = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.1f, 0.05f), 20.f);
		AddBox(Center + FVector(0.f, 0.f, 600.f) - Facing.Vector() * 32.f, FVector(6.f, 1700.f, 18.f), Bar, Facing, false);
		AddText(Center + FVector(0.f, 0.f, 760.f) - Facing.Vector() * 40.f, Facing + FRotator(0.f, 180.f, 0.f), LOCTEXT("Gate", "TRIAL  III"), FLinearColor(1.f, 0.85f, 0.6f), 90.f);
	}

	// Route flares every ~75 m: short poles with acid-green flares the drivers can follow.
	UMaterialInstanceDynamic* Flare = AIBAssets::MakeEmissive(this, FLinearColor(0.55f, 1.f, 0.2f), 40.f);
	float NextFlare = 2500.f;
	for (int32 Index = 1; Index < Path.Num() - 1; ++Index)
	{
		if (CumulativeLength[Index] < NextFlare)
		{
			continue;
		}
		NextFlare += 7500.f;
		const FVector Dir = (Path[Index + 1] - Path[Index - 1]).GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir);
		for (int32 SideIndex = -1; SideIndex <= 1; SideIndex += 2)
		{
			const FVector2D XY = FVector2D(Path[Index] + Side * SideIndex * 900.f);
			const FVector Ground(XY.X, XY.Y, GroundZ(GetWorld(), XY, Path[Index].Z));
			AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 90.f), FVector(0.07f, 0.07f, 1.8f)), Rust, false);
			AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 190.f), FVector(0.16f)), Flare, false);
			if (SideIndex == 1)
			{
				AddPointLight(Ground + FVector(0.f, 0.f, 200.f), FLinearColor(0.55f, 1.f, 0.25f), 250.f, 1500.f, false);
			}
		}
	}

	// Obstacles off the racing line: barrels, barriers and wrecks that punish corner cutting.
	const FString ObstaclePaths[] =
	{
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Barrel"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_ConcreteBarrier"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Box_03"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Car"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Wheel")
	};
	FRandomStream Stream(911);
	for (int32 Index = 4; Index < Path.Num() - 6; Index += 3)
	{
		UStaticMesh* Mesh = AIBAssets::Mesh(ObstaclePaths[Stream.RandRange(0, UE_ARRAY_COUNT(ObstaclePaths) - 1)]);
		if (!Mesh)
		{
			continue;
		}
		const FVector Dir = (Path[Index + 1] - Path[Index - 1]).GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir);
		const float Offset = Stream.FRandRange(650.f, 1300.f) * (Stream.FRand() < 0.5f ? -1.f : 1.f);
		const FVector2D XY = FVector2D(Path[Index] + Side * Offset);
		const FVector Ground(XY.X, XY.Y, GroundZ(GetWorld(), XY, Path[Index].Z));
		AddMesh(Mesh, FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Ground, FVector(1.f)));
	}
}

#undef LOCTEXT_NAMESPACE
