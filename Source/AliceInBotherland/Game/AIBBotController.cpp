// Alice in Botherland - AI champion (fills empty seats)

#include "Game/AIBBotController.h"
#include "Game/AIBPlayerState.h"
#include "Game/AIBGameState.h"
#include "Player/AIBCharacter.h"
#include "Player/AIBVehicle.h"
#include "World/AIBWorldSubsystem.h"
#include "World/AIBRaceCourse.h"
#include "World/AIBGuardianSite.h"
#include "Engine/World.h"

AAIBBotController::AAIBBotController()
{
	bWantsPlayerState = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AAIBBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	PathIndex = 0;
	StuckTimer = 0.f;
	ReverseTimer = 0.f;
	bHasTarget = false;
}

void AAIBBotController::SetWanderPoints(const TArray<FVector>& Points, const FBox& Bounds)
{
	WanderPoints = Points;
	WanderBounds = Bounds;
	bHasTarget = false;
	WanderPause = FMath::FRandRange(0.5f, 2.5f);
}

void AAIBBotController::ClearWander()
{
	WanderPoints.Reset();
	bHasTarget = false;
}

void AAIBBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Ghosts hold a disconnected human's seat: they never act on their behalf.
	const AAIBPlayerState* PS = GetPlayerState<AAIBPlayerState>();
	if (!PS || PS->bDisconnected)
	{
		return;
	}
	if (AAIBVehicle* Vehicle = Cast<AAIBVehicle>(GetPawn()))
	{
		TickDrive(Vehicle, DeltaSeconds);
	}
	else if (AAIBCharacter* Champion = Cast<AAIBCharacter>(GetPawn()))
	{
		TickWander(Champion, DeltaSeconds);
	}
}

void AAIBBotController::TickWander(AAIBCharacter* Champion, float DeltaSeconds)
{
	if (WanderPoints.Num() == 0)
	{
		return;
	}
	if (WanderPause > 0.f)
	{
		WanderPause -= DeltaSeconds;
		return;
	}
	const FVector Location = Champion->GetActorLocation();
	if (!bHasTarget)
	{
		WanderTarget = WanderPoints[FMath::RandRange(0, WanderPoints.Num() - 1)] + FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0.f);
		bHasTarget = true;
	}
	FVector ToTarget = WanderTarget - Location;
	ToTarget.Z = 0.f;
	if (ToTarget.Size() < 120.f)
	{
		bHasTarget = false;
		WanderPause = FMath::FRandRange(1.5f, 5.f);
		return;
	}
	// Bail out of a target we can't reach (blocked) after a while.
	const float Speed = Champion->GetVelocity().Size2D();
	StuckTimer = Speed < 20.f ? StuckTimer + DeltaSeconds : 0.f;
	if (StuckTimer > 1.5f)
	{
		StuckTimer = 0.f;
		bHasTarget = false;
		return;
	}
	Champion->AddMovementInput(ToTarget.GetSafeNormal(), 0.55f);
}

void AAIBBotController::TickDrive(AAIBVehicle* Vehicle, float DeltaSeconds)
{
	const UAIBWorldSubsystem* WorldSys = GetWorld()->GetSubsystem<UAIBWorldSubsystem>();
	const AAIBRaceCourse* Course = WorldSys ? WorldSys->GetRaceCourse() : nullptr;
	const AAIBGuardianSite* Guardian = WorldSys ? WorldSys->GetGuardian() : nullptr;
	if (!Course || !Guardian)
	{
		return;
	}
	const TArray<FVector>& Path = Course->GetPath();
	if (Path.Num() < 2)
	{
		return;
	}

	const FVector Location = Vehicle->GetActorLocation();
	const FVector Switch = Guardian->GetSwitchLocation();
	const float SpeedKmh = Vehicle->GetSpeedKmh();

	// Arrived: stop on the switch and hold.
	if (FVector::Dist2D(Location, Switch) < AAIBGuardianSite::SwitchRadius * 0.6f)
	{
		Vehicle->ApplyThrottle(-0.6f * FMath::Clamp(SpeedKmh / 20.f, 0.f, 1.f));
		Vehicle->ApplySteering(0.f);
		Vehicle->ApplyHandbrake(SpeedKmh < 8.f);
		return;
	}
	Vehicle->ApplyHandbrake(false);

	// Recover from being stuck: reverse with opposite lock.
	if (ReverseTimer > 0.f)
	{
		ReverseTimer -= DeltaSeconds;
		Vehicle->ApplyThrottle(-1.f);
		Vehicle->ApplySteering(ReverseSteer);
		return;
	}

	PathIndex = FMath::Max(PathIndex, Course->FindNearestIndex(Location, PathIndex));
	const int32 Lookahead = SpeedKmh > 50.f ? 4 : 3;
	const int32 TargetIndex = FMath::Min(PathIndex + Lookahead, Path.Num() - 1);
	FVector Target = Path[TargetIndex];
	if (TargetIndex >= Path.Num() - 2)
	{
		Target = Switch;
	}

	const FVector Forward = Vehicle->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget = (Target - Location).GetSafeNormal2D();
	const float Cross = FVector::CrossProduct(Forward, ToTarget).Z;
	const float Dot = FVector::DotProduct(Forward, ToTarget);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Cross, Dot));

	const float Steer = FMath::Clamp(AngleDeg / 32.f, -1.f, 1.f);
	const float MaxSpeed = FMath::Lerp(62.f, 88.f, Skill);
	float Throttle = 1.f;
	if (FMath::Abs(AngleDeg) > 55.f)
	{
		Throttle = 0.45f;
	}
	if (SpeedKmh > MaxSpeed)
	{
		Throttle = 0.2f;
	}
	const float DistToSwitch = FVector::Dist2D(Location, Switch);
	if (DistToSwitch < 3000.f && SpeedKmh > 35.f)
	{
		Throttle = -0.4f; // brake into the switch area
	}
	// Small wobble so bots aren't laser-perfect.
	const float Wobble = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.9f + Skill * 10.f) * (1.f - Skill) * 0.15f;
	Vehicle->ApplySteering(FMath::Clamp(Steer + Wobble, -1.f, 1.f));
	Vehicle->ApplyThrottle(Throttle);

	StuckTimer = (SpeedKmh < 3.f && Throttle > 0.3f) ? StuckTimer + DeltaSeconds : 0.f;
	if (StuckTimer > 1.8f)
	{
		StuckTimer = 0.f;
		ReverseTimer = 1.4f;
		ReverseSteer = Steer > 0.f ? -1.f : 1.f;
	}
}
