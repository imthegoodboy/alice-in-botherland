// Alice in Botherland - the five community ships

#include "World/AIBFleet.h"
#include "World/AIBLayout.h"
#include "Core/AIBAssets.h"
#include "Game/AIBGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Algo/BinarySearch.h"

namespace
{
	struct FShipConfig
	{
		const TCHAR* Mesh;
		float Length;
		/** Rotates the imported mesh so its bow points along +X. */
		float YawOffset;
	};

	const FShipConfig ShipConfigs[AIB::MaxPlayers] =
	{
		{ TEXT("Ships/SM_Ship_Large01"), 3430.f, 0.f },
		{ TEXT("Ships/SM_Ship_Medium"), 2410.f, 0.f },
		{ TEXT("Ships/SM_Ship_Large02"), 3430.f, -90.f },
		{ TEXT("Ships/SM_Ship_Pinnace"), 3980.f, -90.f },
		{ TEXT("Ships/SM_Ship_Large01"), 3100.f, 0.f },
	};

	float EaseOut(float X, float Power) { return 1.f - FMath::Pow(1.f - FMath::Clamp(X, 0.f, 1.f), Power); }

	/** Place of each slot in the single-file column: the ship with the furthest mooring leads. */
	constexpr int32 ColumnPlace[AIB::MaxPlayers] = { 1, 0, 3, 2, 4 };
	/** Open water before the channel at the head of the column, and the gap between ships in it. */
	constexpr float LeadIn = 15000.f;
	constexpr float ColumnSpacing = 6000.f;
	/** Ships glide to a stop over this distance. */
	constexpr float BrakeDistance = 3000.f;

	/** Follows D one to one, then eases into Length instead of stopping dead. */
	float SoftStop(float D, float Length)
	{
		const float Start = Length - BrakeDistance;
		return D <= Start ? D : Start + BrakeDistance * (1.f - FMath::Exp(-(D - Start) / BrakeDistance));
	}
}

AAIBFleet::AAIBFleet()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

namespace
{
	FVector ApproachDir()
	{
		return FVector(AIBLayout::ChannelExit - AIBLayout::FleetOffshore, 0.f).GetSafeNormal();
	}
}

FVector AAIBFleet::GetDockedLocation(int32 Slot) const
{
	const FVector2D XY = AIBLayout::FleetMoorings[FMath::Clamp(Slot, 0, AIB::MaxPlayers - 1)];
	return FVector(XY, AIBLayout::SeaLevel);
}

FVector AAIBFleet::GetOffshoreLocation(int32 Slot) const
{
	// Loose arrowhead formation, perpendicular to the approach, with the column's leader at the tip.
	constexpr float PlaceOffset[AIB::MaxPlayers] = { 0.f, -1.f, 1.f, -2.f, 2.f };
	const FVector Dir = ApproachDir();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Dir);
	const float Offset = PlaceOffset[ColumnPlace[FMath::Clamp(Slot, 0, AIB::MaxPlayers - 1)]];
	return FVector(AIBLayout::FleetOffshore, AIBLayout::SeaLevel) + Right * Offset * 6500.f - Dir * FMath::Abs(Offset) * 5000.f;
}

float AAIBFleet::GetShipLength(int32 Slot) const
{
	return ShipLengths.IsValidIndex(Slot) ? ShipLengths[Slot] : 3000.f;
}

