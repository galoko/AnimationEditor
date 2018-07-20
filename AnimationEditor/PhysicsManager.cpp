#include "PhysicsManager.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/gtc/matrix_transform.hpp>

#include "CharacterManager.hpp"

void PhysicsManager::Initialize(void)
{
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	World = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	World->setGravity(btVector3(0, 0, -9.8));

	btOverlapFilterCallback* filterCallback = new YourOwnFilterCallback();
	World->getPairCache()->setOverlapFilterCallback(filterCallback);

	btContactSolverInfo& Solver = World->getSolverInfo();
	Solver.m_numIterations = 30;

	CreateFloor(4.0f, 100.0f);

	CreatePhysicsForCharacter();
}

void PhysicsManager::CreatePhysicsForCharacter(void) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		float Volume = Bone->Size.x * Bone->Size.y * Bone->Size.z;
		const float Density = 1900;
		float Mass = Density * Volume;

		if (Bone->IsLocked)
			Mass = 0.0f;

		mat4 Transform = Bone->WorldTransform * Bone->MiddleTranslation;

		Bone->PhysicBody = AddDynamicBox(Transform, Bone->Size, Mass);
		Bone->PhysicBody->setUserPointer((void*)Bone);
	}

	for (Bone* Parent : Char->Bones)
		for (Bone* Child : Parent->Childs) {

			Parent->PhysicBody->setActivationState(DISABLE_DEACTIVATION);
			Child->PhysicBody->setActivationState(DISABLE_DEACTIVATION);

			vec4 Zero = vec4(0, 0, 0, 1);

			vec3 ChildHead = Child->WorldTransform * Zero;
			vec3 ChildPosition = Child->WorldTransform * Child->MiddleTranslation * Zero;
			vec3 ParentPosition = Parent->WorldTransform * Parent->MiddleTranslation * Zero;

			vec3 ChildLocalPoint = ChildHead - ChildPosition;
			vec3 ParentLocalPoint = ChildHead - ParentPosition;

			AddConstraint(Parent, Child, ParentLocalPoint, ChildLocalPoint);
		}
}

void PhysicsManager::AddConstraint(Bone* Parent, Bone* Child, vec3 ParentLocalPoint, vec3 ChildLocalPoint)
{
	btVector3 BulletChildLocalPoint = GLMToBullet(ChildLocalPoint);
	btVector3 BulletParentLocalPoint = GLMToBullet(ParentLocalPoint);

	btTransform BulletChildFrame;
	BulletChildFrame.setIdentity();
	BulletChildFrame.setOrigin(BulletChildLocalPoint);

	btTransform BulletParentFrame;
	BulletParentFrame.setIdentity();
	BulletParentFrame.setOrigin(BulletParentLocalPoint);

	vec3 LowLimit = Child->LowLimit;
	vec3 HighLimit = Child->HighLimit;

	if (Child->IsFixed()) {

		btFixedConstraint* Constraint = new btFixedConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
		World->addConstraint(Constraint, true);
	}
	else
	if (Child->IsOnlyXRotation())
	{
		btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
			btVector3(1, 0, 0), btVector3(1, 0, 0));
		Constraint->setLimit(LowLimit.x, HighLimit.x);
		World->addConstraint(Constraint, true);
	}
	else
	if (Child->IsOnlyYRotation())
	{
		btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
			btVector3(0, 1, 0), btVector3(0, 1, 0));
		Constraint->setLimit(LowLimit.y, HighLimit.y);
		World->addConstraint(Constraint, true);
	}
	else
	if (Child->IsOnlyZRotation())
	{
		btHingeConstraint* Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
			btVector3(0, 0, 1), btVector3(0, 0, 1));
		Constraint->setLimit(LowLimit.z, HighLimit.z);
		World->addConstraint(Constraint, true);
	}
	else
	// generic 6DOF 
	{
		ApplyGimbalLockFix(LowLimit, HighLimit, BulletParentFrame, BulletChildFrame);

		btGeneric6DofSpring2Constraint* Constraint =
			new btGeneric6DofSpring2Constraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);
		Constraint->setLinearLowerLimit(btVector3(0, 0, 0));
		Constraint->setLinearUpperLimit(btVector3(0, 0, 0));

		Constraint->setAngularLowerLimit(btVector3(LowLimit.x, LowLimit.y, LowLimit.z));
		Constraint->setAngularUpperLimit(btVector3(HighLimit.x, HighLimit.y, HighLimit.z));

		for (int i = 0; i < 6; i++)
			Constraint->setStiffness(i, 0);
		World->addConstraint(Constraint, true);
	}
}

