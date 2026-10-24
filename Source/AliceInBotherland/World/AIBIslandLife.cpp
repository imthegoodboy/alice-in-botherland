// Alice in Botherland - life on the island: gulls, tumbleweeds, roadside wrecks, signposts and campfire sparks

#include "World/AIBIslandLife.h"
#include "World/AIBLayout.h"
#include "World/AIBRacePath.h"
#include "Core/AIBAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "AIBIslandLife"

namespace
{
	UStaticMesh* PackProp(const TCHAR* Name)
	{
		return AIBAssets::Mesh(FString(TEXT("/Game/JustBStudios/PostApo/Meshes/")) + Name);
	}

	constexpr int32 RoadPoints = UE_ARRAY_COUNT(AIBRacePath::Points);
}

AAIBIslandLife::AAIBIslandLife()
{
	PrimaryActorTick.bCanEverTick = true;
	VisibleInZones = { EAIBSceneZone::Island };
}

FVector2D AAIBIslandLife::RoadDir(int32 Index) const
{
	const int32 A = FMath::Clamp(Index - 1, 0, RoadPoints - 1);
	const int32 B = FMath::Clamp(Index + 1, 0, RoadPoints - 1);
	return (AIBRacePath::Points[B] - AIBRacePath::Points[A]).GetSafeNormal();
}

FVector AAIBIslandLife::RoadSide(int32 Index, float Offset) const
{
	const int32 Clamped = FMath::Clamp(Index, 0, RoadPoints - 1);
	const FVector2D Dir = RoadDir(Clamped);
	const FVector2D XY = AIBRacePath::Points[Clamped] + FVector2D(-Dir.Y, Dir.X) * Offset;
	return FVector(XY.X, XY.Y, GroundZ(GetWorld(), XY, 0.f));
}

void AAIBIslandLife::Build()
{
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	BuildBeach();
	BuildRoadside();
	BuildSignposts();
	BuildTumbleweeds();
	BuildGulls();
}

