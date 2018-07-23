#pragma once

#include <windows.h>

#include <glm/glm.hpp>

#include "Character.hpp"

using namespace glm;

typedef class AnimationManager {
private:
	AnimationManager(void) { };

	void PhysicsPreSolve(void);
public:
	static AnimationManager& GetInstance(void) {
		static AnimationManager Instance;

		return Instance;
	}

	AnimationManager(AnimationManager const&) = delete;
	void operator=(AnimationManager const&) = delete;

	void Initialize(void);
	void Tick(double dt);

	void InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint);
	void CancelInverseKinematic(void);

	bool IsBoneBlocked(Bone* Bone);
	void BlockBone(Bone* Bone);
	void UnblockBone(Bone* Bone);

	void BlockBoneParentsRecursive(Bone* Bone);
	void UnblockAllBones(void);

	void ConstraintBonePosition(Bone* Bone, vec3 WorldPoint);
	void FixBoneRotation(Bone* Bone);
	void RemoveBoneConstraints(Bone* Bone);

} AnimationManager;