// Alice in Botherland - where everything lives on the island map (cm, world space)
// Positions were chosen from a 5 m terrain scan of L_Island (Tools: terrain_scan + A* route); edit with care.

#pragma once

#include "CoreMinimal.h"

namespace AIBLayout
{
	/** Acidic sea surface height. The south-west basin floods into the landing cove. */
	constexpr float SeaLevel = -500.f;

	/** Landscape extents of the island map. */
	inline const FBox2D IslandBounds(FVector2D(-44100.f, -63290.f), FVector2D(44100.f, 43826.f));

	/** Beach on the north shore of the cove (2.5 m above the sea). Champions disembark here. */
	inline const FVector2D LandingBeach(-31500.f, -36200.f);
	/** Champions face inland (towards the town and the Guardian's beam). */
	constexpr float LandingYaw = 42.f;

	/** The pier starts on the beach and runs into deep water. */
	inline const FVector2D PierStart(-31500.f, -36200.f);
	constexpr float PierYaw = -130.f;
	constexpr float PierLength = 4200.f;

	/** Moorings in the cove (6-27 m deep), bows towards the beach. */
	inline const FVector2D FleetMoorings[5] =
	{
		FVector2D(-34500.0f, -41200.0f),
		FVector2D(-38000.0f, -38700.0f),
		FVector2D(-30500.0f, -43200.0f),
		FVector2D(-38000.0f, -43700.0f),
		FVector2D(-34000.0f, -45700.0f)
	};
	constexpr float FleetDockYaw = 50.f;
	/**
	 * The cove is a lagoon closed off by a sand bar; its only passage to the open sea is a narrow channel (~25 m wide,
	 * 3+ m deep) at the south end. The fleet waits on the open sea to the south and sails in through it in single file.
	 */
	inline const FVector2D FleetOffshore(-32300.f, -100000.f);
	inline const FVector2D ChannelEntrance(-32750.f, -64500.f);
	inline const FVector2D ChannelExit(-32300.f, -59000.f);
	inline const FVector2D FleetDocked(-34500.f, -42500.f);

	/** Remote indoor arenas float far away and hide themselves outside their stage. */
	inline const FVector NumberArena(400000.f, 0.f, 60000.f);
	inline const FVector HotelArena(400000.f, 80000.f, 60000.f);

	/** Race: start just inland of the beach, Guardian on the flat high ground east of the town (route ~960 m). */
	inline const FVector2D RaceStart(-26500.0f, -32700.0f);
	inline const FVector2D Guardian(29500.0f, 18300.0f);
}