// ---------------------------------------------------------------------------------------------------------------------
// Gulls
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::BuildGulls()
{
	UMaterialInstanceDynamic* Feathers = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
	AIBAssets::SetColor(Feathers, FLinearColor(0.78f, 0.78f, 0.74f));
	UMaterialInstanceDynamic* Tips = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
	AIBAssets::SetColor(Tips, FLinearColor(0.08f, 0.08f, 0.09f));

	struct FFlock { FVector2D XY; bool bOverSea; float Height; int32 Count; float MinRadius; float MaxRadius; };
	const FFlock Flocks[] =
	{
		// Over the cove where the ships come in.
		{ AIBLayout::FleetDocked + FVector2D(1500.f, 2500.f), true, 2200.f, 6, 1600.f, 3600.f },
		// Above the town halfway along the road.
		{ AIBRacePath::Points[RoadPoints / 2], false, 2800.f, 4, 2000.f, 3200.f },
		// Circling the Guardian's beam like vultures.
		{ AIBLayout::Guardian, false, 4200.f, 5, 1400.f, 2600.f },
	};
	FRandomStream Stream(4242);
	int32 Counter = 0;
	for (const FFlock& Flock : Flocks)
	{
		const float BaseZ = Flock.bOverSea ? AIBLayout::SeaLevel : GroundZ(GetWorld(), Flock.XY, 0.f);
		const bool bClockwise = Stream.FRand() < 0.5f;
		for (int32 Index = 0; Index < Flock.Count; ++Index)
		{
			FGullFlight Flight;
			Flight.Center = FVector(Flock.XY.X, Flock.XY.Y, BaseZ + Flock.Height + Stream.FRandRange(-300.f, 400.f));
			Flight.Radius = Stream.FRandRange(Flock.MinRadius, Flock.MaxRadius);
			Flight.Speed = Stream.FRandRange(700.f, 1100.f);
			Flight.Phase = Stream.FRandRange(0.f, UE_TWO_PI);
			Flight.FlapRate = Stream.FRandRange(2.2f, 3.2f);
			Flight.Bob = Stream.FRandRange(80.f, 260.f);
			Flight.bClockwise = bClockwise;
			GullFlights.Add(Flight);

			// A body and two wings that pivot at the shoulders; forward is +X. Gulls are drawn a little large so
			// they read from the ground.
			USceneComponent* Body = MakeScene(*FString::Printf(TEXT("Gull_%d"), Counter), FTransform(FRotator::ZeroRotator, Flight.Center, FVector(1.7f)));
			AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.34f, 0.11f, 0.1f)), Feathers, false, Body);
			AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, FVector(19.f, 0.f, 3.f), FVector(0.1f, 0.08f, 0.08f)), Feathers, false, Body);
			AddBox(FVector(-20.f, 0.f, 0.f), FVector(14.f, 12.f, 1.5f), Tips, FRotator::ZeroRotator, false, Body);
			USceneComponent* WingL = MakeScene(*FString::Printf(TEXT("GullWingL_%d"), Counter), FTransform(FVector(2.f, -4.f, 2.f)), Body);
			USceneComponent* WingR = MakeScene(*FString::Printf(TEXT("GullWingR_%d"), Counter), FTransform(FVector(2.f, 4.f, 2.f)), Body);
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				USceneComponent* Wing = Side < 0 ? WingL : WingR;
				AddBox(FVector(0.f, Side * 24.f, 0.f), FVector(15.f, 48.f, 1.5f), Feathers, FRotator::ZeroRotator, false, Wing);
				AddBox(FVector(-3.f, Side * 54.f, 0.f), FVector(10.f, 14.f, 1.5f), Tips, FRotator(0.f, Side * 12.f, 0.f), false, Wing);
			}
			for (USceneComponent* Comp : { Body, WingL, WingR })
			{
				TArray<USceneComponent*> Parts;
				Comp->GetChildrenComponents(false, Parts);
				for (USceneComponent* Child : Parts)
				{
					if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Child))
					{
						Prim->SetCastShadow(false);
					}
				}
			}
			GullBodies.Add(Body);
			GullWingsL.Add(WingL);
			GullWingsR.Add(WingR);
			++Counter;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Tumbleweeds
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::BuildTumbleweeds()
{
	WindDir = FVector2D(0.55f, 1.f).GetSafeNormal();
	UStaticMesh* Bush = PackProp(TEXT("SM_DryBush_02"));
	UMaterialInterface* Material = nullptr;
	if (!Bush)
	{
		UMaterialInstanceDynamic* Dry = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Dry, FLinearColor(0.32f, 0.22f, 0.11f));
		Material = Dry;
	}
	FRandomStream Stream(1717);
	const int32 Anchors[] = { 3, 6, 9, 12, 15, 18, 21, 24 };
	for (int32 Anchor : Anchors)
	{
		if (Anchor >= RoadPoints)
		{
			continue;
		}
		FTumble Tumble;
		const FVector Side = RoadSide(Anchor, Stream.FRandRange(-1800.f, 1800.f));
		Tumble.Distance = Stream.FRandRange(4500.f, 7000.f);
		// Start upwind so it rolls across the road.
		Tumble.Start = FVector2D(Side) - WindDir * Tumble.Distance * 0.5f;
		Tumble.Travel = Stream.FRandRange(0.f, Tumble.Distance);
		Tumble.Speed = Stream.FRandRange(260.f, 520.f);
		Tumble.Radius = Stream.FRandRange(32.f, 50.f);
		Tumbles.Add(Tumble);

		const float Scale = Tumble.Radius / 45.f;
		UStaticMeshComponent* Mesh = Bush
			? AddMesh(Bush, FTransform(FRotator::ZeroRotator, Side, FVector(Scale * 0.9f)), nullptr, false)
			: AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, Side, FVector(Tumble.Radius / 50.f)), Material, false);
		if (Mesh)
		{
			Mesh->SetCastShadow(true);
		}
		TumbleMeshes.Add(Mesh);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Roadside: a dead power line, wrecks, an old outpost, scrub
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::BuildRoadside()
{
	FRandomStream Stream(6060);
	UMaterialInterface* Rust = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_RustMetal"));
	UMaterialInstanceDynamic* Cable = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
	AIBAssets::SetColor(Cable, FLinearColor(0.03f, 0.03f, 0.03f));

	// Power line: poles 20 m off the road from the beach to the town, a sagging cable between them, some leaning.
	if (UStaticMesh* Pole = PackProp(TEXT("SM_Electric_Pole_01")))
	{
		const float PoleHeight = FMath::Max(400.f, Pole->GetBoundingBox().Max.Z);
		FVector Previous = FVector::ZeroVector;
		bool bHasPrevious = false;
		for (int32 Index = 1; Index < FMath::Min(RoadPoints - 4, 17); ++Index)
		{
			const FVector Base = RoadSide(Index, 2000.f);
			const bool bFallen = Index == 7;
			const float Lean = bFallen ? 0.f : Stream.FRandRange(-5.f, 5.f);
			const FVector2D Dir = RoadDir(Index);
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
			if (bFallen)
			{
				// One pole snapped and lies in the scrub; the cable ends here.
				AddMesh(Pole, FTransform(FRotator(84.f, Yaw + 25.f, 0.f), Base + FVector(0.f, 0.f, 40.f), FVector(1.f)));
				bHasPrevious = false;
				continue;
			}
			AddMesh(Pole, FTransform(FRotator(Lean, Yaw + 90.f, 0.f), Base, FVector(1.f)));
			const FVector Top = Base + FVector(0.f, 0.f, PoleHeight * 0.93f);
			if (bHasPrevious)
			{
				// Two straight halves dipping in the middle read as a sagging wire.
				const FVector Mid = (Previous + Top) * 0.5f - FVector(0.f, 0.f, 120.f);
				for (const TPair<FVector, FVector>& Span : { TPair<FVector, FVector>(Previous, Mid), TPair<FVector, FVector>(Mid, Top) })
				{
					const FVector Delta = Span.Value - Span.Key;
					AddBox((Span.Key + Span.Value) * 0.5f, FVector(Delta.Size(), 3.f, 3.f), Cable, Delta.Rotation(), false);
				}
			}
			Previous = Top;
			bHasPrevious = true;
		}
	}

	// Wrecks: cars left where they died, with barrels and rubbish around them.
	struct FWreck { int32 Index; float Offset; float Roll; };
	const FWreck Wrecks[] = { { 4, -2300.f, 0.f }, { 10, 2600.f, 175.f }, { 14, -2400.f, 0.f }, { 20, 2500.f, 90.f }, { 26, -2600.f, 0.f } };
	UStaticMesh* Car = PackProp(TEXT("SM_Car"));
	UStaticMesh* Barrel = PackProp(TEXT("SM_Barrel"));
	UStaticMesh* Junk[] = { PackProp(TEXT("SM_Rubbish_01")), PackProp(TEXT("SM_Rubbish_02")), PackProp(TEXT("SM_Wheel")), PackProp(TEXT("SM_Box_01")) };
	for (const FWreck& Wreck : Wrecks)
	{
		if (Wreck.Index >= RoadPoints)
		{
			continue;
		}
		const FVector Base = RoadSide(Wreck.Index, Wreck.Offset);
		const float Yaw = Stream.FRandRange(0.f, 360.f);
		if (Car)
		{
			const float Lift = Wreck.Roll > 120.f ? 110.f : (Wreck.Roll > 45.f ? 80.f : 0.f);
			AddMesh(Car, FTransform(FRotator(0.f, Yaw, Wreck.Roll), Base + FVector(0.f, 0.f, Lift), FVector(1.f)));
		}
		for (int32 Prop = 0; Prop < 3; ++Prop)
		{
			UStaticMesh* Mesh = Prop == 0 ? Barrel : Junk[Stream.RandRange(0, UE_ARRAY_COUNT(Junk) - 1)];
			if (!Mesh)
			{
				continue;
			}
			const FVector2D Around = FVector2D(Base) + FVector2D(Stream.FRandRange(-450.f, 450.f), Stream.FRandRange(-450.f, 450.f));
			const FVector Ground(Around.X, Around.Y, GroundZ(GetWorld(), Around, Base.Z));
			const float Tip = Prop == 0 && Stream.FRand() < 0.5f ? 90.f : 0.f;
			AddMesh(Mesh, FTransform(FRotator(Tip, Stream.FRandRange(0.f, 360.f), 0.f), Ground + FVector(0.f, 0.f, Tip > 0.f ? 30.f : 0.f), FVector(1.f)), nullptr, false);
		}
	}

	// An abandoned outpost before the Guardian's plateau: water tank, trailer, a dish and solar panels.
	const int32 Outpost = FMath::Max(0, RoadPoints - 7);
	const FVector OutpostBase = RoadSide(Outpost, -3200.f);
	const FVector2D OutDir = RoadDir(Outpost);
	const float OutYaw = FMath::RadiansToDegrees(FMath::Atan2(OutDir.Y, OutDir.X));
	struct FPiece { const TCHAR* Name; FVector Local; float Yaw; bool bCollide; };
	const FPiece Pieces[] =
	{
		{ TEXT("SM_WaterTankMerged"), FVector(0.f, 0.f, 0.f), 0.f, true },
		{ TEXT("SM_Trailer"), FVector(900.f, -700.f, 0.f), 70.f, true },
		{ TEXT("SM_Satellite_01"), FVector(-700.f, -500.f, 0.f), 200.f, true },
		{ TEXT("SM_Solar_Panel"), FVector(-300.f, 800.f, 0.f), 15.f, false },
		{ TEXT("SM_Solar_Panel"), FVector(200.f, 900.f, 0.f), 15.f, false },
		{ TEXT("SM_Electric_Cabinet"), FVector(500.f, 300.f, 0.f), 90.f, true },
		{ TEXT("SM_PaletteBoxes_01"), FVector(1100.f, 200.f, 0.f), 30.f, true },
	};
	for (const FPiece& Piece : Pieces)
	{
		if (UStaticMesh* Mesh = PackProp(Piece.Name))
		{
			const FVector Offset = FRotator(0.f, OutYaw, 0.f).RotateVector(Piece.Local);
			const FVector2D XY = FVector2D(OutpostBase + Offset);
			const FVector Ground(XY.X, XY.Y, GroundZ(GetWorld(), XY, OutpostBase.Z));
			AddMesh(Mesh, FTransform(FRotator(0.f, OutYaw + Piece.Yaw, 0.f), Ground, FVector(1.f)), nullptr, Piece.bCollide);
		}
	}
	if (UStaticMesh* Barrier = PackProp(TEXT("SM_BarbedWire")))
	{
		for (int32 Index = -2; Index <= 2; ++Index)
		{
			const FVector Offset = FRotator(0.f, OutYaw, 0.f).RotateVector(FVector(Index * 420.f, 1500.f, 0.f));
			const FVector2D XY = FVector2D(OutpostBase + Offset);
			AddMesh(Barrier, FTransform(FRotator(0.f, OutYaw, 0.f), FVector(XY.X, XY.Y, GroundZ(GetWorld(), XY, OutpostBase.Z)), FVector(1.f)), nullptr, false);
		}
	}

	// Scrub and rocks scattered either side of the road.
	UStaticMesh* Scrub[] = { PackProp(TEXT("SM_DryBush_01_Short")), PackProp(TEXT("SM_DryBush_02")), PackProp(TEXT("SM_WildGrass_M_01")), PackProp(TEXT("SM_WildGrass_S_02")), PackProp(TEXT("SM_DryGrass_01_1")), PackProp(TEXT("SM_Cactus")) };
	UStaticMesh* Rocks[] = { PackProp(TEXT("SM_Rock_02")), PackProp(TEXT("SM_Rock_04")), PackProp(TEXT("SM_Rock_06")) };
	for (int32 Index = 1; Index < RoadPoints - 2; ++Index)
	{
		for (int32 Each = 0; Each < 3; ++Each)
		{
			const bool bRock = Stream.FRand() < 0.18f;
			UStaticMesh* Mesh = bRock ? Rocks[Stream.RandRange(0, UE_ARRAY_COUNT(Rocks) - 1)] : Scrub[Stream.RandRange(0, UE_ARRAY_COUNT(Scrub) - 1)];
			if (!Mesh)
			{
				continue;
			}
			const float Offset = Stream.FRandRange(1500.f, 3200.f) * (Stream.FRand() < 0.5f ? -1.f : 1.f);
			const FVector Base = RoadSide(Index, Offset) + FVector(Stream.FRandRange(-500.f, 500.f), Stream.FRandRange(-500.f, 500.f), 0.f);
			const FVector Ground(Base.X, Base.Y, GroundZ(GetWorld(), FVector2D(Base), Base.Z));
			AddMesh(Mesh, FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Ground - FVector(0.f, 0.f, bRock ? 30.f : 0.f), FVector(bRock ? Stream.FRandRange(0.4f, 0.8f) : Stream.FRandRange(0.8f, 1.3f))), nullptr, false);
		}
	}
	(void)Rust;
}

