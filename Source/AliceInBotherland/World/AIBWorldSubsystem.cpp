// Alice in Botherland - assembles and indexes the island's set pieces

#include "World/AIBWorldSubsystem.h"
#include "World/AIBLayout.h"
#include "World/AIBSetPiece.h"
#include "World/AIBOcean.h"
#include "World/AIBFleet.h"
#include "World/AIBLandingSite.h"
#include "World/AIBNumberArena.h"
#include "World/AIBHotelArena.h"
#include "World/AIBRaceCourse.h"
#include "World/AIBGuardianSite.h"
#include "World/AIBLoreStones.h"
#include "World/AIBIslandLife.h"
#include "World/AIBEnvironmentDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "AliceInBotherland.h"

bool UAIBWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

bool UAIBWorldSubsystem::IsIslandMap() const
{
	// The island map is the one with a landscape; any other map (e.g. a test map) gets no set pieces.
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		return true;
	}
	return false;
}

template <typename T>
T* UAIBWorldSubsystem::FindOrSpawn(TWeakObjectPtr<T>& Slot, const FTransform& Transform)
{
	if (Slot.IsValid())
	{
		return Slot.Get();
	}
	for (TActorIterator<T> It(GetWorld()); It; ++It)
	{
		Slot = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	T* Spawned = GetWorld()->SpawnActor<T>(T::StaticClass(), Transform, Params);
	Slot = Spawned;
	return Spawned;
}

void UAIBWorldSubsystem::EnsureWorldActors()
{
	if (bEnsured || !IsIslandMap())
	{
		return;
	}
	bEnsured = true;

	UWorld* World = GetWorld();
	auto Ground = [World](const FVector2D& XY)
	{
		return FVector(XY.X, XY.Y, AAIBSetPiece::GroundZ(World, XY, 0.f));
	};

	// Environment first so the others can register lights / zones with it.
	FindOrSpawn(Environment, FTransform::Identity);
	FindOrSpawn(Ocean, FTransform(FVector(0.f, 0.f, AIBLayout::SeaLevel)));
	FindOrSpawn(Landing, FTransform(FRotator(0.f, AIBLayout::LandingYaw, 0.f), Ground(AIBLayout::LandingBeach)));
	FindOrSpawn(Fleet, FTransform(FVector(AIBLayout::FleetDocked, AIBLayout::SeaLevel)));
	FindOrSpawn(NumberArena, FTransform(AIBLayout::NumberArena));
	FindOrSpawn(Hotel, FTransform(AIBLayout::HotelArena));
	FindOrSpawn(Guardian, FTransform(Ground(AIBLayout::Guardian)));
	FindOrSpawn(RaceCourse, FTransform(Ground(AIBLayout::RaceStart)));
	FindOrSpawn(LoreStones, FTransform::Identity);
	FindOrSpawn(IslandLife, FTransform::Identity);

	UE_LOG(LogAIB, Log, TEXT("World set pieces ready (net mode %d)."), (int32)World->GetNetMode());
	SetLocalZone(LocalZone);
}

void UAIBWorldSubsystem::SetLocalZone(EAIBSceneZone Zone)
{
	LocalZone = Zone;
	for (TActorIterator<AAIBSetPiece> It(GetWorld()); It; ++It)
	{
		It->OnZoneChanged(Zone);
	}
}
