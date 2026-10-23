// Alice in Botherland - the acidic ocean and the island's shoreline

#include "World/AIBOcean.h"
#include "World/AIBLayout.h"
#include "Core/AIBAssets.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AAIBOcean::AAIBOcean()
{
	Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Surface"));
	Surface->SetupAttachment(Root);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->bUseAsyncCooking = true;
	Surface->SetCastShadow(false);

	Skirt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Skirt"));
	Skirt->SetupAttachment(Root);
	Skirt->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Skirt->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Skirt->bUseAsyncCooking = true;
	Skirt->SetAbsolute(true, true, true);
}

void AAIBOcean::Build()
{
	BuildSurface();
	BuildSkirt();
}

namespace
{
	/** Coordinates: uniform spacing inside [-Inner, Inner], geometric growth out to +-Outer. */
	TArray<float> MakeAxis(float Min, float Max, float Step, float Outer)
	{
		TArray<float> Axis;
		float Spacing = Step;
		TArray<float> Negative;
		for (float V = Min - Step; V > -Outer; )
		{
			Negative.Add(V);
			Spacing *= 1.35f;
			V -= Spacing;
		}
		Negative.Add(-Outer);
		for (int32 Index = Negative.Num() - 1; Index >= 0; --Index)
		{
			Axis.Add(Negative[Index]);
		}
		for (float V = Min; V <= Max + 1.f; V += Step)
		{
			Axis.Add(V);
		}
		Spacing = Step;
		for (float V = Axis.Last() + Step; V < Outer; )
		{
			Axis.Add(V);
			Spacing *= 1.35f;
			V += Spacing;
		}
		Axis.Add(Outer);
		return Axis;
	}

	float Hash01(int32 A, int32 B)
	{
		uint32 H = (uint32)A * 374761393u + (uint32)B * 668265263u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return float(H & 0xFFFFFF) / float(0xFFFFFF);
	}
}

void AAIBOcean::BuildSurface()
{
	const FBox2D Island = AIBLayout::IslandBounds;
	const float Margin = 120000.f;
	const float Step = 1000.f;
	const float Outer = 3000000.f;

	const TArray<float> Xs = MakeAxis(Island.Min.X - Margin, Island.Max.X + Margin, Step, Outer);
	const TArray<float> Ys = MakeAxis(Island.Min.Y - Margin, Island.Max.Y + Margin, Step, Outer);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colors;

	const int32 NX = Xs.Num();
	const int32 NY = Ys.Num();
	Vertices.Reserve(NX * NY);
	for (int32 IY = 0; IY < NY; ++IY)
	{
		for (int32 IX = 0; IX < NX; ++IX)
		{
			Vertices.Add(FVector(Xs[IX], Ys[IY], 0.f));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(Xs[IX], Ys[IY]) / 10000.f);
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}
	}
	Triangles.Reserve((NX - 1) * (NY - 1) * 6);
	for (int32 IY = 0; IY < NY - 1; ++IY)
	{
		for (int32 IX = 0; IX < NX - 1; ++IX)
		{
			const int32 A = IY * NX + IX;
			const int32 B = A + 1;
			const int32 C = A + NX;
			const int32 D = C + 1;
			// Unreal's front faces have Cross(V1-V0, V2-V0) pointing AWAY from the viewer (same order as
			// UKismetProceduralMeshLibrary::CreateGridMeshWelded), so for an upward-facing sea it points down.
			Triangles.Append({ A, C, B, B, C, D });
		}
	}
	Surface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	UMaterialInterface* OceanMat = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("M_AIB_Ocean"));
	if (!OceanMat)
	{
		UMaterialInstanceDynamic* Fallback = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Fallback, FLinearColor(0.01f, 0.06f, 0.035f));
		OceanMat = Fallback;
	}
	Surface->SetMaterial(0, OceanMat);
	// Big bounds so WPO waves are never culled.
	Surface->SetBoundsScale(1.5f);
}

