// Alice in Botherland - the Guardian: race finish, final chamber and ending stage

#include "World/AIBGuardianSite.h"
#include "World/AIBLayout.h"
#include "World/AIBRacePath.h"
#include "Core/AIBAssets.h"
#include "Game/AIBGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AAIBGuardianSite::AAIBGuardianSite()
{
	PrimaryActorTick.bCanEverTick = true;
}

FVector AAIBGuardianSite::GetSwitchLocation() const
{
	return Local(FVector(1150.f, 0.f, 0.f));
}

FTransform AAIBGuardianSite::GetFinalStand(int32 Index) const
{
	const FVector Loc = Local(FVector(260.f, Index == 0 ? -230.f : 230.f, AltarTop + 100.f));
	return FTransform(GetActorRotation() + FRotator(0.f, Index == 0 ? 170.f : 190.f, 0.f), Loc);
}

FTransform AAIBGuardianSite::GetThroneSeat() const
{
	return FTransform(GetActorRotation(), Local(FVector(-260.f, 0.f, AltarTop + 100.f)));
}

FTransform AAIBGuardianSite::GetFinalWideCamera(float Time) const
{
	// Off to the side of the switch beam (which stands between the road and the altar).
	const float Drift = FMath::Sin(Time * 0.15f) * 120.f;
	const FVector CamLoc = Local(FVector(1500.f, 700.f + Drift, 300.f));
	const FVector LookAt = Local(FVector(-200.f, 0.f, 420.f));
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBGuardianSite::GetChooserCamera(int32 Index, float Time) const
{
	const FTransform Stand = GetFinalStand(Index);
	const FVector Face = Stand.GetLocation() + FVector(0.f, 0.f, 65.f);
	const FVector CamLoc = Face + Stand.GetRotation().GetForwardVector() * (230.f - Time * 4.f) + GetActorRightVector() * (Index == 0 ? 60.f : -60.f) + FVector(0.f, 0.f, 10.f);
	return FTransform((Face - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBGuardianSite::GetEndingCamera(bool bKing, float Time, float& OutFOV) const
{
	OutFOV = 50.f;
	if (Time < 7.f)
	{
		// Slow orbit around the altar.
		const float Angle = FMath::DegreesToRadians(-35.f + Time * 9.f);
		const FVector CamLoc = Local(FVector(FMath::Cos(Angle) * 1300.f, FMath::Sin(Angle) * 1300.f, 380.f + Time * 25.f));
		const FVector LookAt = Local(FVector(-200.f, 0.f, bKing ? 260.f : 350.f));
		return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
	}
	if (Time < 12.f)
	{
		// Over the statue's shoulder towards the sea and the leaving ships.
		const float A = (Time - 7.f) / 5.f;
		const FVector CamLoc = Local(FVector(-1700.f - A * 300.f, 600.f, StatueHeight * 0.85f + A * 200.f));
		const FVector Sea(AIBLayout::FleetDocked.X, AIBLayout::FleetDocked.Y - 20000.f, AIBLayout::SeaLevel);
		OutFOV = 45.f;
		return FTransform((Sea - CamLoc).Rotation(), CamLoc);
	}
	// Rising crane shot over the island at dusk.
	const float A = FMath::Clamp((Time - 12.f) / 6.f, 0.f, 1.f);
	const FVector CamLoc = Local(FVector(2600.f + A * 1500.f, -1800.f, 600.f + A * 2600.f));
	const FVector LookAt = Local(FVector(-300.f, 0.f, 500.f));
	OutFOV = 55.f;
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBGuardianSite::GetArrivalCamera(float Time) const
{
	const FVector CamLoc = Local(FVector(-200.f, 900.f, 900.f + Time * 10.f));
	const FVector LookAt = Local(FVector(2600.f, 0.f, 100.f));
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

void AAIBGuardianSite::Build()
{
	// Face down the road the racers arrive on (its last ~40 m), so they drive straight up the ramp onto the plaza.
	FVector2D Approach = AIBLayout::RaceStart;
	for (int32 Index = UE_ARRAY_COUNT(AIBRacePath::Points) - 1; Index >= 0; --Index)
	{
		if (FVector2D::Distance(AIBRacePath::Points[Index], FVector2D(GetActorLocation())) >= 4000.f)
		{
			Approach = AIBRacePath::Points[Index];
			break;
		}
	}
	const FVector2D ToRoad = (Approach - FVector2D(GetActorLocation())).GetSafeNormal();
	SetActorRotation(FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(ToRoad.Y, ToRoad.X)), 0.f));

	UMaterialInterface* Stone = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_TempleStone"));
	if (!Stone)
	{
		UMaterialInstanceDynamic* Fallback = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Fallback, FLinearColor(0.3f, 0.27f, 0.23f));
		Stone = Fallback;
	}
	UMaterialInterface* DarkStone = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_DarkStone"), Stone);
	UMaterialInstanceDynamic* RuneGlow = AIBAssets::MakeEmissive(this, FLinearColor(0.45f, 1.f, 0.35f), 12.f);

	// Plaza levelling the terrain around the site.
	constexpr float PlazaRadius = 2400.f;
	constexpr float PlazaTop = -80.f;
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, PlazaTop - 100.f), FVector(PlazaRadius / 50.f, PlazaRadius / 50.f, 2.f)), Stone);

	// Ramp from the road up onto the plaza: the plaza is level but the hillside the racers climb is not, and the
	// plaza's edge would otherwise be a wall the cars cannot get over.
	{
		const float MaxGrade = FMath::Tan(FMath::DegreesToRadians(11.f));
		const float TopZ = GetActorLocation().Z + PlazaTop;
		float Length = 0.f;
		float Drop = 0.f;
		for (float L = 600.f; L <= 3600.f; L += 200.f)
		{
			const FVector Foot = Local(FVector(PlazaRadius + L, 0.f, 0.f));
			Length = L;
			Drop = TopZ - GroundZ(GetWorld(), FVector2D(Foot), TopZ);
			if (Drop <= L * MaxGrade)
			{
				break;
			}
		}
		if (Drop > 15.f)
		{
			// Starts just inside the plaza edge and runs 2.5 m past the foot so it sinks into the ground.
			const float Grade = Drop / Length;
			const float StartX = PlazaRadius - 50.f;
			const float EndX = PlazaRadius + Length + 250.f;
			const float MidX = (StartX + EndX) * 0.5f;
			const float SurfaceZ = PlazaTop - Grade * (MidX - PlazaRadius);
			const float Thickness = 80.f;
			const FRotator Slope(-FMath::RadiansToDegrees(FMath::Atan(Grade)), 0.f, 0.f);
			const FVector Center = FVector(MidX, 0.f, SurfaceZ) - Slope.RotateVector(FVector::UpVector) * (Thickness * 0.5f);
			const float SlopeLength = (EndX - StartX) * FMath::Sqrt(1.f + Grade * Grade);
			AddBox(Center, FVector(SlopeLength, 2200.f, Thickness), Stone, Slope);
		}
	}

	// ---------------- Altar + throne ----------------
	if (UStaticMesh* Altar = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Guardian/SM_Altar")))
	{
		const FVector Size = Altar->GetBoundingBox().GetSize();
		const float Scale = Size.X > 1.f ? 1000.f / Size.X : 1.f;
		UStaticMeshComponent* AltarComp = AddMesh(Altar, FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20.f), FVector(Scale)), Stone);
		if (AltarComp)
		{
			const int32 GlowIndex = AltarComp->GetMaterialIndex(TEXT("Glow"));
			if (GlowIndex != INDEX_NONE)
			{
				AltarComp->SetMaterial(GlowIndex, RuneGlow);
			}
		}
		AltarTop = 20.f + Size.Z * Scale;
	}
	else
	{
		for (int32 Step = 0; Step < 3; ++Step)
		{
			const float Radius = 500.f - Step * 110.f;
			AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20.f + Step * 30.f + 15.f), FVector(Radius / 50.f, Radius / 50.f, 0.3f)), Stone);
		}
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 111.f), FVector(5.2f, 5.2f, 0.01f)), RuneGlow, false);
		AltarTop = 110.f;
	}
	if (UStaticMesh* Throne = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Guardian/SM_Throne")))
	{
		const float Scale = 260.f / FMath::Max(1.f, Throne->GetBoundingBox().GetSize().Z);
		AddMesh(Throne, FTransform(FRotator(0.f, 90.f, 0.f), FVector(-330.f, 0.f, AltarTop), FVector(Scale)), Stone);
	}
	else
	{
		AddBox(FVector(-330.f, 0.f, AltarTop + 30.f), FVector(110.f, 150.f, 60.f), DarkStone);
		AddBox(FVector(-385.f, 0.f, AltarTop + 160.f), FVector(30.f, 150.f, 300.f), DarkStone);
		AddBox(FVector(-330.f, -80.f, AltarTop + 85.f), FVector(110.f, 20.f, 50.f), DarkStone);
		AddBox(FVector(-330.f, 80.f, AltarTop + 85.f), FVector(110.f, 20.f, 50.f), DarkStone);
	}

	// ---------------- The Guardian statue ----------------
	UStaticMesh* StatueMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Guardian/SM_GuardianStatue"));
	if (StatueMesh)
	{
		const FVector Size = StatueMesh->GetBoundingBox().GetSize();
		const float Scale = StatueHeight / FMath::Max(1.f, Size.Z);
		AddMesh(StatueMesh, FTransform(FRotator(0.f, 0.f, 0.f), FVector(-1500.f, 0.f, -StatueMesh->GetBoundingBox().Min.Z * Scale - 20.f), FVector(Scale)), Stone);
	}
	else
	{
		// Stylised hooded colossus out of primitives until the sculpted mesh is imported.
		AddBox(FVector(-1500.f, 0.f, 120.f), FVector(900.f, 900.f, 240.f), Stone);
		AddMesh(AIBAssets::Cone(), FTransform(FRotator::ZeroRotator, FVector(-1500.f, 0.f, 240.f + 550.f), FVector(6.5f, 5.5f, 11.f)), DarkStone);
		AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, FVector(-1500.f, 0.f, 1350.f), FVector(2.6f, 2.4f, 3.0f)), DarkStone);
		AddMesh(AIBAssets::Cone(), FTransform(FRotator::ZeroRotator, FVector(-1480.f, 0.f, 1470.f), FVector(3.2f, 3.0f, 2.6f)), DarkStone);
		AddBox(FVector(-1300.f, 0.f, 700.f), FVector(40.f, 40.f, 1300.f), Stone, FRotator(0.f, 0.f, 0.f));
		AddBox(FVector(-1300.f, 0.f, 1150.f), FVector(40.f, 260.f, 50.f), Stone);
	}
	EyeGlow = AIBAssets::MakeEmissive(this, FLinearColor(0.5f, 1.f, 0.35f), 40.f);
	for (int32 Eye = -1; Eye <= 1; Eye += 2)
	{
		AddBox(FVector(-1500.f + 230.f, Eye * 45.f, StatueHeight * 0.83f), FVector(6.f, 30.f, 8.f), EyeGlow, FRotator::ZeroRotator, false);
	}
	AddSpotLight(FVector(-700.f, 0.f, 80.f), FRotator(55.f, 180.f, 0.f), FLinearColor(0.55f, 0.9f, 0.6f), 40000.f, 3000.f, 30.f, true);

	// ---------------- Switch pedestal ----------------
	const FVector SwitchLocal(1150.f, 0.f, 0.f);
	SwitchGlow = AIBAssets::MakeEmissive(this, FLinearColor(0.45f, 1.f, 0.35f), 10.f);
	if (UStaticMesh* Pedestal = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Guardian/SM_SwitchPedestal")))
	{
		const float Scale = 140.f / FMath::Max(1.f, Pedestal->GetBoundingBox().GetSize().Z);
		UStaticMeshComponent* Comp = AddMesh(Pedestal, FTransform(FRotator(0.f, 180.f, 0.f), SwitchLocal, FVector(Scale)), Stone);
		if (Comp)
		{
			const int32 GlowIndex = Comp->GetMaterialIndex(TEXT("Glow"));
			if (GlowIndex != INDEX_NONE)
			{
				Comp->SetMaterial(GlowIndex, SwitchGlow);
			}
		}
	}
	else
	{
		AddBox(SwitchLocal + FVector(0.f, 0.f, 60.f), FVector(70.f, 70.f, 120.f), DarkStone);
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator(-30.f, 0.f, 0.f), SwitchLocal + FVector(0.f, 0.f, 125.f), FVector(0.5f, 0.5f, 0.06f)), SwitchGlow, false);
	}
	// Ring marking the activation area.
	for (int32 Seg = 0; Seg < 40; ++Seg)
	{
		const float A = Seg * 9.f;
		const FVector P = SwitchLocal + FRotator(0.f, A, 0.f).Vector() * SwitchRadius;
		AddBox(P + FVector(0.f, 0.f, -150.f + 154.f), FVector(10.f, 90.f, 4.f), SwitchGlow, FRotator(0.f, A, 0.f), false);
	}
	SwitchLight = AddPointLight(SwitchLocal + FVector(0.f, 0.f, 220.f), FLinearColor(0.45f, 1.f, 0.35f), 4000.f, 2500.f, false);

	// Beam of light into the sky: visible from anywhere on the island.
	BeamMaterial = AIBAssets::MakeBeam(this, FLinearColor(0.45f, 1.f, 0.4f), 8.f);
	UStaticMesh* BeamMesh = AIBAssets::Mesh(AIBAssets::Paths::VFX + TEXT("SM_BeamCylinder"));
	if (BeamMesh)
	{
		Beam = AddMesh(BeamMesh, FTransform(FRotator::ZeroRotator, SwitchLocal, FVector(1.6f, 1.6f, 900.f)), BeamMaterial, false);
	}
	else
	{
		Beam = AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, SwitchLocal + FVector(0.f, 0.f, 45000.f), FVector(1.2f, 1.2f, 900.f)), BeamMaterial, false);
	}
	if (Beam)
	{
		Beam->SetCastShadow(false);
	}

	// Braziers at the altar corners.
	UMaterialInstanceDynamic* Fire = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.45f, 0.1f), 50.f);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector P = FRotator(0.f, 45.f + Index * 90.f, 0.f).Vector() * 620.f;
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, 70.f), FVector(0.5f, 0.5f, 1.4f)), DarkStone);
		AddMesh(AIBAssets::Sphere(), FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, 150.f), FVector(0.5f, 0.5f, 0.35f)), Fire, false);
		Braziers.Add(AddPointLight(P + FVector(0.f, 0.f, 230.f), FLinearColor(1.f, 0.5f, 0.18f), 3000.f, 2200.f, Index < 2));
	}

	// Standing stones.
	const FString RockPaths[] =
	{
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rock_01"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rock_03"),
		TEXT("/Game/JustBStudios/PostApo/Meshes/SM_Rock_05")
	};
	for (int32 Index = 0; Index < 9; ++Index)
	{
		const float A = 200.f + Index * 18.f; // arc behind the statue, opening towards the road
		const FVector P = FRotator(0.f, A, 0.f).Vector() * 2600.f;
		if (UStaticMesh* Rock = AIBAssets::Mesh(RockPaths[Index % 3]))
		{
			const float Scale = 600.f / FMath::Max(1.f, Rock->GetBoundingBox().GetSize().Z);
			AddMesh(Rock, FTransform(FRotator(0.f, A * 3.f, 0.f), P, FVector(Scale * 0.7f, Scale * 0.7f, Scale)), Stone);
		}
		else
		{
			AddBox(P + FVector(0.f, 0.f, 300.f), FVector(160.f, 120.f, 600.f), Stone, FRotator(0.f, A, Index % 2 ? 4.f : -3.f));
		}
	}
}

void AAIBGuardianSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UWorld* World = GetWorld();
	const float Time = World->GetTimeSeconds();
	const AAIBGameState* GS = World->GetGameState<AAIBGameState>();

	for (int32 Index = 0; Index < Braziers.Num(); ++Index)
	{
		const float Flicker = 0.75f + 0.25f * FMath::PerlinNoise1D(Time * 4.f + Index * 9.f) + 0.08f * FMath::Sin(Time * 31.f + Index);
		Braziers[Index]->SetIntensity(3000.f * Flicker * AIB::LightScale);
	}

	float Progress = 0.f;
	bool bClaimed = false;
	if (GS)
	{
		for (float P : GS->SwitchProgress)
		{
			Progress = FMath::Max(Progress, P);
		}
		bClaimed = GS->RaceWinnerSlot != INDEX_NONE;
	}
	const FLinearColor Base(0.45f, 1.f, 0.35f);
	const FLinearColor Claimed(1.f, 0.85f, 0.4f);
	const FLinearColor Color = bClaimed ? Claimed : Base;
	if (SwitchGlow)
	{
		AIBAssets::SetColor(SwitchGlow, Color);
		SwitchGlow->SetScalarParameterValue(TEXT("Intensity"), 10.f + Progress * 60.f + (bClaimed ? 30.f : 0.f));
	}
	if (SwitchLight)
	{
		SwitchLight->SetLightColor(Color);
		SwitchLight->SetIntensity((4000.f + Progress * 20000.f) * AIB::LightScale);
	}
	if (BeamMaterial)
	{
		// A bright beacon until the race is decided, then a soft glow: the finale cameras stand right next to it and
		// a full-strength beam washes the whole frame out to white.
		const bool bBeacon = !GS || GS->Phase <= EAIBPhase::Stage3Race;
		AIBAssets::SetColor(BeamMaterial, Color);
		BeamMaterial->SetScalarParameterValue(TEXT("Intensity"), bBeacon ? 8.f + Progress * 6.f + 2.f * FMath::Sin(Time * 1.3f) : 1.2f + 0.3f * FMath::Sin(Time * 1.3f));
	}
	if (Beam)
	{
		// The beacon goes out once the Guardian's choice has been made.
		const bool bResolved = GS && GS->Phase >= EAIBPhase::EndingCinematic;
		Beam->SetVisibility(!bResolved);
	}
	if (EyeGlow)
	{
		const bool bAwake = GS && (GS->Phase >= EAIBPhase::Stage3Finish && GS->Phase <= EAIBPhase::EndingCinematic);
		EyeGlow->SetScalarParameterValue(TEXT("Intensity"), bAwake ? 120.f + 40.f * FMath::Sin(Time * 2.f) : 15.f);
	}
}