void AAIBFleet::Build()
{
	SetActorLocation(FVector::ZeroVector);
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		const FShipConfig& Config = ShipConfigs[Slot];
		USceneComponent* ShipRoot = MakeScene(*FString::Printf(TEXT("Ship_%d"), Slot), FTransform::Identity);
		ShipRoots.Add(ShipRoot);

		UStaticMesh* Mesh = AIBAssets::Mesh(AIBAssets::Paths::Env + Config.Mesh);
		float Length = Config.Length;
		if (Mesh)
		{
			const FBox Bounds = Mesh->GetBoundingBox();
			const FVector Size = Bounds.GetSize();
			const float MeshLength = FMath::Max(Size.X, Size.Y);
			const float Scale = MeshLength > 1.f ? Config.Length / MeshLength : 1.f;
			// The ship meshes have their origin at the waterline.
			const float Lift = 30.f;
			UStaticMeshComponent* Hull = AddMesh(Mesh, FTransform(FRotator(0.f, Config.YawOffset, 0.f), FVector(0.f, 0.f, Lift), FVector(Scale)), nullptr, false, ShipRoot);
			if (Hull)
			{
				Hull->SetCastShadow(true);
			}
		}
		else
		{
			// Placeholder hull until the ship meshes are imported.
			UMaterialInstanceDynamic* Wood = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
			AIBAssets::SetColor(Wood, FLinearColor(0.12f, 0.07f, 0.04f));
			AddBox(FVector(0.f, 0.f, 150.f), FVector(Length, Length * 0.25f, 500.f), Wood, FRotator::ZeroRotator, false, ShipRoot);
			AddBox(FVector(0.f, 0.f, 1400.f), FVector(60.f, 60.f, 2200.f), Wood, FRotator::ZeroRotator, false, ShipRoot);
		}
		ShipLengths.Add(Length);

		// Community banner + lantern so each ship reads at a glance.
		const FLinearColor Color = AIB::Community(Slot).Color;
		UMaterialInstanceDynamic* Glow = AIBAssets::MakeEmissive(this, Color, 6.f);
		AddBox(FVector(-Length * 0.42f, 0.f, 900.f), FVector(10.f, 240.f, 160.f), Glow, FRotator::ZeroRotator, false, ShipRoot);
		UPointLightComponent* Lantern = AddPointLight(FVector(-Length * 0.40f, 0.f, 650.f), Color, 900.f, 3500.f, false, ShipRoot);
		Lanterns.Add(Lantern);
		AddPointLight(FVector(Length * 0.42f, 0.f, 420.f), FLinearColor(1.f, 0.72f, 0.4f), 500.f, 2200.f, false, ShipRoot);
	}
	BuildRoutes();
}

void AAIBFleet::BuildRoutes()
{
	const FVector Entrance(AIBLayout::ChannelEntrance, AIBLayout::SeaLevel);
	const FVector Exit(AIBLayout::ChannelExit, AIBLayout::SeaLevel);
	const FVector Along = (Exit - Entrance).GetSafeNormal();
	const float ChannelLength = FVector::Dist(Entrance, Exit);

	Routes.SetNum(AIB::MaxPlayers);
	TArray<float> Inside;
	float MaxLength = 0.f;
	float MaxInside = 0.f;
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		FRoute& Route = Routes[Slot];
		Route.Points.Reset();
		// Straight in through the channel; the column's gaps come from the staggered starting points.
		const FVector Start = Entrance - Along * (LeadIn + ColumnPlace[Slot] * ColumnSpacing);
		for (int32 Step = 0; Step < 40; ++Step)
		{
			Route.Points.Add(FMath::Lerp(Start, Exit, Step / 40.f));
		}
		// Out of the channel, a curve that keeps the ship's heading and ends at its mooring.
		const FVector Mooring = GetDockedLocation(Slot);
		const FVector Control = Exit + Along * 6000.f;
		for (int32 Step = 0; Step <= 60; ++Step)
		{
			const float U = Step / 60.f;
			const float V = 1.f - U;
			Route.Points.Add(V * V * Exit + 2.f * V * U * Control + U * U * Mooring);
		}
		Route.Distance.SetNum(Route.Points.Num());
		Route.Distance[0] = 0.f;
		for (int32 Index = 1; Index < Route.Points.Num(); ++Index)
		{
			Route.Distance[Index] = Route.Distance[Index - 1] + FVector::Dist(Route.Points[Index - 1], Route.Points[Index]);
		}
		MaxLength = FMath::Max(MaxLength, Route.Length());
		// Distance from the channel exit to the mooring.
		Inside.Add(Route.Length() - (LeadIn + ColumnPlace[Slot] * ColumnSpacing + ChannelLength));
		MaxInside = FMath::Max(MaxInside, Inside.Last());
	}
	// Long enough that every ship has finished braking by the end of the arrival.
	ConvoyLength = MaxLength + BrakeDistance * 5.f;

	// Leaving: the last ship in goes first, and each one reaches the channel a column gap after the one before.
	DepartDelay.SetNum(AIB::MaxPlayers);
	DepartLength = 0.f;
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		DepartDelay[Slot] = MaxInside + (AIB::MaxPlayers - 1 - ColumnPlace[Slot]) * ColumnSpacing - Inside[Slot];
		DepartLength = FMath::Max(DepartLength, DepartDelay[Slot] + Inside[Slot] + ChannelLength + 20000.f);
	}
}

