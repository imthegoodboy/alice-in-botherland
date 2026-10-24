// Alice in Botherland - Stage 3 race vehicle (Chaos offroad car)

#include "Player/AIBVehicle.h"
#include "Core/AIBAssets.h"
#include "Core/AIBTypes.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundBase.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "AliceInBotherland.h"

UAIBWheelFront::UAIBWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 38.f;
	bAffectedByEngine = true;
	WheelRadius = 50.f;
	CorneringStiffness = 750.f;
	FrictionForceMultiplier = 4.f;
	SuspensionMaxRaise = 20.f;
	SuspensionMaxDrop = 20.f;
	WheelLoadRatio = 1.f;
	SpringRate = 100.f;
	SpringPreload = 100.f;
	SweepShape = ESweepShape::Shapecast;
	MaxBrakeTorque = 3000.f;
	MaxHandBrakeTorque = 6000.f;
}

UAIBWheelRear::UAIBWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
	WheelRadius = 50.f;
	CorneringStiffness = 750.f;
	FrictionForceMultiplier = 4.f;
	SuspensionMaxRaise = 20.f;
	SuspensionMaxDrop = 20.f;
	WheelLoadRatio = 1.f;
	SpringRate = 100.f;
	SpringPreload = 100.f;
	SweepShape = ESweepShape::Shapecast;
	MaxBrakeTorque = 3000.f;
	MaxHandBrakeTorque = 6000.f;
}

AAIBVehicle::AAIBVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(60.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT("/Game/Vehicles/OffroadCar/SKM_Offroad.SKM_Offroad"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> CarAnim(TEXT("/Game/Vehicles/OffroadCar/Offroad_AnimBP"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Body.SM_Offroad_Body"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TireMesh(TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Tire.SM_Offroad_Tire"));

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (CarMesh.Succeeded())
	{
		MeshComp->SetSkeletalMesh(CarMesh.Object);
	}
	if (CarAnim.Succeeded())
	{
		MeshComp->SetAnimInstanceClass(CarAnim.Class);
	}
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionProfileName(FName("Vehicle"));

	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(MeshComp);
	Chassis->SetCollisionProfileName(FName("NoCollision"));
	if (BodyMesh.Succeeded())
	{
		Chassis->SetStaticMesh(BodyMesh.Object);
	}

	auto MakeTire = [&](const TCHAR* Name, const TCHAR* Socket, bool bFlip)
	{
		UStaticMeshComponent* Tire = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Tire->SetupAttachment(MeshComp, FName(Socket));
		Tire->SetCollisionProfileName(FName("NoCollision"));
		if (bFlip)
		{
			Tire->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		}
		if (TireMesh.Succeeded())
		{
			Tire->SetStaticMesh(TireMesh.Object);
		}
		return Tire;
	};
	TireFL = MakeTire(TEXT("TireFL"), TEXT("VisWheel_FL"), false);
	TireFR = MakeTire(TEXT("TireFR"), TEXT("VisWheel_FR"), true);
	TireRL = MakeTire(TEXT("TireRL"), TEXT("VisWheel_BL"), false);
	TireRR = MakeTire(TEXT("TireRR"), TEXT("VisWheel_BR"), true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(MeshComp);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 75.f));
	CameraBoom->TargetArmLength = 720.f;
	CameraBoom->SocketOffset.Z = 170.f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 3.f;
	CameraBoom->CameraLagMaxDistance = 60.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);
	Camera->SetFieldOfView(80.f);

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(MeshComp);
	EngineAudio->bAutoActivate = false;

	UnderGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("UnderGlow"));
	UnderGlow->SetupAttachment(MeshComp);
	UnderGlow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	UnderGlow->SetIntensityUnits(ELightUnits::Candelas);
	UnderGlow->SetIntensity(350.f * AIB::LightScale);
	UnderGlow->SetAttenuationRadius(700.f);
	UnderGlow->SetCastShadows(false);

	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(MeshComp);
	Beacon->SetRelativeLocation(FVector(-30.f, 0.f, 205.f));
	Beacon->SetRelativeScale3D(FVector(0.18f, 0.6f, 0.08f));
	Beacon->SetCollisionProfileName(FName("NoCollision"));

	UChaosWheeledVehicleMovementComponent* Move = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	Move->ChassisHeight = 160.f;
	Move->DragCoefficient = 0.1f;
	Move->DownforceCoefficient = 0.1f;
	Move->CenterOfMassOverride = FVector(0.f, 0.f, 75.f);
	Move->bEnableCenterOfMassOverride = true;
	Move->bLegacyWheelFrictionPosition = false;
	Move->WheelSetups.SetNum(4);
	const TCHAR* Bones[] = { TEXT("PhysWheel_FL"), TEXT("PhysWheel_FR"), TEXT("PhysWheel_BL"), TEXT("PhysWheel_BR") };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Move->WheelSetups[Index].WheelClass = Index < 2 ? UAIBWheelFront::StaticClass() : UAIBWheelRear::StaticClass();
		Move->WheelSetups[Index].BoneName = FName(Bones[Index]);
		Move->WheelSetups[Index].AdditionalOffset = FVector::ZeroVector;
	}
	Move->EngineSetup.MaxTorque = 650.f;
	Move->EngineSetup.MaxRPM = 5200.f;
	Move->EngineSetup.EngineIdleRPM = 1100.f;
	Move->EngineSetup.EngineBrakeEffect = 0.05f;
	Move->EngineSetup.EngineRevUpMOI = 5.f;
	Move->EngineSetup.EngineRevDownRate = 600.f;
	static ConstructorHelpers::FObjectFinder<UCurveFloat> Torque(TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/OffroadCar_TorqueCurve.OffroadCar_TorqueCurve"));
	if (Torque.Succeeded())
	{
		Move->EngineSetup.TorqueCurve.ExternalCurve = Torque.Object;
	}
	else
	{
		FRichCurve* Curve = Move->EngineSetup.TorqueCurve.GetRichCurve();
		Curve->AddKey(0.f, 0.75f);
		Curve->AddKey(2200.f, 1.f);
		Curve->AddKey(4500.f, 0.9f);
		Curve->AddKey(5200.f, 0.7f);
	}
	Move->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	Move->DifferentialSetup.FrontRearSplit = 0.5f;
	Move->SteeringSetup.SteeringType = ESteeringType::AngleRatio;
	Move->SteeringSetup.AngleRatio = 0.7f;
	FRichCurve* Steer = Move->SteeringSetup.SteeringCurve.GetRichCurve();
	Steer->AddKey(0.f, 1.f);
	Steer->AddKey(40.f, 0.75f);
	Steer->AddKey(120.f, 0.55f);
	Move->TransmissionSetup.bUseAutomaticGears = true;
	Move->TransmissionSetup.bUseAutoReverse = true;
}

void AAIBVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAIBVehicle, DriverSlot);
	DOREPLIFETIME(AAIBVehicle, bInputLocked);
}

UChaosWheeledVehicleMovementComponent* AAIBVehicle::GetChaos() const
{
	return Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
}

void AAIBVehicle::BeginPlay()
{
	Super::BeginPlay();
	if (USoundBase* Engine = AIBAssets::Sound(AIBAssets::Paths::Audio + TEXT("SFX/SFX_Engine_Loop")))
	{
		EngineAudio->SetSound(Engine);
		EngineAudio->Play();
	}
	Beacon->SetStaticMesh(AIBAssets::Cube());
	OnRep_DriverSlot();
}

void AAIBVehicle::SetDriverSlot(int32 Slot)
{
	DriverSlot = Slot;
	OnRep_DriverSlot();
}

void AAIBVehicle::OnRep_DriverSlot()
{
	if (DriverSlot == INDEX_NONE)
	{
		return;
	}
	const FLinearColor Color = AIB::Community(DriverSlot).Color;
	UnderGlow->SetLightColor(Color);
	Beacon->SetMaterial(0, AIBAssets::MakeEmissive(this, Color, 30.f));
}

void AAIBVehicle::SetInputLocked(bool bLocked)
{
	bInputLocked = bLocked;
	if (bLocked)
	{
		if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
		{
			Move->SetThrottleInput(0.f);
			Move->SetSteeringInput(0.f);
			Move->SetHandbrakeInput(true);
		}
	}
	else if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
	{
		Move->SetHandbrakeInput(false);
	}
}

void AAIBVehicle::ApplyThrottle(float Value)
{
	if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
	{
		if (bInputLocked)
		{
			Move->SetThrottleInput(0.f);
			Move->SetBrakeInput(0.f);
			return;
		}
		// W/S on one axis: positive throttle, negative brake/reverse (auto reverse handles direction).
		Move->SetThrottleInput(FMath::Max(Value, 0.f));
		Move->SetBrakeInput(FMath::Max(-Value, 0.f));
	}
}

void AAIBVehicle::ApplySteering(float Value)
{
	if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
	{
		Move->SetSteeringInput(bInputLocked ? 0.f : Value);
	}
}

