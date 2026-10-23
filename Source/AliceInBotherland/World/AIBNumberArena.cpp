// Alice in Botherland - Stage 1 arena: the Room of Numbers

#include "World/AIBNumberArena.h"
#include "Core/AIBAssets.h"
#include "Game/AIBGameState.h"
#include "Game/AIBPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "AIBNumberArena"

namespace
{
	constexpr float RoomRadius = 1350.f;
	constexpr float RoomHeight = 950.f;
	constexpr float TableRadius = 210.f;
	constexpr float TableHeight = 95.f;
	constexpr float PodiumRadius = 275.f;

	const FNumberFormattingOptions* OneDecimal()
	{
		static FNumberFormattingOptions Options = []
		{
			FNumberFormattingOptions O;
			O.SetUseGrouping(false);
			O.SetMinimumFractionalDigits(1);
			O.SetMaximumFractionalDigits(1);
			return O;
		}();
		return &Options;
	}
}

AAIBNumberArena::AAIBNumberArena()
{
	PrimaryActorTick.bCanEverTick = true;
	VisibleInZones = { EAIBSceneZone::NumberRoom };
}

FVector AAIBNumberArena::SlotDirection(int32 Slot) const
{
	const float Angle = FMath::DegreesToRadians(90.f + Slot * 72.f);
	return GetActorRotation().RotateVector(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f));
}

FTransform AAIBNumberArena::GetStandTransform(int32 Slot) const
{
	const FVector Dir = SlotDirection(Slot);
	const FVector Loc = GetActorLocation() + Dir * StandRadius + FVector(0.f, 0.f, 100.f);
	return FTransform((-Dir).Rotation(), Loc);
}