FVector AAIBFleet::SampleRoute(int32 Slot, float S, FVector& OutDir) const
{
	const FRoute& Route = Routes[Slot];
	const int32 Num = Route.Points.Num();
	if (Num < 2)
	{
		OutDir = FRotator(0.f, AIBLayout::FleetDockYaw, 0.f).Vector();
		return GetDockedLocation(Slot);
	}
	if (S <= 0.f)
	{
		// Before the start (or sailing away past it): straight on out to sea.
		OutDir = (Route.Points[1] - Route.Points[0]).GetSafeNormal();
		return Route.Points[0] + OutDir * S;
	}
	S = FMath::Min(S, Route.Length());
	const int32 Index = FMath::Clamp(Algo::UpperBound(Route.Distance, S) - 1, 0, Num - 2);
	const float Span = FMath::Max(Route.Distance[Index + 1] - Route.Distance[Index], 1.f);
	OutDir = (Route.Points[Index + 1] - Route.Points[Index]).GetSafeNormal();
	return FMath::Lerp(Route.Points[Index], Route.Points[Index + 1], (S - Route.Distance[Index]) / Span);
}

int32 AAIBFleet::GetLeadShip() const
{
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		if (ColumnPlace[Slot] == 0)
		{
			return Slot;
		}
	}
	return 0;
}

float AAIBFleet::RouteDistance(int32 Slot, const AAIBGameState* GS, bool& bDeparting) const
{
	bDeparting = false;
	const float Length = Routes.IsValidIndex(Slot) ? Routes[Slot].Length() : 0.f;
	switch (GS->Phase)
	{
	case EAIBPhase::ArrivalCinematic:
	{
		// Every ship covers the same convoy distance, so the column keeps its gaps through the channel; each one
		// then eases to a stop at its own mooring.
		const float T = GS->GetPhaseElapsed() / AIB::ArrivalCinematic;
		return SoftStop(ConvoyLength * EaseOut(T, 1.8f), Length);
	}
	case EAIBPhase::EndingCinematic:
	case EAIBPhase::Results:
	{
		const bool bStays = GS->Ending == EAIBEnding::SacrificeThenKing && GS->ChampionSlot == Slot;
		if (bStays || GS->Ending == EAIBEnding::None)
		{
			return Length;
		}
		bDeparting = true;
		const float A = GS->Phase == EAIBPhase::Results ? 1.f : FMath::Clamp((GS->GetPhaseElapsed() - 3.f) / (AIB::EndingCinematic - 3.f), 0.f, 1.f);
		const float Back = DepartLength * FMath::Pow(A, 1.4f) - (DepartDelay.IsValidIndex(Slot) ? DepartDelay[Slot] : 0.f);
		return Length - FMath::Max(0.f, Back);
	}
	default:
		return Length;
	}
}

FTransform AAIBFleet::GetShipTransform(int32 Slot) const
{
	return ShipRoots.IsValidIndex(Slot) && ShipRoots[Slot] ? ShipRoots[Slot]->GetComponentTransform() : FTransform(GetDockedLocation(Slot));
}

void AAIBFleet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	const AAIBGameState* GS = World ? World->GetGameState<AAIBGameState>() : nullptr;
	const float Time = World ? World->GetTimeSeconds() : 0.f;

	for (int32 Slot = 0; Slot < ShipRoots.Num(); ++Slot)
	{
		USceneComponent* Ship = ShipRoots[Slot];
		if (!Ship)
		{
			continue;
		}
		const FVector Docked = FRotator(0.f, AIBLayout::FleetDockYaw, 0.f).Vector();
		FVector Location;
		FVector Forward;
		const bool bWaiting = !GS || GS->Phase == EAIBPhase::None || GS->Phase == EAIBPhase::Lobby || GS->Phase == EAIBPhase::LobbyCountdown;
		if (bWaiting || !Routes.IsValidIndex(Slot))
		{
			// In formation on the open sea, bows towards the island.
			Location = GetOffshoreLocation(Slot);
			Forward = ApproachDir();
		}
		else
		{
			bool bDeparting = false;
			const float S = RouteDistance(Slot, GS, bDeparting);
			FVector Dir;
			Location = SampleRoute(Slot, S, Dir);
			const float FromMooring = Routes[Slot].Length() - S;
			Forward = bDeparting
				// Turn about over the first 30 m before heading back out.
				? FMath::Lerp(Docked, -Dir, FMath::SmoothStep(0.f, 3000.f, FromMooring)).GetSafeNormal()
				// Swing round to the mooring heading over the last 25 m.
				: FMath::Lerp(Dir, Docked, 1.f - FMath::SmoothStep(0.f, 2500.f, FromMooring)).GetSafeNormal();
			if (Forward.IsNearlyZero())
			{
				Forward = Docked;
			}
		}

		// Swell.
		const float Phase = Slot * 1.7f;
		Location.Z = AIBLayout::SeaLevel + FMath::Sin(Time * 0.55f + Phase) * 45.f;
		const float Roll = FMath::Sin(Time * 0.42f + Phase * 1.3f) * 2.2f;
		const float Pitch = FMath::Sin(Time * 0.37f + Phase * 0.7f) * 1.3f;
		const FRotator Rotation(Pitch, Forward.Rotation().Yaw, Roll);
		Ship->SetWorldLocationAndRotation(Location, Rotation);

		if (Lanterns.IsValidIndex(Slot) && Lanterns[Slot])
		{
			Lanterns[Slot]->SetIntensity((800.f + FMath::Sin(Time * 6.f + Phase) * 120.f) * AIB::LightScale);
		}
	}
}

