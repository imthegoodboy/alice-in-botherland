// Alice in Botherland - baked race route (A* over the 5 m terrain scan: slope <= 21 deg, 7 m clear of rocks/buildings)

#pragma once

#include "CoreMinimal.h"

namespace AIBRacePath
{
	/** Control points (world XY, cm) from the start line to the Guardian switch. */
	inline const FVector2D Points[] =
	{
		FVector2D(-26500.0f, -32700.0f),
		FVector2D(-24500.0f, -31700.0f),
		FVector2D(-22500.0f, -31700.0f),
		FVector2D(-21000.0f, -30200.0f),
		FVector2D(-18500.0f, -30200.0f),
		FVector2D(-17500.0f, -30700.0f),
		FVector2D(-16500.0f, -31200.0f),
		FVector2D(-15500.0f, -31700.0f),
		FVector2D(-13500.0f, -32200.0f),
		FVector2D(-10500.0f, -32700.0f),
		FVector2D(-8000.0f, -30200.0f),
		FVector2D(-8000.0f, -26200.0f),
		FVector2D(-7000.0f, -25200.0f),
		FVector2D(-5000.0f, -22700.0f),
		FVector2D(-4500.0f, -21200.0f),
		FVector2D(-2000.0f, -18200.0f),
		FVector2D(2000.0f, -18200.0f),
		FVector2D(6000.0f, -18200.0f),
		FVector2D(7000.0f, -17200.0f),
		FVector2D(11000.0f, -17200.0f),
		FVector2D(13000.0f, -17200.0f),
		FVector2D(15500.0f, -16700.0f),
		FVector2D(16500.0f, -15700.0f),
		FVector2D(18000.0f, -15700.0f),
		FVector2D(19500.0f, -14200.0f),
		FVector2D(19500.0f, -12700.0f),
		FVector2D(20500.0f, -11700.0f),
		FVector2D(22000.0f, -9700.0f),
		FVector2D(22000.0f, -8200.0f),
		FVector2D(24500.0f, -5700.0f),
		FVector2D(25000.0f, -4200.0f),
		FVector2D(25500.0f, -2200.0f),
		FVector2D(26000.0f, 1300.0f),
		FVector2D(26000.0f, 5300.0f),
		FVector2D(26000.0f, 6800.0f),
		FVector2D(26500.0f, 10300.0f),
		FVector2D(26500.0f, 14300.0f),
		FVector2D(27500.0f, 15800.0f),
		FVector2D(29500.0f, 18300.0f)
	};
}