void PhysicsManager::ApplyGimbalLockFix(vec3& LowLimit, vec3& HighLimit, btTransform& ParentFrame, btTransform& ChildFrame)
{
	vec3 Ranges = vec3(
		max(fabs(LowLimit.x), fabs(HighLimit.x)),
		max(fabs(LowLimit.y), fabs(HighLimit.y)),
		max(fabs(LowLimit.z), fabs(HighLimit.z))
	);

	float MinRange = min(min(Ranges.x, Ranges.y), Ranges.z);
	float MaxRange = max(max(Ranges.x, Ranges.y), Ranges.z);

	if (MinRange > radians(80.0f))
		throw new runtime_error("Too large angle range");

	// switch Y -> X
	if (MinRange == Ranges.x) {

		ParentFrame.getBasis().setEulerZYX(0, 0, M_PI_2);
		ChildFrame.getBasis().setEulerZYX(0, 0, M_PI_2);

		swap(LowLimit.x, LowLimit.y);
		swap(HighLimit.x, HighLimit.y);

		swap(LowLimit.y, HighLimit.y);
		LowLimit.y = -LowLimit.y;
		HighLimit.y = -HighLimit.y;
	}
	else
	// switch Y -> Z
	if (MinRange == Ranges.z) {

		ParentFrame.getBasis().setEulerZYX(-M_PI_2, 0, 0);
		ChildFrame.getBasis().setEulerZYX(-M_PI_2, 0, 0);

		swap(LowLimit.z, LowLimit.y);
		swap(HighLimit.z, HighLimit.y);

		swap(LowLimit.y, HighLimit.y);
		LowLimit.y = -LowLimit.y;
		HighLimit.y = -HighLimit.y;
	}
	else
		assert(Ranges.y == MinRange);
}

void PhysicsManager::CreateFloor(float FloorSize2D, float FloorHeight)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	FloorPosition = vec3(0, 0, -FloorHeight * 0.5f + Char->FloorZ);
	FloorSize = vec3(FloorSize2D, FloorSize2D, FloorHeight);

	btRigidBody* Floor = AddStaticBox(translate(mat4(1.0f), FloorPosition), FloorSize);
	Floor->setUserPointer((void*)SOLID_ID);
}

void PhysicsManager::Tick(double dt) {

	PhysicsTime += dt;

	uint64 StepCount = (uint64)(PhysicsTime * PHYSICS_FPS);

	// skip steps (especially useful after breakpoint wake up)
	if (DoneStepCount + MaxStepsPerTick < StepCount)
		DoneStepCount = StepCount - MaxStepsPerTick;

	for (; DoneStepCount < StepCount; DoneStepCount++) 
		World->stepSimulation(fixed_dt, 0, fixed_dt);

	SyncCharacterWithWorld();
}

void PhysicsManager::SyncCharacterWithWorld(void) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	// apply changes to bones
	for (Bone* Bone : Char->Bones) {

		btTransform BulletTransfrom;
		Bone->PhysicBody->getMotionState()->getWorldTransform(BulletTransfrom);

		Bone->WorldTransform = BulletToGLM(BulletTransfrom) * inverse(Bone->MiddleTranslation);
	}
	Char->UpdateRotationsFromWorldTransforms();
}

btRigidBody* PhysicsManager::AddDynamicBox(mat4 Transform, vec3 Size, float Mass)
{
	Size *= 0.5f;
	btCollisionShape* Shape = new btBoxShape(btVector3(Size.x, Size.y, Size.z));

	btTransform BulletTransform = GLMToBullet(Transform);

	bool IsDynamic = (Mass > 0);

	btVector3 LocalInertia(0, 0, 0);
	if (IsDynamic)
		Shape->calculateLocalInertia(Mass, LocalInertia);

	btDefaultMotionState* MotionState = new btDefaultMotionState(BulletTransform);
	btRigidBody::btRigidBodyConstructionInfo BodyDef(Mass, MotionState, Shape, LocalInertia);
	btRigidBody* Body = new btRigidBody(BodyDef);

	float MinSize = min(min(Size.x, Size.y), Size.z);
	float MaxSize = max(max(Size.x, Size.y), Size.z);
	// Body->setCcdMotionThreshold(MinSize * 0.1);
	// Body->setCcdSweptSphereRadius(MinSize * 0.1f);
	Body->setFriction(1.0);
	Body->setRestitution(0.0);

	World->addRigidBody(Body);

	return Body;
}