FTransform AAIBFleet::GetLobbyCamera(float Time) const
{
	// Beside the waiting fleet, looking along the approach towards the island on the horizon.
	const FVector Dir = ApproachDir();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Dir);
	const FVector Center = FVector(AIBLayout::FleetOffshore, AIBLayout::SeaLevel);
	// Far enough back to hold the whole formation (the arrowhead is ~260 m wide) with the island beyond it.
	const float Drift = FMath::Sin(Time * 0.05f) * 2500.f;
	const FVector CamLoc = Center - Dir * 17000.f + Right * (6000.f + Drift) + FVector(0.f, 0.f, 1900.f + FMath::Sin(Time * 0.11f) * 150.f);
	const FVector LookAt = Center + Dir * 6000.f + Right * Drift * 0.3f + FVector(0.f, 0.f, 600.f);
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBFleet::GetArrivalCamera(float Time, float& OutFOV) const
{
	OutFOV = 60.f;
	const FTransform LeadShip = GetShipTransform(GetLeadShip());
	const FVector Lead = LeadShip.GetLocation();
	const FVector Fwd = LeadShip.GetRotation().GetForwardVector().GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd);
	if (Time < 6.f)
	{
		// Tracking shot behind the head of the column as it threads the channel, the island ahead. Kept above the
		// dunes either side of the channel.
		const float A = Time / 6.f;
		FVector CamLoc = Lead - Fwd * (5200.f - A * 1200.f) + Side * (1100.f + A * 400.f) + FVector(0.f, 0.f, 1500.f + A * 200.f);
		CamLoc.Z = FMath::Max(CamLoc.Z, GroundZ(GetWorld(), FVector2D(CamLoc), AIBLayout::SeaLevel) + 600.f);
		const FVector LookAt = Lead + Fwd * 12000.f + FVector(0.f, 0.f, 700.f);
		OutFOV = 55.f;
		return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
	}
	if (Time < 12.f)
	{
		// High establishing shot from the open sea: the column fanning out across the cove, the island behind with
		// its ruins, roads and the Guardian's beam.
		const float A = (Time - 6.f) / 6.f;
		const FVector CamLoc = FVector(-26000.f + A * 3000.f, -96000.f + A * 10000.f, 13000.f - A * 2500.f);
		const FVector LookAt = FVector(-12000.f, -12000.f, -1500.f);
		OutFOV = 50.f;
		return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
	}
	if (Time < 16.5f)
	{
		// Low over the open water on the east side of the cove (clear of every ship's route): the ships glide to
		// their moorings with the beach and landing site behind them.
		const float A = (Time - 12.f) / 4.5f;
		const FVector CamLoc(-25500.f + A * 800.f, -50500.f + A * 1500.f, AIBLayout::SeaLevel + 380.f + A * 120.f);
		const FVector LookAt(-34500.f, -41000.f, AIBLayout::SeaLevel + 700.f);
		OutFOV = 48.f - A * 5.f;
		return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
	}
	// Over the beach, the fleet moored behind the landing site.
	const float A = (Time - 16.5f) / 3.5f;
	const FVector Inland = FRotator(0.f, AIBLayout::LandingYaw, 0.f).Vector();
	const FVector Beach(AIBLayout::LandingBeach, 0.f);
	const FVector CamLoc = Beach + Inland * (2600.f - A * 500.f) + FVector(0.f, 0.f, 800.f + A * 100.f);
	const FVector LookAt = FVector(AIBLayout::FleetDocked, AIBLayout::SeaLevel + 800.f);
	OutFOV = 55.f;
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}
