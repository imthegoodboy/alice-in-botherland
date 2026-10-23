// Alice in Botherland - Stage 2 arena: the abandoned Grand Meridian Hotel lobby

#include "World/AIBHotelArena.h"
#include "Core/AIBAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBHotel"

AAIBHotelArena::AAIBHotelArena()
{
	PrimaryActorTick.bCanEverTick = true;
	VisibleInZones = { EAIBSceneZone::Hotel };
}

FTransform AAIBHotelArena::GetSpawn(int32 Index, int32 Count) const
{
	const float Angle = FMath::DegreesToRadians(Index * 360.f / FMath::Max(Count, 1) + 45.f);
	const FVector Offset(FMath::Cos(Angle) * 320.f, FMath::Sin(Angle) * 320.f, 100.f);
	return FTransform((-Offset.GetSafeNormal2D()).Rotation(), GetActorLocation() + Offset);
}

FTransform AAIBHotelArena::GetOverviewCamera(float Time) const
{
	const float Sway = FMath::Sin(Time * 0.07f);
	const FVector CamLoc = GetActorLocation() + FVector(-HalfX + 300.f, -HalfY + 300.f + Sway * 300.f, 820.f);
	const FVector LookAt = GetActorLocation() + FVector(200.f, 100.f, 80.f);
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FBox AAIBHotelArena::GetWalkBounds() const
{
	const FVector C = GetActorLocation();
	return FBox(C + FVector(-HalfX + 120.f, -HalfY + 120.f, -100.f), C + FVector(HalfX - 120.f, HalfY - 120.f, Height));
}

void AAIBHotelArena::PlaceProp(const FString& Path, const FVector& Location, const FRotator& Rotation, float TargetSize, UMaterialInterface* FallbackMat, const FVector& FallbackSize)
{
	if (UStaticMesh* Mesh = AIBAssets::Mesh(Path))
	{
		const FVector Size = Mesh->GetBoundingBox().GetSize();
		const float Largest = FMath::Max3(Size.X, Size.Y, Size.Z);
		const float Scale = (TargetSize > 0.f && Largest > 1.f) ? TargetSize / Largest : 1.f;
		AddMesh(Mesh, FTransform(Rotation, Location - FVector(0.f, 0.f, Mesh->GetBoundingBox().Min.Z * Scale), FVector(Scale)));
	}
	else if (FallbackMat)
	{
		AddBox(Location + FVector(0.f, 0.f, FallbackSize.Z * 0.5f), FallbackSize, FallbackMat, Rotation);
	}
}

void AAIBHotelArena::Build()
{
	auto Mat = [this](const TCHAR* Name, const FLinearColor& Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Found = AIBAssets::Material(AIBAssets::Paths::Materials + Name))
		{
			return Found;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(MID, Fallback);
		return MID;
	};
	UMaterialInterface* Floor = Mat(TEXT("MI_AIB_HotelFloor"), FLinearColor(0.35f, 0.33f, 0.3f));
	UMaterialInterface* Wall = Mat(TEXT("MI_AIB_HotelWall"), FLinearColor(0.32f, 0.27f, 0.22f));
	UMaterialInterface* Wood = Mat(TEXT("MI_AIB_DarkWood"), FLinearColor(0.12f, 0.06f, 0.03f));
	UMaterialInterface* Velvet = Mat(TEXT("MI_AIB_Velvet"), FLinearColor(0.25f, 0.02f, 0.03f));
	UMaterialInterface* Plaster = Mat(TEXT("MI_AIB_Plaster"), FLinearColor(0.55f, 0.52f, 0.47f));
	UMaterialInterface* Brass = Mat(TEXT("MI_AIB_Brass"), FLinearColor(0.6f, 0.45f, 0.18f));

	// ---------------- Shell ----------------
	AddBox(FVector(0.f, 0.f, -20.f), FVector(HalfX * 2.f + 200.f, HalfY * 2.f + 200.f, 40.f), Floor);
	AddBox(FVector(0.f, 0.f, Height + 30.f), FVector(HalfX * 2.f + 200.f, HalfY * 2.f + 200.f, 60.f), Plaster);
	// Coffered ceiling beams.
	for (float X = -HalfX + 450.f; X < HalfX; X += 600.f)
	{
		AddBox(FVector(X, 0.f, Height - 25.f), FVector(60.f, HalfY * 2.f, 50.f), Wood, FRotator::ZeroRotator, false);
	}

	const float WallT = 60.f;
	// North (+Y) wall with tall windows letting the sun in.
	{
		const float Y = HalfY + WallT * 0.5f;
		const int32 Windows = 5;
		const float WindowW = 260.f;
		const float Spacing = (HalfX * 2.f) / Windows;
		for (int32 Index = 0; Index <= Windows; ++Index)
		{
			// Pier between windows.
			const float Left = (Index == 0) ? -HalfX - WallT : -HalfX + Index * Spacing - Spacing * 0.5f + WindowW * 0.5f;
			const float Right = (Index == Windows) ? HalfX + WallT : -HalfX + Index * Spacing + Spacing * 0.5f - WindowW * 0.5f;
			if (Right > Left)
			{
				AddBox(FVector((Left + Right) * 0.5f, Y, Height * 0.5f), FVector(Right - Left, WallT, Height), Wall);
			}
			if (Index < Windows)
			{
				const float CX = -HalfX + Index * Spacing + Spacing * 0.5f;
				AddBox(FVector(CX, Y, 60.f), FVector(WindowW, WallT, 120.f), Wall);                 // sill
				AddBox(FVector(CX, Y, Height - 110.f), FVector(WindowW, WallT, 220.f), Wall);     // lintel
				AddBox(FVector(CX, Y - 10.f, 500.f), FVector(12.f, 20.f, 660.f), Wood, FRotator::ZeroRotator, false); // mullion
				AddBox(FVector(CX, Y - 10.f, 520.f), FVector(WindowW, 20.f, 12.f), Wood, FRotator::ZeroRotator, false);
				// Torn curtain on one side.
				AddBox(FVector(CX - WindowW * 0.5f - 40.f, Y - 50.f, 470.f), FVector(70.f, 12.f, 720.f), Velvet, FRotator(0.f, 0.f, Index % 2 ? 2.f : -1.5f), false);
				// Cold sky fill through the window.
				AddRectLight(FVector(CX, Y - 60.f, 500.f), FRotator(0.f, -90.f, 0.f), FLinearColor(0.65f, 0.75f, 1.f), 60.f, WindowW, 600.f, 2600.f);
			}
		}
	}
	// South, east and west walls.
	AddBox(FVector(0.f, -HalfY - WallT * 0.5f, Height * 0.5f), FVector(HalfX * 2.f + WallT * 2.f, WallT, Height), Wall);
	AddBox(FVector(HalfX + WallT * 0.5f, 0.f, Height * 0.5f), FVector(WallT, HalfY * 2.f, Height), Wall);
	AddBox(FVector(-HalfX - WallT * 0.5f, 0.f, Height * 0.5f), FVector(WallT, HalfY * 2.f, Height), Wall);
	// Wainscoting.
	AddBox(FVector(0.f, -HalfY + 8.f, 65.f), FVector(HalfX * 2.f, 16.f, 130.f), Wood, FRotator::ZeroRotator, false);
	AddBox(FVector(HalfX - 8.f, 0.f, 65.f), FVector(16.f, HalfY * 2.f, 130.f), Wood, FRotator::ZeroRotator, false);
	AddBox(FVector(-HalfX + 8.f, 0.f, 65.f), FVector(16.f, HalfY * 2.f, 130.f), Wood, FRotator::ZeroRotator, false);

	// ---------------- Columns ----------------
	UStaticMesh* ColumnMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Hotel/SM_ColumnRuined"));
	for (float X = -1200.f; X <= 1200.f; X += 800.f)
	{
		for (float Y : { -850.f, 850.f })
		{
			if (ColumnMesh)
			{
				const float Scale = Height / FMath::Max(1.f, ColumnMesh->GetBoundingBox().GetSize().Z);
				AddMesh(ColumnMesh, FTransform(FRotator(0.f, X * 0.1f, 0.f), FVector(X, Y, 0.f), FVector(Scale)));
			}
			else
			{
				AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(X, Y, Height * 0.5f), FVector(0.8f, 0.8f, Height / 100.f)), Plaster);
				AddBox(FVector(X, Y, 25.f), FVector(110.f, 110.f, 50.f), Wood);
				AddBox(FVector(X, Y, Height - 40.f), FVector(110.f, 110.f, 60.f), Plaster);
			}
		}
	}

	// ---------------- Reception (west end) ----------------
	PlaceProp(AIBAssets::Paths::Env + TEXT("Hotel/SM_ReceptionDesk"), FVector(-HalfX + 420.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f), 400.f, Wood, FVector(120.f, 420.f, 110.f));
	for (int32 Row = 0; Row < 4; ++Row)
	{
		for (int32 Col = 0; Col < 8; ++Col)
		{
			AddBox(FVector(-HalfX + 20.f, -280.f + Col * 80.f, 200.f + Row * 70.f), FVector(30.f, 70.f, 60.f), Wood, FRotator::ZeroRotator, false);
		}
	}
	AddText(FVector(-HalfX + 40.f, 0.f, 650.f), FRotator(0.f, 0.f, 0.f), LOCTEXT("HotelSign", "GRAND  MERIDIAN"), FLinearColor(0.8f, 0.55f, 0.2f), 110.f);
	AddText(FVector(-HalfX + 40.f, 0.f, 560.f), FRotator(0.f, 0.f, 0.f), LOCTEXT("HotelSub", "EST. 1931  -  CLOSED"), FLinearColor(0.5f, 0.35f, 0.15f), 36.f);
	{
		const FString Lamp = AIBAssets::Paths::Env + TEXT("Hotel/SM_VintageOilLamp");
		PlaceProp(Lamp, FVector(-HalfX + 420.f, -150.f, 112.f), FRotator::ZeroRotator, 45.f, nullptr, FVector::ZeroVector);
		UPointLightComponent* DeskLight = AddPointLight(FVector(-HalfX + 420.f, -150.f, 170.f), FLinearColor(1.f, 0.6f, 0.25f), 600.f, 900.f, true);
		FlickerLights.Add(DeskLight);
		FlickerBase.Add(600.f);
	}

	// ---------------- Grand staircase (east end, collapsed half way) ----------------
	for (int32 Step = 0; Step < 14; ++Step)
	{
		const float Depth = 45.f;
		const float Rise = 28.f;
		if (Step > 9 && Step % 2 == 0)
		{
			continue; // broken steps
		}
		AddBox(FVector(HalfX - 120.f - Step * Depth, 0.f, Rise * (Step + 0.5f)), FVector(Depth, 900.f, Rise), Wood);
	}
	AddBox(FVector(HalfX - 120.f - 14 * 45.f - 150.f, 0.f, 14 * 28.f - 20.f), FVector(300.f, 900.f, 40.f), Velvet, FRotator(0.f, 0.f, 4.f));

	// Red carpet runner from the doors to the reception.
	AddBox(FVector(0.f, 0.f, 1.f), FVector(HalfX * 2.f - 800.f, 300.f, 2.f), Velvet, FRotator::ZeroRotator, false);

	// ---------------- Chandeliers ----------------
	const FString ChandelierPaths[] = { AIBAssets::Paths::Env + TEXT("Hotel/SM_Chandelier_01"), AIBAssets::Paths::Env + TEXT("Hotel/SM_Chandelier_02") };
	const FVector ChandelierSpots[] = { FVector(-700.f, 0.f, Height - 330.f), FVector(700.f, 0.f, Height - 330.f) };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UStaticMesh* Mesh = AIBAssets::Mesh(ChandelierPaths[Index]);
		if (Mesh)
		{
			// Hang from the top of the bounds (pivots differ between the two models).
			const FBox Bounds = Mesh->GetBoundingBox();
			const float Scale = 200.f / FMath::Max(1.f, FMath::Max(Bounds.GetSize().X, Bounds.GetSize().Y));
			const FVector Spot = ChandelierSpots[Index] + FVector(0.f, 0.f, 120.f - Bounds.Max.Z * Scale);
			AddMesh(Mesh, FTransform(FRotator::ZeroRotator, Spot, FVector(Scale)), nullptr, false);
		}
		else
		{
			AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, ChandelierSpots[Index], FVector(1.6f, 1.6f, 0.25f)), Brass, false);
		}
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, ChandelierSpots[Index] + FVector(0.f, 0.f, 200.f), FVector(0.03f, 0.03f, 3.2f)), Brass, false);
		UPointLightComponent* Light = AddPointLight(ChandelierSpots[Index] - FVector(0.f, 0.f, 60.f), FLinearColor(1.f, 0.62f, 0.3f), 2200.f, 2400.f, true);
		FlickerLights.Add(Light);
		FlickerBase.Add(2200.f);
	}
	// The fallen one, shattered in the middle of the floor.
	if (UStaticMesh* Fallen = AIBAssets::Mesh(ChandelierPaths[0]))
	{
		const FVector Size = Fallen->GetBoundingBox().GetSize();
		const float Scale = 200.f / FMath::Max(1.f, FMath::Max(Size.X, Size.Y));
		AddMesh(Fallen, FTransform(FRotator(62.f, 20.f, 15.f), FVector(150.f, -560.f, 70.f), FVector(Scale)));
	}

	// ---------------- Lounges ----------------
	const FString Env = AIBAssets::Paths::Env + TEXT("Hotel/");
	struct FPropSpot { const TCHAR* Name; FVector Loc; float Yaw; float Size; FVector Fallback; bool bToppled; };
	const FPropSpot Props[] =
	{
		{ TEXT("SM_Sofa_01"), FVector(-900.f, 1050.f, 0.f), 180.f, 230.f, FVector(220.f, 90.f, 80.f), false },
		{ TEXT("SM_ArmChair_01"), FVector(-1250.f, 850.f, 0.f), 120.f, 100.f, FVector(90.f, 90.f, 90.f), false },
		{ TEXT("SM_CoffeeTableRound_01"), FVector(-950.f, 800.f, 0.f), 0.f, 110.f, FVector(110.f, 110.f, 45.f), false },
		{ TEXT("SM_Sofa_02"), FVector(900.f, -1050.f, 0.f), 0.f, 230.f, FVector(220.f, 90.f, 80.f), false },
		{ TEXT("SM_ArmChair_01"), FVector(1250.f, -850.f, 0.f), -60.f, 100.f, FVector(90.f, 90.f, 90.f), true },
		{ TEXT("SM_Ottoman_01"), FVector(1000.f, -780.f, 0.f), 30.f, 70.f, FVector(70.f, 70.f, 45.f), false },
		{ TEXT("SM_GreenChair_01"), FVector(-300.f, -1100.f, 0.f), 200.f, 95.f, FVector(80.f, 80.f, 95.f), true },
		{ TEXT("SM_ClassicConsole_01"), FVector(0.f, -1330.f, 0.f), 0.f, 160.f, FVector(160.f, 50.f, 90.f), false },
		{ TEXT("SM_GothicCabinet_01"), FVector(-1500.f, -1300.f, 0.f), 0.f, 210.f, FVector(120.f, 60.f, 210.f), false },
		{ TEXT("SM_Sofa_01"), FVector(1300.f, 1050.f, 0.f), 160.f, 230.f, FVector(220.f, 90.f, 80.f), true },
	};
	for (const FPropSpot& Spot : Props)
	{
		const FRotator Rot = Spot.bToppled ? FRotator(0.f, Spot.Yaw, 85.f) : FRotator(0.f, Spot.Yaw, 0.f);
		const FVector Loc = Spot.bToppled ? Spot.Loc + FVector(0.f, 0.f, 40.f) : Spot.Loc;
		PlaceProp(Env + Spot.Name, Loc, Rot, Spot.Size, Velvet, Spot.Fallback);
	}

	// Debris from the desert pack.
	const FString Debris[] =
	{
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rubbish_01"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rubbish_02"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Plank_03"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Plank_07"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Box_02"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rubbish_03")
	};
	FRandomStream Stream(4242);
	for (int32 Index = 0; Index < 26; ++Index)
	{
		UStaticMesh* Mesh = AIBAssets::Mesh(Debris[Index % UE_ARRAY_COUNT(Debris)]);
		if (!Mesh)
		{
			continue;
		}
		FVector Loc(Stream.FRandRange(-HalfX + 150.f, HalfX - 150.f), Stream.FRandRange(-HalfY + 150.f, HalfY - 150.f), 0.f);
		if (FMath::Abs(Loc.X) < 500.f && FMath::Abs(Loc.Y) < 500.f)
		{
			Loc.Y += Loc.Y >= 0.f ? 600.f : -600.f; // keep the meeting circle clear
		}
		AddMesh(Mesh, FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Loc, FVector(Stream.FRandRange(0.7f, 1.1f))), nullptr, false);
	}

	// Meeting circle: a faded brass compass rose inlaid in the marble.
	UMaterialInstanceDynamic* Inlay = AIBAssets::MakeEmissive(this, FLinearColor(0.9f, 0.6f, 0.25f), 0.8f);
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 2.f), FVector(8.f, 8.f, 0.01f)), Brass, false);
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 3.f), FVector(7.6f, 7.6f, 0.01f)), Floor, false);
	for (int32 Arm = 0; Arm < 4; ++Arm)
	{
		AddBox(FVector(0.f, 0.f, 4.f), FVector(760.f, 12.f, 2.f), Inlay, FRotator(0.f, Arm * 45.f, 0.f), false);
	}

	// Warm fill so faces are readable, plus a cool bounce from the windows.
	AddPointLight(FVector(0.f, 0.f, 420.f), FLinearColor(1.f, 0.75f, 0.5f), 900.f, 1800.f, false);
	AddPointLight(FVector(0.f, 900.f, 300.f), FLinearColor(0.6f, 0.7f, 1.f), 500.f, 1600.f, false);

	// Wander points around the hall.
	for (float X = -1300.f; X <= 1300.f; X += 650.f)
	{
		for (float Y = -900.f; Y <= 900.f; Y += 600.f)
		{
			WanderPoints.Add(GetActorLocation() + FVector(X, Y, 100.f));
		}
	}
}

void AAIBHotelArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsHidden())
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds();
	for (int32 Index = 0; Index < FlickerLights.Num(); ++Index)
	{
		if (UPointLightComponent* Light = FlickerLights[Index])
		{
			// Dying-bulb flicker: mostly steady with occasional dips.
			const float Noise = FMath::PerlinNoise1D(Time * 3.1f + Index * 17.f);
			const float Dip = Noise < -0.32f ? 0.25f : 1.f;
			Light->SetIntensity(FlickerBase[Index] * Dip * (0.9f + 0.1f * FMath::Sin(Time * 23.f + Index)) * AIB::LightScale);
		}
	}
}

#undef LOCTEXT_NAMESPACE