void AAIBOcean::BuildSkirt()
{
	UWorld* World = GetWorld();
	const FBox2D Island = AIBLayout::IslandBounds;
	const float Inset = 60.f;
	const float SampleStep = 800.f;
	const float SeaZ = GetSeaLevel();

	// Walk the landscape border counter-clockwise. Each sample stores its border point and outward normal;
	// corners get a fan of normals so the skirt wraps around them smoothly.
	struct FEdgeSample { FVector2D Point; FVector2D Normal; };
	TArray<FEdgeSample> Samples;
	const FVector2D Corners[4] =
	{
		FVector2D(Island.Min.X + Inset, Island.Min.Y + Inset),
		FVector2D(Island.Max.X - Inset, Island.Min.Y + Inset),
		FVector2D(Island.Max.X - Inset, Island.Max.Y - Inset),
		FVector2D(Island.Min.X + Inset, Island.Max.Y - Inset)
	};
	const FVector2D EdgeNormals[4] = { FVector2D(0, -1), FVector2D(1, 0), FVector2D(0, 1), FVector2D(-1, 0) };
	for (int32 Edge = 0; Edge < 4; ++Edge)
	{
		const FVector2D From = Corners[Edge];
		const FVector2D To = Corners[(Edge + 1) % 4];
		const float Length = FVector2D::Distance(From, To);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / SampleStep));
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Samples.Add({ FMath::Lerp(From, To, float(Step) / Steps), EdgeNormals[Edge] });
		}
		// Corner fan at To.
		const FVector2D N0 = EdgeNormals[Edge];
		const FVector2D N1 = EdgeNormals[(Edge + 1) % 4];
		for (int32 Fan = 0; Fan <= 6; ++Fan)
		{
			const float Alpha = float(Fan) / 6.f;
			FVector2D N = (N0 * (1.f - Alpha) + N1 * Alpha).GetSafeNormal();
			Samples.Add({ To, N });
		}
	}

	// Ring profile: distance outward and target depth relative to the sea.
	const float RingDist[] = { 0.f, 1800.f, 4200.f, 9000.f, 20000.f, 45000.f };
	const float RingDepth[] = { 0.f, 0.f, -350.f, -1400.f, -4000.f, -9000.f };
	const int32 NumRings = UE_ARRAY_COUNT(RingDist);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colors;
	TArray<FTransform> ShorePoints;

	for (int32 SampleIndex = 0; SampleIndex < Samples.Num(); ++SampleIndex)
	{
		const FEdgeSample& S = Samples[SampleIndex];
		const float EdgeZ = GroundZ(World, S.Point, SeaZ - 200.f) - 15.f;
		float PrevZ = EdgeZ;
		for (int32 Ring = 0; Ring < NumRings; ++Ring)
		{
			const float Noise = (Hash01(SampleIndex, Ring) - 0.5f) * (Ring == 0 ? 0.f : 500.f);
			const float Wobble = (Ring == 0) ? 0.f : (Hash01(SampleIndex * 7, Ring * 3) - 0.5f) * 700.f;
			const FVector2D P = S.Point + S.Normal * (RingDist[Ring] + Wobble);
			float Z;
			if (Ring == 0)
			{
				Z = EdgeZ;
			}
			else if (Ring == 1)
			{
				// First ring: drop half way to the waterline, keeps cliffs steep but natural.
				Z = FMath::Lerp(EdgeZ, SeaZ - 120.f, 0.62f) + Noise;
			}
			else
			{
				Z = SeaZ + RingDepth[Ring] + Noise;
			}
			Z = FMath::Min(Z, PrevZ - 40.f);
			PrevZ = Z;
			Vertices.Add(FVector(P.X, P.Y, Z));
			Normals.Add(FVector::UpVector);
			UVs.Add(P / 600.f);
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
			if (Ring == 1 && (SampleIndex % 7) == 0)
			{
				// Rocks along the waterline, long axis following the coast.
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(S.Normal.Y, S.Normal.X)) + 90.f;
				ShorePoints.Add(FTransform(FRotator(0.f, Yaw, 0.f), FVector(P.X, P.Y, FMath::Max(Z, SeaZ - 150.f))));
			}
		}
	}

	const int32 NumSamples = Samples.Num();
	for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		const int32 Next = (SampleIndex + 1) % NumSamples;
		for (int32 Ring = 0; Ring < NumRings - 1; ++Ring)
		{
			const int32 A = SampleIndex * NumRings + Ring;
			const int32 B = Next * NumRings + Ring;
			const int32 C = A + 1;
			const int32 D = B + 1;
			Triangles.Append({ A, B, C, C, B, D });
		}
	}

	// Smooth normals from triangles.
	for (FVector& N : Normals)
	{
		N = FVector::ZeroVector;
	}
	for (int32 Tri = 0; Tri < Triangles.Num(); Tri += 3)
	{
		const FVector& A = Vertices[Triangles[Tri]];
		const FVector& B = Vertices[Triangles[Tri + 1]];
		const FVector& C = Vertices[Triangles[Tri + 2]];
		FVector FaceN = FVector::CrossProduct(C - A, B - A);
		if (FaceN.Z < 0.f)
		{
			FaceN = -FaceN;
		}
		Normals[Triangles[Tri]] += FaceN;
		Normals[Triangles[Tri + 1]] += FaceN;
		Normals[Triangles[Tri + 2]] += FaceN;
	}
	for (FVector& N : Normals)
	{
		N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	}
	// Make sure triangles face up: check winding on the first triangle and flip all if needed.
	if (Triangles.Num() >= 3)
	{
		const FVector& A = Vertices[Triangles[0]];
		const FVector& B = Vertices[Triangles[1]];
		const FVector& C = Vertices[Triangles[2]];
		// An upward front face has Cross(V1-V0, V2-V0) pointing down (see BuildSurface).
		const FVector Face = FVector::CrossProduct(B - A, C - A);
		if (Face.Z > 0.f)
		{
			for (int32 Tri = 0; Tri < Triangles.Num(); Tri += 3)
			{
				Swap(Triangles[Tri + 1], Triangles[Tri + 2]);
			}
		}
	}

	Skirt->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
	UMaterialInterface* RockMat = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_ShoreRock"));
	if (!RockMat)
	{
		UMaterialInstanceDynamic* Fallback = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Fallback, FLinearColor(0.23f, 0.13f, 0.08f));
		RockMat = Fallback;
	}
	Skirt->SetMaterial(0, RockMat);

	ScatterShoreRocks(ShorePoints);
}

