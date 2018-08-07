#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glm/glm.hpp>

#include "Character.hpp"
#include "PhysicsManager.hpp"
#include "SerializationManager.hpp"

using namespace glm;

typedef struct BlockingInfo {

	bool XAxis, YAxis, ZAxis, XPos, YPos, ZPos;

	bool IsFullyBlocked(void);

	static BlockingInfo GetAllBlocked(void);
	static BlockingInfo GetAllUnblocked(void);
} BlockingInfo;

typedef struct PoseContext {
	friend class PoseManager;
private:
	BlockingInfo Blocking;
public:
	PhysicsManager::Pinpoint Pinpoint;
} PoseContext;

typedef class PoseManager {
private:
	PoseManager(void) { };

	PhysicsManager::Pinpoint IKPinpoint;

	void PhysicsPreSolve(void);
public:
	static PoseManager& GetInstance(void) {
		static PoseManager Instance;

		return Instance;
	}

	PoseManager(PoseManager const&) = delete;
	void operator=(PoseManager const&) = delete;

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

	void Serialize(PoseSerializedState& State);
	void Deserialize(PoseSerializedState& State);
} PoseManager;