FTransform AAIBNumberArena::GetSeatCamera(int32 Slot) const
{
	const FVector Dir = SlotDirection(Slot);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Dir);
	const FVector CamLoc = GetActorLocation() + Dir * 640.f + Side * 120.f + FVector(0.f, 0.f, 255.f);
	const FVector LookAt = GetActorLocation() - Dir * 120.f + FVector(0.f, 0.f, 135.f);
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBNumberArena::GetOverviewCamera(float Time) const
{
	const float Angle = Time * 0.08f;
	const FVector CamLoc = GetActorLocation() + FVector(FMath::Cos(Angle) * 900.f, FMath::Sin(Angle) * 900.f, 720.f);
	const FVector LookAt = GetActorLocation() + FVector(0.f, 0.f, 110.f);
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

FTransform AAIBNumberArena::GetRevealCamera(float Time) const
{
	const float Angle = 0.6f + Time * 0.12f;
	const FVector CamLoc = GetActorLocation() + FVector(FMath::Cos(Angle) * 520.f, FMath::Sin(Angle) * 520.f, 175.f);
	const FVector LookAt = GetActorLocation() + FVector(0.f, 0.f, 255.f);
	return FTransform((LookAt - CamLoc).Rotation(), CamLoc);
}

void AAIBNumberArena::Build()
{
	UMaterialInterface* Stone = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_DarkStone"));
	UMaterialInterface* Metal = AIBAssets::Material(AIBAssets::Paths::Materials + TEXT("MI_AIB_DarkMetal"));
	if (!Stone)
	{
		UMaterialInstanceDynamic* Fallback = UMaterialInstanceDynamic::Create(AIBAssets::BasicMaterial(), this);
		AIBAssets::SetColor(Fallback, FLinearColor(0.02f, 0.02f, 0.025f));
		Stone = Fallback;
	}
	if (!Metal)
	{
		Metal = Stone;
	}

	// ---------------- Shell ----------------
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, -25.f), FVector(RoomRadius * 2.3f / 100.f, RoomRadius * 2.3f / 100.f, 0.5f)), Stone);
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, RoomHeight + 25.f), FVector(RoomRadius * 2.3f / 100.f, RoomRadius * 2.3f / 100.f, 0.5f)), Stone);
	const float PanelWidth = 2.f * RoomRadius * FMath::Tan(FMath::DegreesToRadians(22.5f)) + 30.f;
	InlayMaterial = AIBAssets::MakeEmissive(this, FLinearColor(0.9f, 0.05f, 0.08f), 4.f);
	for (int32 Side = 0; Side < 8; ++Side)
	{
		const float Angle = Side * 45.f + 22.5f;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		AddBox(Dir * RoomRadius + FVector(0.f, 0.f, RoomHeight * 0.5f), FVector(80.f, PanelWidth, RoomHeight + 60.f), Stone, FRotator(0.f, Angle, 0.f));
		// Vertical light seam at each corner.
		const FVector Corner = FRotator(0.f, Side * 45.f, 0.f).Vector() * (RoomRadius / FMath::Cos(FMath::DegreesToRadians(22.5f)) - 45.f);
		AddBox(Corner + FVector(0.f, 0.f, RoomHeight * 0.5f), FVector(8.f, 8.f, RoomHeight - 120.f), InlayMaterial, FRotator(0.f, Side * 45.f, 0.f), false);
		// Ribbed wall panels.
		for (int32 Rib = -1; Rib <= 1; ++Rib)
		{
			const FVector Tangent = FRotator(0.f, Angle + 90.f, 0.f).Vector();
			AddBox(Dir * (RoomRadius - 45.f) + Tangent * Rib * 260.f + FVector(0.f, 0.f, RoomHeight * 0.5f), FVector(14.f, 18.f, RoomHeight - 180.f), Metal, FRotator(0.f, Angle, 0.f), false);
		}
	}
	// Floor inlay rings.
	for (int32 Ring = 0; Ring < 3; ++Ring)
	{
		const float R = 520.f + Ring * 260.f;
		const int32 Segments = 48;
		for (int32 Seg = 0; Seg < Segments; ++Seg)
		{
			const float A = Seg * 360.f / Segments;
			const FVector P = FRotator(0.f, A, 0.f).Vector() * R;
			AddBox(P + FVector(0.f, 0.f, 1.f), FVector(4.f, 2.f * PI * R / Segments * 0.7f, 2.f), InlayMaterial, FRotator(0.f, A, 0.f), false);
		}
	}

	// ---------------- Table ----------------
	UStaticMesh* TableMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("NumberRoom/SM_RoundTable_Big"));
	if (TableMesh)
	{
		const FVector Size = TableMesh->GetBoundingBox().GetSize();
		const float Scale = Size.X > 1.f ? TableRadius * 2.f / Size.X : 1.f;
		AddMesh(TableMesh, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(Scale)));
	}
	else
	{
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, TableHeight - 6.f), FVector(TableRadius * 2.f / 100.f, TableRadius * 2.f / 100.f, 0.12f)), Metal);
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, TableHeight * 0.5f - 6.f), FVector(1.2f, 1.2f, (TableHeight - 12.f) / 100.f)), Stone);
		UMaterialInstanceDynamic* Rim = AIBAssets::MakeEmissive(this, FLinearColor(0.9f, 0.08f, 0.1f), 8.f);
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, TableHeight), FVector(TableRadius * 1.6f / 100.f, TableRadius * 1.6f / 100.f, 0.01f)), Rim, false);
	}

	// ---------------- Hologram ----------------
	HoloMaterial = AIBAssets::MakeHologram(this, FLinearColor(1.f, 0.15f, 0.12f), 0.6f);
	UStaticMesh* RingMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("NumberRoom/SM_HoloRing"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		USceneComponent* Pivot = MakeScene(*FString::Printf(TEXT("HoloPivot_%d"), Index), FTransform(FVector(0.f, 0.f, 250.f)));
		HoloRings.Add(Pivot);
		const float Radius = 75.f + Index * 22.f;
		if (RingMesh)
		{
			AddMesh(RingMesh, FTransform(FRotator(Index * 25.f, 0.f, Index * 40.f), FVector::ZeroVector, FVector(Radius / 100.f)), HoloMaterial, false, Pivot);
		}
		else
		{
			AddMesh(AIBAssets::Cylinder(), FTransform(FRotator(Index * 25.f, 0.f, Index * 40.f), FVector::ZeroVector, FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, 0.01f)), HoloMaterial, false, Pivot);
		}
	}
	AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 175.f), FVector(0.06f, 0.06f, 1.5f)), HoloMaterial, false);
	HoloMain = AddText(FVector(0.f, 0.f, 255.f), FRotator::ZeroRotator, FText::FromString(TEXT("?")), FLinearColor(1.f, 0.25f, 0.2f), 90.f);
	HoloSub = AddText(FVector(0.f, 0.f, 185.f), FRotator::ZeroRotator, FText::GetEmpty(), FLinearColor(1.f, 0.8f, 0.75f), 24.f);
	AddPointLight(FVector(0.f, 0.f, 250.f), FLinearColor(1.f, 0.2f, 0.15f), 1500.f, 1200.f, false);

	WallTitle = AddText(FVector(RoomRadius - 70.f, 0.f, 640.f), FRotator(0.f, 180.f, 0.f), LOCTEXT("WallTitle", "AVERAGE  x  0.8"), FLinearColor(0.9f, 0.1f, 0.12f), 70.f);
	AddText(FVector(-RoomRadius + 70.f, 0.f, 640.f), FRotator(0.f, 0.f, 0.f), LOCTEXT("WallTitle2", "CLOSEST SURVIVES"), FLinearColor(0.9f, 0.1f, 0.12f), 60.f);

	// ---------------- Podiums ----------------
	UStaticMesh* PodiumMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("NumberRoom/SM_Podium"));
	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		const float Angle = 90.f + Slot * 72.f;
		const FVector Dir = FRotator(0.f, Angle, 0.f).Vector();
		const FLinearColor Color = AIB::Community(Slot).Color;
		UMaterialInstanceDynamic* Glow = AIBAssets::MakeEmissive(this, Color, 10.f);
		PodiumGlow.Add(Glow);
		const FRotator FaceCenter(0.f, Angle + 180.f, 0.f);
		if (PodiumMesh)
		{
			UStaticMeshComponent* Podium = AddMesh(PodiumMesh, FTransform(FaceCenter, Dir * PodiumRadius));
			if (Podium)
			{
				// Material slot 2 ("Trim") glows in the champion's colour.
				const int32 TrimIndex = Podium->GetMaterialIndex(TEXT("Trim"));
				Podium->SetMaterial(TrimIndex != INDEX_NONE ? TrimIndex : 0, Glow);
			}
		}
		else
		{
			AddMesh(AIBAssets::Cylinder(), FTransform(FaceCenter, Dir * PodiumRadius + FVector(0.f, 0.f, 50.f), FVector(0.8f, 0.8f, 1.f)), Metal);
			AddMesh(AIBAssets::Cylinder(), FTransform(FaceCenter, Dir * PodiumRadius + FVector(0.f, 0.f, 101.f), FVector(0.84f, 0.84f, 0.03f)), Glow, false);
		}
		// Floor glyph ring under each champion.
		AddMesh(AIBAssets::Cylinder(), FTransform(FRotator::ZeroRotator, Dir * StandRadius + FVector(0.f, 0.f, 1.f), FVector(1.1f, 1.1f, 0.01f)), Glow, false);

		PodiumLights.Add(AddSpotLight(Dir * StandRadius + FVector(0.f, 0.f, RoomHeight - 40.f), FRotator(-90.f, 0.f, 0.f), Color, 9000.f, 1400.f, 22.f, true));
		PodiumNumbers.Add(AddText(Dir * PodiumRadius + FVector(0.f, 0.f, 200.f), FaceCenter, FText::GetEmpty(), FLinearColor::White, 60.f));
		PodiumNames.Add(AddText(Dir * (PodiumRadius - 70.f) + FVector(0.f, 0.f, 112.f), FRotator(-35.f, Angle + 180.f, 0.f) , FText::GetEmpty(), Color, 14.f));
	}

	// Moody ambient: dim cool fill from above, hot rim from the walls.
	AddPointLight(FVector(0.f, 0.f, RoomHeight - 80.f), FLinearColor(0.55f, 0.6f, 0.8f), 600.f, 2500.f, false);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Dir = FRotator(0.f, Index * 90.f + 45.f, 0.f).Vector();
		AddRectLight(Dir * (RoomRadius - 120.f) + FVector(0.f, 0.f, 120.f), (-Dir).Rotation(), FLinearColor(0.9f, 0.1f, 0.1f), 120.f, 400.f, 30.f, 2200.f);
	}
}

void AAIBNumberArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsHidden())
	{
		return;
	}
	UWorld* World = GetWorld();
	const AAIBGameState* GS = World ? World->GetGameState<AAIBGameState>() : nullptr;
	const float Time = World ? World->GetTimeSeconds() : 0.f;

	for (int32 Index = 0; Index < HoloRings.Num(); ++Index)
	{
		HoloRings[Index]->SetRelativeRotation(FRotator(0.f, Time * (18.f + Index * 11.f) * (Index % 2 ? -1.f : 1.f), 0.f));
	}

	// Billboard the hologram text towards the local camera.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector CamLoc = Cam->GetCameraLocation();
		for (UTextRenderComponent* Text : { HoloMain.Get(), HoloSub.Get() })
		{
			if (Text)
			{
				FVector ToCam = CamLoc - Text->GetComponentLocation();
				ToCam.Z = 0.f;
				Text->SetWorldRotation(ToCam.Rotation());
			}
		}
	}

	if (!GS)
	{
		return;
	}

	// Hologram content by phase.
	FText Main = FText::FromString(TEXT("?"));
	FText Sub = FText::GetEmpty();
	FLinearColor MainColor(1.f, 0.25f, 0.2f);
	const FAIBStage1Round& Last = GS->Stage1Last;
	switch (GS->Phase)
	{
	case EAIBPhase::Stage1Briefing:
		Main = LOCTEXT("HoloBrief", "0 - 100");
		Sub = LOCTEXT("HoloBriefSub", "TARGET = AVERAGE x 0.8");
		break;
	case EAIBPhase::Stage1RoundIntro:
		Main = FText::Format(LOCTEXT("HoloRound", "ROUND {0}"), GS->Stage1Round);
		Sub = GS->bStage1SuddenDeath ? LOCTEXT("HoloSudden", "SUDDEN DEATH") : FText::GetEmpty();
		break;
	case EAIBPhase::Stage1Selection:
		Main = FText::AsNumber(FMath::CeilToInt(GS->GetPhaseRemaining()));
		Sub = LOCTEXT("HoloChoose", "CHOOSE");
		break;
	case EAIBPhase::Stage1Reveal:
	{
		// Rolling digits, then lock on the target.
		const float Alpha = GS->GetPhaseAlpha();
		if (Alpha < 0.7f)
		{
			Main = FText::AsNumber(FMath::RandRange(0, 100));
			Sub = LOCTEXT("HoloCalc", "CALCULATING");
		}
		else
		{
			Main = FText::AsNumber(Last.Target, OneDecimal());
			Sub = FText::Format(LOCTEXT("HoloAvg", "AVERAGE {0}"), FText::AsNumber(Last.Average, OneDecimal()));
			MainColor = FLinearColor(1.f, 0.85f, 0.4f);
		}
		break;
	}
	case EAIBPhase::Stage1Results:
	case EAIBPhase::Stage1Elimination:
		Main = FText::AsNumber(Last.Target, OneDecimal());
		Sub = FText::Format(LOCTEXT("HoloAvg2", "TARGET  (AVERAGE {0})"), FText::AsNumber(Last.Average, OneDecimal()));
		MainColor = FLinearColor(1.f, 0.85f, 0.4f);
		break;
	default:
		break;
	}
	if (HoloMain)
	{
		HoloMain->SetText(Main);
		HoloMain->SetTextRenderColor(MainColor.ToFColor(true));
	}
	if (HoloSub)
	{
		HoloSub->SetText(Sub);
	}

	const bool bShowNumbers = GS->Phase == EAIBPhase::Stage1Results || GS->Phase == EAIBPhase::Stage1Elimination
		|| (GS->Phase == EAIBPhase::Stage1Reveal && GS->GetPhaseAlpha() > 0.7f);

	for (int32 Slot = 0; Slot < AIB::MaxPlayers; ++Slot)
	{
		const AAIBPlayerState* PS = GS->GetPlayerBySlot(Slot);
		const bool bSeated = PS != nullptr;
		const bool bActive = PS && PS->IsActiveChampion();
		FLinearColor Color = AIB::Community(Slot).Color;
		float Intensity = 9000.f;
		float Glow = 10.f;

		const FAIBNumberEntry* Entry = Last.Entries.FindByPredicate([Slot](const FAIBNumberEntry& E) { return E.Slot == Slot; });
		if (!bSeated)
		{
			Intensity = 0.f;
			Glow = 0.f;
		}
		else if (!bActive)
		{
			Color = FLinearColor(0.6f, 0.02f, 0.02f);
			Intensity = 1200.f;
			Glow = 2.f;
		}
		if (bShowNumbers && Entry)
		{
			if (Entry->bWinner)
			{
				Color = FLinearColor(1.f, 0.9f, 0.5f);
				Intensity = 20000.f;
				Glow = 25.f;
			}
			else if (Entry->bFarthest)
			{
				const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 10.f);
				Color = FLinearColor(1.f, 0.05f, 0.05f);
				Intensity = 6000.f + 14000.f * Pulse;
				Glow = 8.f + 20.f * Pulse;
			}
		}
		if (PodiumLights.IsValidIndex(Slot) && PodiumLights[Slot])
		{
			PodiumLights[Slot]->SetLightColor(Color);
			PodiumLights[Slot]->SetIntensity(Intensity * AIB::LightScale);
		}
		if (PodiumGlow.IsValidIndex(Slot) && PodiumGlow[Slot])
		{
			AIBAssets::SetColor(PodiumGlow[Slot], Color);
			PodiumGlow[Slot]->SetScalarParameterValue(TEXT("Intensity"), Glow);
		}
		if (PodiumNumbers.IsValidIndex(Slot) && PodiumNumbers[Slot])
		{
			PodiumNumbers[Slot]->SetText(bShowNumbers && Entry ? FText::AsNumber(Entry->Number) : FText::GetEmpty());
			PodiumNumbers[Slot]->SetTextRenderColor(Color.ToFColor(true));
		}
		if (PodiumNames.IsValidIndex(Slot) && PodiumNames[Slot])
		{
			PodiumNames[Slot]->SetText(PS ? FText::FromString(PS->GetPlayerName().ToUpper()) : FText::GetEmpty());
		}
	}
}

#undef LOCTEXT_NAMESPACE
