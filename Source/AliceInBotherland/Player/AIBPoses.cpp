// Alice in Botherland - story poses: procedural body language for the story's big moments

#include "Player/AIBPoses.h"
#include "Components/PoseableMeshComponent.h"

namespace
{
	// Directions in the mesh's component space (the mannequin faces +Y; the mesh is turned -90 degrees in the capsule).
	const FVector PoseFwd(0.f, 1.f, 0.f);
	const FVector PoseLeft(1.f, 0.f, 0.f);
	const FVector PoseRight(-1.f, 0.f, 0.f);
	const FVector PoseUp(0.f, 0.f, 1.f);

	/**
	 * Turns a bone by Degrees about an axis given in component space (so it doesn't depend on how each bone's own axes
	 * are set up). Children follow, because the pose keeps them relative to their parent.
	 * Quick reference for a limb hanging down: about PoseFwd, + swings it out to the right, - to the left; about PoseLeft,
	 * + swings it forward. About PoseRight, + tips the torso or head forward (a nod / bow), - tips it back.
	 */
	void TurnBone(UPoseableMeshComponent* Mesh, FName Bone, const FVector& Axis, float Degrees)
	{
		if (FMath::IsNearlyZero(Degrees) || Mesh->GetBoneIndex(Bone) == INDEX_NONE)
		{
			return;
		}
		FTransform Transform = Mesh->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace);
		Transform.SetRotation(FQuat(Axis, FMath::DegreesToRadians(Degrees)) * Transform.GetRotation());
		Mesh->SetBoneTransformByName(Bone, Transform, EBoneSpaces::ComponentSpace);
	}

	float Oscillate(float Time, float Hz)
	{
		return FMath::Sin(Time * Hz * 2.f * PI);
	}
}

namespace AIBPoses
{
	void Apply(UPoseableMeshComponent* Mesh, EAIBPose Pose, float Time, float Weight)
	{
		if (!Mesh || Weight <= 0.001f)
		{
			return;
		}
		const float W = Weight;
		switch (Pose)
		{
		case EAIBPose::Grief:
		{
			// Face in hands, shoulders slumped and shaking with each breath.
			const float Sob = FMath::Max(0.f, Oscillate(Time, 0.9f));
			TurnBone(Mesh, TEXT("spine_02"), PoseRight, (10.f + 2.f * Sob) * W);
			TurnBone(Mesh, TEXT("spine_03"), PoseRight, 12.f * W);
			TurnBone(Mesh, TEXT("clavicle_r"), PoseFwd, 4.f * Sob * W);
			TurnBone(Mesh, TEXT("clavicle_l"), PoseFwd, -4.f * Sob * W);
			TurnBone(Mesh, TEXT("head"), PoseRight, 28.f * W);
			TurnBone(Mesh, TEXT("head"), PoseUp, 6.f * Oscillate(Time, 0.35f) * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseLeft, 62.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseLeft, 62.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseUp, -16.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseUp, 16.f * W);
			TurnBone(Mesh, TEXT("lowerarm_r"), PoseLeft, 112.f * W);
			TurnBone(Mesh, TEXT("lowerarm_l"), PoseLeft, 112.f * W);
			break;
		}
		case EAIBPose::Judged:
		{
			// Hands drop, palms a little open; the head tips back to face the light coming down.
			const float Rise = FMath::SmoothStep(0.f, 0.8f, Time);
			TurnBone(Mesh, TEXT("spine_04"), PoseRight, -8.f * Rise * W);
			TurnBone(Mesh, TEXT("head"), PoseRight, -34.f * Rise * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseFwd, 16.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseFwd, -16.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseLeft, 10.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseLeft, 10.f * W);
			TurnBone(Mesh, TEXT("lowerarm_r"), PoseLeft, 18.f * W);
			TurnBone(Mesh, TEXT("lowerarm_l"), PoseLeft, 18.f * W);
			break;
		}
		case EAIBPose::Ponder:
		{
			// Hand to chin, weighing the choice; the head drifts a little.
			TurnBone(Mesh, TEXT("spine_04"), PoseRight, 6.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseLeft, 32.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseUp, -18.f * W);
			TurnBone(Mesh, TEXT("lowerarm_r"), PoseLeft, 118.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseLeft, 22.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseUp, 24.f * W);
			TurnBone(Mesh, TEXT("lowerarm_l"), PoseLeft, 70.f * W);
			TurnBone(Mesh, TEXT("head"), PoseRight, (10.f + 3.f * Oscillate(Time, 0.25f)) * W);
			TurnBone(Mesh, TEXT("head"), PoseFwd, 6.f * Oscillate(Time, 0.18f) * W);
			break;
		}
		case EAIBPose::Crowned:
		{
			// Both fists up, slowly, under the crown.
			const float Pump = 6.f * Oscillate(Time, 0.5f);
			TurnBone(Mesh, TEXT("spine_03"), PoseLeft, 8.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseFwd, (140.f + Pump) * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseFwd, -(140.f + Pump) * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseLeft, 12.f * W);
			TurnBone(Mesh, TEXT("upperarm_l"), PoseLeft, 12.f * W);
			TurnBone(Mesh, TEXT("lowerarm_r"), PoseFwd, 14.f * W);
			TurnBone(Mesh, TEXT("lowerarm_l"), PoseFwd, -14.f * W);
			TurnBone(Mesh, TEXT("head"), PoseLeft, 14.f * W);
			break;
		}
		case EAIBPose::Farewell:
		{
			// Right hand on the heart, head bowed, as the light takes them.
			const float Bow = FMath::SmoothStep(0.f, 1.2f, Time);
			TurnBone(Mesh, TEXT("spine_03"), PoseRight, 8.f * Bow * W);
			TurnBone(Mesh, TEXT("head"), PoseRight, 22.f * Bow * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseLeft, 38.f * W);
			TurnBone(Mesh, TEXT("upperarm_r"), PoseUp, -34.f * W);
			TurnBone(Mesh, TEXT("lowerarm_r"), PoseLeft, 118.f * W);
			break;
		}
		default:
			break;
		}
	}
}
