#pragma once

#include <windows.h>

#include <glm/glm.hpp>

#include "Character.hpp"

using namespace glm;

typedef struct BlockingInfo {

	bool XAxis, YAxis, ZAxis, XPos, YPos, ZPos;

	bool IsFullyBlocked(void);

	static BlockingInfo GetAllBlocked(void);
	static BlockingInfo GetAllUnblocked(void);
} BlockingInfo;

typedef struct AnimationManagerInfo {

	btPoint2PointConstraint* PositionConstraint;
	btRigidBody* ConstraintDummy;

	BlockingInfo Blocking;

} AnimationManagerInfo;

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

	BlockingInfo GetBoneBlocking(Bone* Bone);
	void SetBoneBlocking(Bone* Bone, BlockingInfo Blocking);

	bool IsBonePositionConstrained(Bone* Bone);
	void ConstrainBonePosition(Bone* Bone, vec3 WorldPoint);
	void RemoveBonePositionConstraint(Bone* Bone);

	void BlockEverythingExceptThisBranch(Bone* Parent, Bone* Exception);
	void UnblockAllBones(void);
} AnimationManager;