btRigidBody* PhysicsManager::AddStaticBox(mat4 Transform, vec3 Size)
{
	return AddDynamicBox(Transform, Size, 0.0f);
}

vec3 PhysicsManager::GetFloorPosition(void)
{
	return FloorPosition;
}

vec3 PhysicsManager::GetFloorSize(void)
{
	return FloorSize;
}

void PhysicsManager::GetBoneFromRay(vec3 RayStart, vec3 RayDirection, Bone*& TouchedBone, vec3& WorldPoint, vec3& WorldNormal) {

	vec3 StartPoint = RayStart;
	vec3 EndPoint = StartPoint + RayDirection * 1000.0f;

	btCollisionWorld::ClosestRayResultCallback RayCallback(GLMToBullet(StartPoint), GLMToBullet(EndPoint));

	World->rayTest(GLMToBullet(StartPoint), GLMToBullet(EndPoint), RayCallback);

	TouchedBone = NULL;
	WorldPoint = {};
	WorldNormal = {};

	if (RayCallback.hasHit()) {

		Bone* Selected = (Bone*)RayCallback.m_collisionObject->getUserPointer();
		if (Selected != nullptr && Selected != SOLID_ID) {

			TouchedBone = Selected;
			WorldPoint = BulletToGLM(RayCallback.m_hitPointWorld);
			WorldNormal = BulletToGLM(RayCallback.m_hitNormalWorld);
		}
	}
}

// Collision filter

bool PhysicsManager::YourOwnFilterCallback::needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
{
	btCollisionObject* obj0 = static_cast<btCollisionObject*>(proxy0->m_clientObject);
	btCollisionObject* obj1 = static_cast<btCollisionObject*>(proxy1->m_clientObject);

	Bone* Bone0 = (Bone*)obj0->getUserPointer();
	Bone* Bone1 = (Bone*)obj1->getUserPointer();

	// some wierd shit
	if (Bone0 == nullptr || Bone1 == nullptr)
		return false;

	// either one is solid
	if (Bone0 == PhysicsManager::GetInstance().SOLID_ID || Bone1 == PhysicsManager::GetInstance().SOLID_ID)
		return true;

	// if we have parent->child collision - filter it out
	if (Bone0->Parent == Bone1 || Bone1->Parent == Bone0)
		return false;

	// non parent bones = collision
	return true;
}

// Bullet <-> GLM conversion utils

mat4 PhysicsManager::BulletToGLM(btTransform t) {

	btScalar matrix[16];

	t.getOpenGLMatrix(matrix);

	return mat4(
		matrix[0], matrix[1], matrix[2], matrix[3],
		matrix[4], matrix[5], matrix[6], matrix[7],
		matrix[8], matrix[9], matrix[10], matrix[11],
		matrix[12], matrix[13], matrix[14], matrix[15]);
}

vec3 PhysicsManager::BulletToGLM(btVector3 v) {

	return vec3(v.getX(), v.getY(), v.getZ());
}

btTransform PhysicsManager::GLMToBullet(mat4 m) {

	btScalar matrix[16];

	matrix[0] = m[0][0];
	matrix[1] = m[0][1];
	matrix[2] = m[0][2];
	matrix[3] = m[0][3];
	matrix[4] = m[1][0];
	matrix[5] = m[1][1];
	matrix[6] = m[1][2];
	matrix[7] = m[1][3];
	matrix[8] = m[2][0];
	matrix[9] = m[2][1];
	matrix[10] = m[2][2];
	matrix[11] = m[2][3];
	matrix[12] = m[3][0];
	matrix[13] = m[3][1];
	matrix[14] = m[3][2];
	matrix[15] = m[3][3];

	btTransform Result;
	Result.setFromOpenGLMatrix(matrix);

	return Result;
}

btVector3 PhysicsManager::GLMToBullet(vec3 v) {

	return btVector3(v.x, v.y, v.z);
}