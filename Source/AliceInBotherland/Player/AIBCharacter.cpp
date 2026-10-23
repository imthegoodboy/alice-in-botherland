// Alice in Botherland - champion character

#include "Player/AIBCharacter.h"
#include "Game/AIBPlayerState.h"
#include "Core/AIBAssets.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"

AAIBCharacter::AAIBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(60.f);

	GetCapsuleComponent()->InitCapsuleSize(40.f, 94.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->JumpZVelocity = 520.f;
	Move->AirControl = 0.3f;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->BrakingDecelerationFalling = 1500.f;
	Move->bCanWalkOffLedgesWhenCrouching = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 360.f;
	CameraBoom->SocketOffset = FVector(0.f, 45.f, 55.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->ProbeSize = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(75.f);

	// Survival Character (Fab) with the pack's demo anim blueprint; falls back to the template mannequin.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SurvivalMesh(TEXT("/Game/Survival_Character/Meshes/SK_Survival_Character.SK_Survival_Character"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> SurvivalAnim(TEXT("/Game/Survival_Character/Demo/Characters/Mannequins/Animations/ABP_Quinn"));
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -94.f), FRotator(0.f, -90.f, 0.f));
	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	if (SurvivalMesh.Succeeded())
	{
		MeshComp->SetSkeletalMesh(SurvivalMesh.Object);
		if (SurvivalAnim.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(SurvivalAnim.Class);
		}
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
		static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (Quinn.Succeeded())
		{
			MeshComp->SetSkeletalMesh(Quinn.Object);
		}
		if (UnarmedAnim.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(UnarmedAnim.Class);
		}
	}

	SymbolMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SymbolMesh"));
	SymbolMesh->SetupAttachment(RootComponent);
	SymbolMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	SymbolMesh->SetRelativeScale3D(FVector(0.45f));
	SymbolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SymbolMesh->SetCastShadow(false);
	SymbolMesh->SetVisibility(false);
	SymbolMesh->SetUsingAbsoluteRotation(true);

	SymbolLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SymbolLight"));
	SymbolLight->SetupAttachment(SymbolMesh);
	SymbolLight->SetIntensityUnits(ELightUnits::Candelas);
	SymbolLight->SetIntensity(120.f * AIB::LightScale);
	SymbolLight->SetAttenuationRadius(350.f);
	SymbolLight->SetCastShadows(false);
	SymbolLight->SetVisibility(false);

	CommunityRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CommunityRing"));
	CommunityRing->SetupAttachment(RootComponent);
	CommunityRing->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
	CommunityRing->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.004f));
	CommunityRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CommunityRing->SetCastShadow(false);
}

void AAIBCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAIBCharacter, bSprinting);
	DOREPLIFETIME(AAIBCharacter, bHiddenForRace);
}

void AAIBCharacter::BeginPlay()
{
	Super::BeginPlay();
	// A thin glowing ring at the feet (a flat disc read as a solid plate floating under the champion).
	if (UStaticMesh* Ring = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("NumberRoom/SM_HoloRing")))
	{
		CommunityRing->SetStaticMesh(Ring);
		CommunityRing->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.2f));
	}
	else if (UStaticMesh* Cyl = AIBAssets::Cylinder())
	{
		CommunityRing->SetStaticMesh(Cyl);
	}
	ApplyCommunityLook();
}

void AAIBCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyCommunityLook();
}

void AAIBCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyCommunityLook();
}

AAIBPlayerState* AAIBCharacter::GetChampionState() const
{
	return GetPlayerState<AAIBPlayerState>();
}

int32 AAIBCharacter::GetSlot() const
{
	const AAIBPlayerState* PS = GetChampionState();
	return PS ? PS->Slot : INDEX_NONE;
}

void AAIBCharacter::ApplyCommunityLook()
{
	const int32 Slot = GetSlot();
	if (Slot == INDEX_NONE || Slot == AppliedSlot)
	{
		return;
	}
	AppliedSlot = Slot;
	const FLinearColor Color = AIB::Community(Slot).Color;

	USkeletalMeshComponent* MeshComp = GetMesh();
	const int32 JacketIndex = MeshComp->GetMaterialIndex(TEXT("Jacket1"));
	if (JacketIndex != INDEX_NONE)
	{
		JacketMID = MeshComp->CreateDynamicMaterialInstance(JacketIndex);
		if (JacketMID)
		{
			JacketMID->SetVectorParameterValue(TEXT("Tint"), Color * 1.15f);
			JacketMID->SetScalarParameterValue(TEXT("Desaturation"), 0.85f);
		}
	}
	const int32 BackpackIndex = MeshComp->GetMaterialIndex(TEXT("Backpack1"));
	if (BackpackIndex != INDEX_NONE)
	{
		if (UMaterialInstanceDynamic* BackpackMID = MeshComp->CreateDynamicMaterialInstance(BackpackIndex))
		{
			BackpackMID->SetVectorParameterValue(TEXT("Tint"), FMath::Lerp(FLinearColor::White, Color, 0.35f));
		}
	}

	RingMID = AIBAssets::MakeHologram(this, Color, 0.55f);
	CommunityRing->SetMaterial(0, RingMID);
}

void AAIBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Time = GetWorld()->GetTimeSeconds();

	if (SymbolMesh->IsVisible())
	{
		const float Bob = FMath::Sin(Time * 2.2f) * 6.f;
		SymbolMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f + Bob));
		SymbolMesh->SetWorldRotation(FRotator(0.f, Time * 60.f, 0.f));
	}

	// Timed effects.
	if (ActiveEffect != EEffect::None && EffectStartTime >= 0.f)
	{
		const float T = Time - EffectStartTime;
		for (int32 Index = 0; Index < EffectComponents.Num(); ++Index)
		{
			USceneComponent* Comp = EffectComponents[Index];
			if (!Comp)
			{
				continue;
			}
			if (ActiveEffect == EEffect::Sacrifice)
			{
				// Pillar widens then collapses, the champion vanishes into the light.
				const float Grow = FMath::Clamp(T / 1.2f, 0.f, 1.f);
				const float Fade = 1.f - FMath::Clamp((T - 3.5f) / 1.5f, 0.f, 1.f);
				if (Index == 0)
				{
					Comp->SetRelativeScale3D(FVector(0.2f + 0.4f * Grow * Fade, 0.2f + 0.4f * Grow * Fade, 60.f));
				}
				if (UPointLightComponent* Light = Cast<UPointLightComponent>(Comp))
				{
					Light->SetIntensity(9000.f * Grow * Fade * AIB::LightScale);
				}
				if (T > 2.4f)
				{
					GetMesh()->SetVisibility(false, true);
				}
			}
			else if (ActiveEffect == EEffect::Crown)
			{
				if (Index == 0)
				{
					Comp->SetRelativeRotation(FRotator(0.f, T * 40.f, 0.f));
					Comp->SetRelativeLocation(FVector(0.f, 0.f, 120.f + FMath::Sin(T * 2.f) * 5.f));
				}
			}
			else if (ActiveEffect == EEffect::Eliminated)
			{
				if (UPointLightComponent* Light = Cast<UPointLightComponent>(Comp))
				{
					Light->SetIntensity(8000.f * FMath::Max(0.f, 1.f - T / 2.5f) * AIB::LightScale);
				}
			}
		}
		if (ActiveEffect == EEffect::Eliminated && T > 5.f)
		{
			GetMesh()->SetVisibility(false, true);
		}
	}
}

void AAIBCharacter::SetMovementLocked(bool bLocked)
{
	if (!HasAuthority())
	{
		return;
	}
	if (bLocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AAIBCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	bSprinting = bNewSprinting;
	OnRep_Sprinting();
}

void AAIBCharacter::OnRep_Sprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? RunSpeed : WalkSpeed;
}

void AAIBCharacter::SetHiddenForRace(bool bHide)
{
	bHiddenForRace = bHide;
	OnRep_HiddenForRace();
}

void AAIBCharacter::OnRep_HiddenForRace()
{
	SetActorHiddenInGame(bHiddenForRace);
	SetActorEnableCollision(!bHiddenForRace);
}

void AAIBCharacter::ShowLocalSymbol(EAIBSymbol Symbol)
{
	LocalSymbol = Symbol;
	if (Symbol == EAIBSymbol::None)
	{
		SymbolMesh->SetVisibility(false);
		SymbolLight->SetVisibility(false);
		return;
	}
	const TCHAR* Names[] = { TEXT(""), TEXT("Heart"), TEXT("Diamond"), TEXT("Club"), TEXT("Spade") };
	UStaticMesh* SuitMesh = AIBAssets::Mesh(AIBAssets::Paths::Env + TEXT("Symbols/SM_Suit_") + Names[(int32)Symbol], AIBAssets::Sphere());
	SymbolMesh->SetStaticMesh(SuitMesh);
	const FLinearColor Color = AIB::SymbolColor(Symbol);
	SymbolMID = AIBAssets::MakeEmissive(this, Color, 12.f);
	SymbolMesh->SetMaterial(0, SymbolMID);
	SymbolMesh->SetVisibility(true);
	SymbolLight->SetLightColor(Color);
	SymbolLight->SetVisibility(true);
}