// ---------------------------------------------------------------------------------------------------------------------
// Signposts
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::BuildSignposts()
{
	UMaterialInterface* Wood = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_DarkWood"));
	if (!Wood)
	{
		UMaterialInstanceDynamic* WoodMID = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(WoodMID, FLinearColor(0.2f, 0.12f, 0.06f));
		Wood = WoodMID;
	}
	const FLinearColor Paint(1.f, 0.82f, 0.55f);

	struct FSign { int32 Index; float Offset; FText Ahead; FText Behind; };
	const FSign Signs[] =
	{
		{ 2, 750.f, LOCTEXT("SignGuardian", "THE GUARDIAN"), LOCTEXT("SignBeach", "THE BEACH") },
		{ RoadPoints / 2, -750.f, LOCTEXT("SignGuardian2", "THE GUARDIAN"), LOCTEXT("SignLanding", "LANDING") },
		{ RoadPoints - 5, 800.f, LOCTEXT("SignTurnBack", "NO RETURN"), LOCTEXT("SignTown", "THE TOWN") },
	};
	for (const FSign& Sign : Signs)
	{
		const FVector Base = RoadSide(Sign.Index, Sign.Offset);
		const FVector2D Dir = RoadDir(Sign.Index);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		AddBox(Base + FVector(0.f, 0.f, 160.f), FVector(16.f, 16.f, 320.f), Wood, FRotator(0.f, Yaw, 2.f), true);
		// Two arrow boards, one pointing on along the road, one back the way you came.
		for (int32 Board = 0; Board < 2; ++Board)
		{
			const float BoardYaw = Yaw + (Board == 0 ? 0.f : 180.f) + (Board == 0 ? -4.f : 6.f);
			const FRotator Rot(0.f, BoardYaw, Board == 0 ? -2.f : 3.f);
			const FVector Along = Rot.Vector();
			const FVector Center = Base + FVector(0.f, 0.f, Board == 0 ? 270.f : 215.f) + Along * 95.f;
			AddBox(Center, FVector(190.f, 8.f, 40.f), Wood, Rot, false);
			AddBox(Center + Along * 105.f, FVector(28.f, 8.f, 28.f), Wood, Rot + FRotator(0.f, 0.f, 45.f), false);
			const FText& Label = Board == 0 ? Sign.Ahead : Sign.Behind;
			const FVector Normal = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);
			// Lettering on both faces (text reads towards its +X).
			AddText(Center + Normal * 5.f, Normal.Rotation(), Label, Paint, 22.f);
			AddText(Center - Normal * 5.f, (-Normal).Rotation(), Label, Paint, 22.f);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Beach: a wreck on the sand and sparks over the signal fire
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::BuildBeach()
{
	const FRotator Inland(0.f, AIBLayout::LandingYaw, 0.f);
	const FVector InlandDir = Inland.Vector();
	const FVector InlandSide = FRotationMatrix(Inland).GetUnitAxis(EAxis::Y);

	// An older boat that never left: half buried, heeled over, a little way down the beach.
	if (UStaticMesh* Wreck = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Ships/SM_Ship_Pinnace")))
	{
		const FVector2D XY = AIBLayout::LandingBeach - FVector2D(InlandSide.X, InlandSide.Y) * 3600.f - FVector2D(InlandDir.X, InlandDir.Y) * 300.f;
		const float Z = GroundZ(GetWorld(), XY, AIBLayout::SeaLevel + 250.f);
		AddMesh(Wreck, FTransform(FRotator(-6.f, AIBLayout::LandingYaw + 115.f, 28.f), FVector(XY.X, XY.Y, Z - 90.f), FVector(0.42f)), nullptr, false);
	}

	// Sparks over the signal fire (same spot the landing site builds it).
	const FVector2D FireXY = AIBLayout::PierStart + FVector2D(InlandDir.X, InlandDir.Y) * 1500.f;
	FireLocation = FVector(FireXY.X, FireXY.Y, GroundZ(GetWorld(), FireXY, 0.f) + 40.f);
	UMaterialInstanceDynamic* Ember = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.42f, 0.08f), 120.f);
	FRandomStream Stream(99);
	for (int32 Index = 0; Index < 18; ++Index)
	{
		FSpark Spark;
		Spark.Life = Stream.FRandRange(1.1f, 2.4f);
		Spark.Born = -Stream.FRandRange(0.f, Spark.Life);
		Spark.Drift = FVector(Stream.FRandRange(-60.f, 60.f), Stream.FRandRange(-60.f, 60.f), Stream.FRandRange(160.f, 300.f));
		Spark.Size = Stream.FRandRange(0.025f, 0.05f);
		Sparks.Add(Spark);
		UStaticMeshComponent* Mesh = AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, FireLocation, FVector(Spark.Size)), Ember, false);
		if (Mesh)
		{
			Mesh->SetCastShadow(false);
		}
		SparkMeshes.Add(Mesh);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------------------------------------------------

void AAIBIslandLife::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsHidden() || !bBuilt)
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();

	// Gulls: wide circles with a slow bob, flapping in bursts between glides.
	for (int32 Index = 0; Index < GullFlights.Num(); ++Index)
	{
		const FGullFlight& Flight = GullFlights[Index];
		USceneComponent* Body = GullBodies[Index];
		if (!Body)
		{
			continue;
		}
		const float Direction = Flight.bClockwise ? -1.f : 1.f;
		const float Angle = Flight.Phase + Direction * Time * Flight.Speed / Flight.Radius;
		const FVector Offset(FMath::Cos(Angle) * Flight.Radius, FMath::Sin(Angle) * Flight.Radius, FMath::Sin(Time * 0.35f + Flight.Phase) * Flight.Bob);
		const FVector Tangent = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f) * Direction;
		const FRotator Heading(FMath::Cos(Time * 0.35f + Flight.Phase) * 6.f, Tangent.Rotation().Yaw, -Direction * 14.f);
		Body->SetWorldLocationAndRotation(Flight.Center + Offset, Heading);

		const bool bGliding = FMath::Sin(Time * 0.45f + Flight.Phase * 3.f) > 0.25f;
		const float Flap = bGliding ? 6.f + FMath::Sin(Time * 1.3f + Flight.Phase) * 3.f : FMath::Sin(Time * Flight.FlapRate * UE_TWO_PI) * 34.f;
		if (GullWingsL[Index])
		{
			GullWingsL[Index]->SetRelativeRotation(FRotator(0.f, 0.f, -Flap));
		}
		if (GullWingsR[Index])
		{
			GullWingsR[Index]->SetRelativeRotation(FRotator(0.f, 0.f, Flap));
		}
	}

	// Tumbleweeds roll downwind across the road, bounce a little, and fade in again upwind.
	const FVector Wind3(WindDir.X, WindDir.Y, 0.f);
	const FVector RollAxis = FVector::CrossProduct(FVector::UpVector, Wind3).GetSafeNormal();
	for (int32 Index = 0; Index < Tumbles.Num(); ++Index)
	{
		FTumble& Tumble = Tumbles[Index];
		UStaticMeshComponent* Mesh = TumbleMeshes[Index];
		if (!Mesh)
		{
			continue;
		}
		const float Gust = 0.75f + 0.5f * FMath::Max(0.f, FMath::Sin(Time * 0.6f + Index * 1.7f));
		Tumble.Travel += Tumble.Speed * Gust * DeltaSeconds;
		if (Tumble.Travel > Tumble.Distance)
		{
			Tumble.Travel = 0.f;
		}
		const FVector2D XY = Tumble.Start + WindDir * Tumble.Travel;
		const float Bounce = FMath::Abs(FMath::Sin(Tumble.Travel * 0.004f)) * 45.f;
		const float Ground = GroundZ(GetWorld(), XY, 0.f);
		const float Fade = FMath::Clamp(Tumble.Travel / 400.f, 0.f, 1.f) * FMath::Clamp((Tumble.Distance - Tumble.Travel) / 400.f, 0.f, 1.f);
		const float Scale = Tumble.Radius / 45.f * FMath::Max(0.05f, Fade);
		const FQuat Roll(-RollAxis, Tumble.Travel / FMath::Max(10.f, Tumble.Radius));
		Mesh->SetWorldLocationAndRotation(FVector(XY.X, XY.Y, Ground + Tumble.Radius * 0.6f + Bounce), Roll);
		Mesh->SetWorldScale3D(FVector(Scale));
	}

	// Sparks rise, drift and wink out.
	for (int32 Index = 0; Index < Sparks.Num(); ++Index)
	{
		FSpark& Spark = Sparks[Index];
		UStaticMeshComponent* Mesh = SparkMeshes[Index];
		if (!Mesh)
		{
			continue;
		}
		float Age = Time - Spark.Born;
		if (Age > Spark.Life)
		{
			Spark.Born = Time;
			Spark.Drift = FVector(FMath::FRandRange(-70.f, 70.f), FMath::FRandRange(-70.f, 70.f), FMath::FRandRange(160.f, 320.f));
			Age = 0.f;
		}
		const float T = Age / Spark.Life;
		const FVector Swirl(FMath::Sin(Age * 5.f + Index) * 18.f, FMath::Cos(Age * 4.f + Index) * 18.f, 0.f);
		Mesh->SetWorldLocation(FireLocation + Spark.Drift * Age + Swirl);
		Mesh->SetWorldScale3D(FVector(Spark.Size * (1.f - T * 0.8f)));
	}
}

#undef LOCTEXT_NAMESPACE