void AAIBVehicle::ApplyBrake(float Value)
{
	if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
	{
		Move->SetBrakeInput(Value);
	}
}

void AAIBVehicle::ApplyHandbrake(bool bOn)
{
	if (UChaosWheeledVehicleMovementComponent* Move = GetChaos())
	{
		Move->SetHandbrakeInput(bOn || bInputLocked);
	}
}

void AAIBVehicle::LookAround(float Yaw)
{
	LookYaw = FMath::Clamp(LookYaw + Yaw, -120.f, 120.f);
}

float AAIBVehicle::GetSpeedKmh() const
{
	return GetVelocity().Size() * 0.036f;
}

bool AAIBVehicle::IsUpsideDown() const
{
	return FVector::DotProduct(GetActorUpVector(), FVector::UpVector) < 0.2f;
}

void AAIBVehicle::ResetTo(const FTransform& Transform)
{
	if (!HasAuthority())
	{
		return;
	}
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);
	StuckTime = 0.f;
}

void AAIBVehicle::ServerRequestReset_Implementation()
{
	// Upright in place, a little higher, keep heading.
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastResetRequest < 3.f)
	{
		return;
	}
	LastResetRequest = Now;
	const FRotator Yaw(0.f, GetActorRotation().Yaw, 0.f);
	ResetTo(FTransform(Yaw, GetActorLocation() + FVector(0.f, 0.f, 150.f)));
}

bool AAIBVehicle::EnsureWheelBodiesDetached()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UChaosWheeledVehicleMovementComponent* Move = GetChaos();
	UPhysicsAsset* PhysicsAsset = MeshComp ? MeshComp->GetPhysicsAsset() : nullptr;
	if (!Move || !PhysicsAsset || MeshComp->Bodies.Num() == 0)
	{
		return false;
	}
	int32 Terminated = 0;
	for (const FChaosWheelSetup& Wheel : Move->WheelSetups)
	{
		const int32 BodyIndex = PhysicsAsset->FindBodyIndex(Wheel.BoneName);
		if (!MeshComp->Bodies.IsValidIndex(BodyIndex) || !MeshComp->Bodies[BodyIndex])
		{
			continue;
		}
		FBodyInstance* WheelBody = MeshComp->Bodies[BodyIndex];
		WheelBody->SetResponseToAllChannels(ECR_Ignore);
		WheelBody->SetInstanceSimulatePhysics(false);
		for (FConstraintInstance* Constraint : MeshComp->Constraints)
		{
			if (Constraint && (Constraint->ConstraintBone1 == Wheel.BoneName || Constraint->ConstraintBone2 == Wheel.BoneName) && Constraint->IsValidConstraintInstance())
			{
				Constraint->TermConstraint();
				++Terminated;
			}
		}
	}
	MeshComp->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
	MeshComp->WakeAllRigidBodies();
	UE_LOG(LogAIB, Log, TEXT("%s: detached wheel bodies (%d constraints released)."), *GetName(), Terminated);
	return true;
}

void AAIBVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bWheelBodiesDetached)
	{
		bWheelBodiesDetached = EnsureWheelBodiesDetached();
	}

	UChaosWheeledVehicleMovementComponent* Move = GetChaos();
	if (Move)
	{
		// Some angular damping in the air keeps jumps from turning into barrel rolls.
		GetMesh()->SetAngularDamping(Move->IsMovingOnGround() ? 0.f : 3.f);
		if (EngineAudio->IsPlaying())
		{
			const float RPM = Move->GetEngineRotationSpeed();
			const float Norm = FMath::Clamp(RPM / FMath::Max(1.f, Move->EngineSetup.MaxRPM), 0.f, 1.f);
			EngineAudio->SetPitchMultiplier(0.65f + Norm * 1.25f);
			EngineAudio->SetVolumeMultiplier(0.45f + Norm * 0.55f);
		}
	}

	// Let the camera drift back behind the car.
	LookYaw = FMath::FInterpTo(LookYaw, 0.f, DeltaSeconds, 1.5f);
	CameraBoom->SetRelativeRotation(FRotator(-12.f, LookYaw, 0.f));

	// Server: auto-recover when flipped for a while.
	if (HasAuthority() && !bInputLocked)
	{
		const bool bStuck = IsUpsideDown() || (GetSpeedKmh() < 2.f && Move && Move->GetThrottleInput() > 0.5f);
		StuckTime = bStuck ? StuckTime + DeltaSeconds : 0.f;
		if (IsUpsideDown() && StuckTime > 2.5f)
		{
			ServerRequestReset_Implementation();
		}
	}
}
