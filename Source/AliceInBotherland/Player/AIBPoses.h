// Alice in Botherland - story poses: procedural body language for the story's big moments

#pragma once

#include "CoreMinimal.h"

class UPoseableMeshComponent;

/** Body language the story puts on a champion (never player-triggered). Every pose holds until replaced or stopped. */
enum class EAIBPose : uint8
{
	None,
	/** Eliminated: face in hands, shoulders shaking. */
	Grief,
	/** The Guardian's judgement gathers overhead: hands fall, the champion looks up and accepts it. */
	Judged,
	/** The final choice: hand to chin, weighing it. */
	Ponder,
	/** Became the King: both fists raised under the crown. */
	Crowned,
	/** Gave their life: hand on heart, head bowed. */
	Farewell
};

namespace AIBPoses
{
	/**
	 * Bends the pose in Mesh (already a copy of the animated pose) into Pose. Time = seconds since it started,
	 * Weight 0..1 blends from the plain animation to the full pose.
	 */
	void Apply(UPoseableMeshComponent* Mesh, EAIBPose Pose, float Time, float Weight);
}