void AAIBOcean::ScatterShoreRocks(const TArray<FTransform>& ShoreSpots)
{
	const FString RockPaths[] =
	{
		AIBAssets::Paths::Env + TEXT("Coast/SM_Cliff_01"),
		AIBAssets::Paths::Env + TEXT("Coast/SM_Cliff_02"),
		AIBAssets::Paths::Env + TEXT("Coast/SM_Cliff_04"),
		AIBAssets::Paths::Env + TEXT("Coast/SM_CoastRocks_01"),
		AIBAssets::Paths::Env + TEXT("Coast/SM_CoastLine_01")
	};
	TArray<UStaticMesh*> Rocks;
	for (const FString& Path : RockPaths)
	{
		if (UStaticMesh* Mesh = AIBAssets::Mesh(Path))
		{
			Rocks.Add(Mesh);
		}
	}
	if (Rocks.Num() == 0)
	{
		return;
	}
	FRandomStream Stream(1337);
	for (const FTransform& Spot : ShoreSpots)
	{
		UStaticMesh* Mesh = Rocks[Stream.RandRange(0, Rocks.Num() - 1)];
		const float Scale = Stream.FRandRange(0.7f, 1.1f);
		const FRotator Rot = Spot.Rotator() + FRotator(0.f, Stream.FRandRange(-12.f, 12.f), 0.f);
		const FVector Loc = Spot.GetLocation() - GetActorLocation() + FVector(0.f, 0.f, Stream.FRandRange(-300.f, -80.f));
		if (UStaticMeshComponent* Comp = AddMesh(Mesh, FTransform(Rot, Loc, FVector(Scale)), nullptr, false))
		{
			Comp->SetCastShadow(true);
		}
	}
}