void AAIBCharacter::MulticastEliminated_Implementation()
{
	ShowLocalSymbol(EAIBSymbol::None);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();
	MeshComp->AddImpulse(FVector(0.f, 0.f, -2000.f) + GetActorForwardVector() * -3000.f, NAME_None, true);

	UPointLightComponent* Flash = NewObject<UPointLightComponent>(this);
	Flash->SetupAttachment(RootComponent);
	Flash->SetIntensityUnits(ELightUnits::Candelas);
	Flash->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	Flash->SetAttenuationRadius(900.f);
	Flash->SetIntensity(8000.f * AIB::LightScale);
	Flash->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Flash->RegisterComponent();
	EffectComponents.Add(Flash);
	ActiveEffect = EEffect::Eliminated;
	EffectStartTime = GetWorld()->GetTimeSeconds();
	if (RingMID)
	{
		AIBAssets::SetColor(RingMID, FLinearColor(0.6f, 0.02f, 0.02f));
	}
}

void AAIBCharacter::MulticastSacrifice_Implementation()
{
	ShowLocalSymbol(EAIBSymbol::None);
	const FLinearColor Color = FLinearColor(0.85f, 0.95f, 1.f);
	UStaticMeshComponent* Pillar = NewObject<UStaticMeshComponent>(this);
	Pillar->SetupAttachment(RootComponent);
	Pillar->SetStaticMesh(AIBAssets::Mesh(AIBAssets::Paths::VFX + TEXT("SM_BeamCylinder"), AIBAssets::Cylinder()));
	Pillar->SetMaterial(0, AIBAssets::MakeBeam(this, Color, 6.f));
	Pillar->SetRelativeLocation(FVector(0.f, 0.f, -94.f));
	Pillar->SetRelativeScale3D(FVector(0.2f, 0.2f, 60.f));
	Pillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pillar->SetCastShadow(false);
	Pillar->RegisterComponent();
	EffectComponents.Add(Pillar);

	UPointLightComponent* Glow = NewObject<UPointLightComponent>(this);
	Glow->SetupAttachment(RootComponent);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetLightColor(Color);
	Glow->SetAttenuationRadius(2500.f);
	Glow->SetIntensity(0.f);
	Glow->RegisterComponent();
	EffectComponents.Add(Glow);

	ActiveEffect = EEffect::Sacrifice;
	EffectStartTime = GetWorld()->GetTimeSeconds();
}

void AAIBCharacter::MulticastCrown_Implementation()
{
	ShowLocalSymbol(EAIBSymbol::None);
	USceneComponent* CrownRoot = NewObject<USceneComponent>(this);
	CrownRoot->SetupAttachment(RootComponent);
	CrownRoot->RegisterComponent();
	EffectComponents.Add(CrownRoot);

	UMaterialInstanceDynamic* Gold = AIBAssets::MakeEmissive(this, FLinearColor(1.f, 0.75f, 0.25f), 25.f);
	for (int32 Index = 0; Index < 7; ++Index)
	{
		UStaticMeshComponent* Spike = NewObject<UStaticMeshComponent>(this);
		Spike->SetupAttachment(CrownRoot);
		Spike->SetStaticMesh(AIBAssets::Cone());
		Spike->SetMaterial(0, Gold);
		const FVector Dir = FRotator(0.f, Index * 360.f / 7.f, 0.f).Vector();
		Spike->SetRelativeLocation(Dir * 14.f);
		Spike->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.14f));
		Spike->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Spike->RegisterComponent();
	}
	UPointLightComponent* Glow = NewObject<UPointLightComponent>(this);
	Glow->SetupAttachment(CrownRoot);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetLightColor(FLinearColor(1.f, 0.75f, 0.35f));
	Glow->SetAttenuationRadius(800.f);
	Glow->SetIntensity(3000.f * AIB::LightScale);
	Glow->RegisterComponent();

	ActiveEffect = EEffect::Crown;
	EffectStartTime = GetWorld()->GetTimeSeconds();
}

void AAIBCharacter::MulticastRestore_Implementation()
{
	for (USceneComponent* Comp : EffectComponents)
	{
		if (Comp)
		{
			Comp->DestroyComponent(true);
		}
	}
	EffectComponents.Reset();
	ActiveEffect = EEffect::None;
	EffectStartTime = -1.f;
	ShowLocalSymbol(EAIBSymbol::None);

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetAllBodiesSimulatePhysics(false);
	MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
	MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MeshComp->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -94.f), FRotator(0.f, -90.f, 0.f));
	MeshComp->SetVisibility(true, true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	AppliedSlot = INDEX_NONE;
	ApplyCommunityLook();
}
