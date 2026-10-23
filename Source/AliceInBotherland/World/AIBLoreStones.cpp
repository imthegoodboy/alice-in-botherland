// Alice in Botherland - marker stones to discover while exploring the island

#include "World/AIBLoreStones.h"
#include "World/AIBRacePath.h"
#include "Core/AIBAssets.h"
#include "Game/AIBPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AAIBLoreStones::AAIBLoreStones()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	VisibleInZones = { EAIBSceneZone::Island };
}

FName AAIBLoreStones::LineFor(int32 Index)
{
	return FName(*FString::Printf(TEXT("NAR_Lore_%d"), Index + 1));
}

void AAIBLoreStones::Build()
{
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);

	UMaterialInterface* Stone = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_TempleStone"));
	RuneGlow = AIBAssets::MakeEmissive(this, FLinearColor(0.45f, 1.f, 0.6f), 14.f);
	UMaterialInstanceDynamic* PillarGlow = AIBAssets::MakeBeam(this, FLinearColor(0.45f, 1.f, 0.6f), 1.4f);

	// Points along the road from the beach towards the town (walkable by construction), set off to the side.
	const int32 PathIndices[Count] = { 2, 5, 9, 13, 16 };
	const float Side[Count] = { 1.f, -1.f, 1.f, -1.f, 1.f };
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 P = FMath::Min(PathIndices[Index], int32(UE_ARRAY_COUNT(AIBRacePath::Points)) - 2);
		const FVector2D A = AIBRacePath::Points[P];
		const FVector2D B = AIBRacePath::Points[P + 1];
		const FVector2D Dir = (B - A).GetSafeNormal();
		const FVector2D XY = A + FVector2D(-Dir.Y, Dir.X) * 1300.f * Side[Index];
		const FVector Ground(XY.X, XY.Y, GroundZ(GetWorld(), XY, 0.f));
		Stones.Add(Ground);

		const FRotator Facing(0.f, FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)) + 90.f * Side[Index], 0.f);
		// A weathered standing slab on a low plinth, with a glowing rune on its face.
		AddBox(Ground + FVector(0.f, 0.f, 15.f), FVector(220.f, 140.f, 30.f), Stone, Facing, false);
		AddBox(Ground + FVector(0.f, 0.f, 150.f), FVector(110.f, 34.f, 240.f), Stone, Facing + FRotator(0.f, 0.f, 3.f), false);
		AddBox(Ground + FVector(0.f, 0.f, 175.f) + Facing.RotateVector(FVector(0.f, 18.f, 0.f)), FVector(46.f, 4.f, 70.f), RuneGlow, Facing, false);
		Lights.Add(AddPointLight(Ground + FVector(0.f, 0.f, 210.f), FLinearColor(0.5f, 1.f, 0.65f), 900.f, 900.f, false));

		UStaticMeshComponent* Pillar = AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 2600.f), FVector(0.35f, 0.35f, 50.f)), PillarGlow, false);
		if (Pillar)
		{
			Pillar->SetCastShadow(false);
		}
		Pillars.Add(Pillar);
	}
}

void AAIBLoreStones::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Each player only sees the pillars of the stones they still have to read.
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AAIBPlayerState* PS = PC ? PC->GetPlayerState<AAIBPlayerState>() : nullptr;
	const int32 Found = PS ? PS->LoreFound : 0;
	for (int32 Index = 0; Index < Pillars.Num(); ++Index)
	{
		const bool bUnread = !(Found & (1 << Index));
		if (Pillars[Index])
		{
			Pillars[Index]->SetVisibility(bUnread);
		}
		if (Lights.IsValidIndex(Index) && Lights[Index])
		{
			Lights[Index]->SetIntensity((bUnread ? 900.f : 250.f) * AIB::LightScale);
		}
	}
}
