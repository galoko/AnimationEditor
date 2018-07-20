#pragma once

#include <windows.h>

#include <glm/glm.hpp>

#include <btBulletDynamicsCommon.h>

#include "Character.hpp"

using namespace glm;

typedef class PhysicsManager {
private:
	PhysicsManager(void) { };

#if _DEBUG
	const int PHYSICS_FPS = 100;
#else
	const int PHYSICS_FPS = 2000;
#endif

	const double fixed_dt = 1.0 / (double)PHYSICS_FPS;

	const int MaxStepsPerTick = 100;

	const void* SOLID_ID = (void*)1;

	struct YourOwnFilterCallback : public btOverlapFilterCallback
	{
		// return true when pairs need collision
		virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const;
	};

	// Integration

	double PhysicsTime;
	uint64 DoneStepCount;

	// Floor

	vec3 FloorPosition, FloorSize;

	// World

	btDiscreteDynamicsWorld* World;

	btRigidBody* AddDynamicBox(mat4 Transform, vec3 Size, float Mass);
	btRigidBody* AddStaticBox(mat4 Transform, vec3 Size);

	// Tick Utils

	void SyncCharacterWithWorld(void);

	void CreateFloor(float FloorSize2D, float FloorHeight);
	void CreatePhysicsForCharacter(void);

	void ApplyGimbalLockFix(vec3& LowLimit, vec3& HighLimit, btTransform& ParentFrame, btTransform& ChildFrame);
	void AddConstraint(Bone* Parent, Bone* Child, vec3 ParentLocalPoint, vec3 ChildLocalPoint);

	// Conversion Utils

	static mat4 BulletToGLM(btTransform t);
	static vec3 BulletToGLM(btVector3 v);

	static btTransform GLMToBullet(mat4 m);
	static btVector3 GLMToBullet(vec3 v);
public:
	static PhysicsManager& GetInstance(void) {
		static PhysicsManager Instance;

		return Instance;
	}

	PhysicsManager(PhysicsManager const&) = delete;
	void operator=(PhysicsManager const&) = delete;

	void Initialize(void);
	void Tick(double dt);

	vec3 GetFloorPosition(void);
	vec3 GetFloorSize(void);

	void GetBoneFromRay(vec3 RayStart, vec3 RayDirection, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal);
} PhysicsManager;