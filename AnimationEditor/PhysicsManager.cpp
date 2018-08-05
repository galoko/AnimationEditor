#include "PhysicsManager.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/gtc/matrix_transform.hpp>

#include "CharacterManager.hpp"

void PhysicsManager::Initialize(void)
{
	btBroadphaseInterface* Broadphase = new btDbvtBroadphase();

	btDefaultCollisionConfiguration* CollisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* Dispatcher = new btCollisionDispatcher(CollisionConfiguration);

	btSequentialImpulseConstraintSolver* Solver = new btSequentialImpulseConstraintSolver;

	World = new btDiscreteDynamicsWorld(Dispatcher, Broadphase, Solver, CollisionConfiguration);
	World->setGravity(btVector3(0, 0, 0));

	btOverlapFilterCallback* filterCallback = new YourOwnFilterCallback();
	World->getPairCache()->setOverlapFilterCallback(filterCallback);

	btContactSolverInfo& SolverInfo = World->getSolverInfo();
	SolverInfo.m_numIterations = 30;

	CreateFloor(4.0f, 100.0f);

	CreatePhysicsForCharacter();
}

void PhysicsManager::CreatePhysicsForCharacter(void) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		float Volume = Bone->Size.x * Bone->Size.y * Bone->Size.z;
		const float Density = 1900;
		Bone->Mass = Density * Volume;

		mat4 Transform = Bone->WorldTransform * Bone->MiddleTranslation;

		Bone->PhysicBody = AddDynamicBox(Transform, Bone->Size, Bone->Mass);
		Bone->PhysicBody->setUserPointer((void*)Bone);
	}

	for (Bone* Child : Char->Bones)
		UpdateBoneConstraint(Child, false, false, false);
}

void PhysicsManager::ChangeObjectMass(btRigidBody* Body, float NewMass)
{
	World->removeRigidBody(Body);

	btVector3 Inertia;
	Body->getCollisionShape()->calculateLocalInertia(NewMass, Inertia);
	Body->setMassProps(NewMass, Inertia);

	//Add the rigid body to the dynamics world
	World->addRigidBody(Body);
}

void PhysicsManager::UpdateBoneConstraint(Bone* Child, bool XAxisBlocked, bool YAxisBlocked, bool ZAxisBlocked)
{
	Bone* Parent = Child->Parent;
	if (Parent == nullptr) {

		Child->PhysicBody->setAngularFactor(GLMToBullet({ XAxisBlocked ? 0 : 1, YAxisBlocked ? 0 : 1, ZAxisBlocked ? 0 : 1 }));
		return;
	}

	btVector3 BulletChildLocalPoint = GLMToBullet(Child->JointLocalPoint);
	btVector3 BulletParentLocalPoint = GLMToBullet(Child->ParentJointLocalPoint);

	btTransform BulletChildFrame;
	BulletChildFrame.setIdentity();
	BulletChildFrame.setOrigin(BulletChildLocalPoint);

	btTransform BulletParentFrame;
	BulletParentFrame.setIdentity();
	BulletParentFrame.setOrigin(BulletParentLocalPoint);

	vec3 LowLimit = Child->LowLimit;
	vec3 HighLimit = Child->HighLimit;

	if (Child->IsFixed()) {

		if (Child->PhysicConstraint == nullptr) {

			btFixedConstraint* Constraint = new btFixedConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);

			Child->PhysicConstraint = Constraint;
			World->addConstraint(Child->PhysicConstraint, true);
		}
	}
	else
	if (Child->IsOnlyXRotation())
	{
		btHingeConstraint* Constraint;
		
		if (Child->PhysicConstraint == nullptr) {
			Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
				btVector3(1, 0, 0), btVector3(1, 0, 0));

			Child->PhysicConstraint = Constraint;
			World->addConstraint(Child->PhysicConstraint, true);
		}
		else
			Constraint = (btHingeConstraint*)Child->PhysicConstraint;

		if (XAxisBlocked) {

			btScalar Angle = Constraint->getHingeAngle();

			Constraint->setLimit(Angle, Angle);
		}
		else
			Constraint->setLimit(LowLimit.x, HighLimit.x);
	}
	else
	if (Child->IsOnlyYRotation())
	{
		btHingeConstraint* Constraint;
		
		if (Child->PhysicConstraint == nullptr) {

			Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
				btVector3(0, 1, 0), btVector3(0, 1, 0));
			
			Child->PhysicConstraint = Constraint;
			World->addConstraint(Child->PhysicConstraint, true);
		}
		else
			Constraint = (btHingeConstraint*)Child->PhysicConstraint;
		
		if (YAxisBlocked) {

			btScalar Angle = Constraint->getHingeAngle();

			Constraint->setLimit(Angle, Angle);
		}
		else
			Constraint->setLimit(LowLimit.y, HighLimit.y);
	}
	else
	if (Child->IsOnlyZRotation())
	{
		btHingeConstraint* Constraint;
		
		if (Child->PhysicConstraint == nullptr) {

			Constraint = new btHingeConstraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentLocalPoint, BulletChildLocalPoint,
				btVector3(0, 0, 1), btVector3(0, 0, 1));

			Child->PhysicConstraint = Constraint;
			World->addConstraint(Child->PhysicConstraint, true);
		}
		else
			Constraint = (btHingeConstraint*)Child->PhysicConstraint;

		if (ZAxisBlocked) {

			btScalar Angle = Constraint->getHingeAngle();

			Constraint->setLimit(Angle, Angle);
		}
		else
			Constraint->setLimit(LowLimit.z, HighLimit.z);
	}
	else
	// generic 6DOF 
	{
		ApplyGimbalLockFix(LowLimit, HighLimit, BulletParentFrame, BulletChildFrame, XAxisBlocked, YAxisBlocked, ZAxisBlocked);

		btGeneric6DofSpring2Constraint* Constraint;

		if (Child->PhysicConstraint == nullptr) {

			Constraint = new btGeneric6DofSpring2Constraint(*Parent->PhysicBody, *Child->PhysicBody, BulletParentFrame, BulletChildFrame);

			Constraint->setLinearLowerLimit(btVector3(0, 0, 0));
			Constraint->setLinearUpperLimit(btVector3(0, 0, 0));

			for (int i = 0; i < 6; i++)
				Constraint->setStiffness(i, 0);

			Child->PhysicConstraint = Constraint;
			World->addConstraint(Child->PhysicConstraint, true);
		}
		else
			Constraint = (btGeneric6DofSpring2Constraint*)Child->PhysicConstraint;

		if (XAxisBlocked || YAxisBlocked || ZAxisBlocked) {

			Constraint->calculateTransforms();

			vec3 CurrentAngles = { Constraint->getAngle(0), Constraint->getAngle(1), Constraint->getAngle(2) };

			if (XAxisBlocked) {
				LowLimit.x = CurrentAngles.x;
				HighLimit.x = CurrentAngles.x;
			}

			if (YAxisBlocked) {
				LowLimit.y = CurrentAngles.y;
				HighLimit.y = CurrentAngles.y;
			}

			if (ZAxisBlocked) {
				LowLimit.z = CurrentAngles.z;
				HighLimit.z = CurrentAngles.z;
			}
		}

		Constraint->setAngularLowerLimit(GLMToBullet(LowLimit));
		Constraint->setAngularUpperLimit(GLMToBullet(HighLimit));
	}
}

vec3 PhysicsManager::GetBoneAngles(Bone* Bone)
{
	float NaN = nanf("");

	if (Bone->PhysicConstraint == nullptr) {

		quat Q = quat_cast(Bone->Rotation);
		vec3 angles = -eulerAngles(Q);

		return angles;
	}

	if (Bone->IsFixed())
		return vec3(NaN); // ?
	else 
	if (Bone->IsOnlyXRotation()) {

		btHingeConstraint* Constraint = (btHingeConstraint*)Bone->PhysicConstraint;

		return { Constraint->getHingeAngle(), NaN, NaN };
	}
	else
	if (Bone->IsOnlyYRotation()) {

		btHingeConstraint* Constraint = (btHingeConstraint*)Bone->PhysicConstraint;

		return { NaN, Constraint->getHingeAngle(), NaN };
	}
	else
	if (Bone->IsOnlyZRotation()) {

		btHingeConstraint* Constraint = (btHingeConstraint*)Bone->PhysicConstraint;

		return { NaN, NaN, Constraint->getHingeAngle() };
	}
	else {

		btGeneric6DofSpring2Constraint* Constraint = (btGeneric6DofSpring2Constraint*)Bone->PhysicConstraint;

		Constraint->calculateTransforms();

		vec3 CurrentAngles = { Constraint->getAngle(0), Constraint->getAngle(1), Constraint->getAngle(2) };

		ReverseGimbalLockFix(Bone->LowLimit, Bone->HighLimit, CurrentAngles);

		return CurrentAngles;
	}
}

void PhysicsManager::SetBoneAngles(Bone* Bone, vec3 Angles)
{
	bool SavedXAxisBlocked, SavedYAxisBlocked, SavedZAxisBlocked;

	SavedXAxisBlocked = Bone->XAxisBlocked;
	SavedYAxisBlocked = Bone->YAxisBlocked;
	SavedZAxisBlocked = Bone->ZAxisBlocked;

	UpdateBoneConstraint(Bone, false, false, false);

	Angles = clamp(Angles, Bone->LowLimit, Bone->HighLimit);

	mat4 RotationX, RotationY, RotationZ;

	if (!isnan(Angles.x))
		RotationX = rotate(mat4(1.0f), Angles.x, vec3(-1, 0, 0));
	else
		RotationX = mat4(1.0f);

	if (!isnan(Angles.y))
		RotationY = rotate(mat4(1.0f), Angles.y, vec3(0, -1, 0));
	else
		RotationY = mat4(1.0f);

	if (!isnan(Angles.z))
		RotationZ = rotate(mat4(1.0f), Angles.z, vec3(0, 0, -1));
	else
		RotationZ = mat4(1.0f);

	GimbalLockFixType FixType;

	if (Bone->Parent != nullptr)
		FixType = GetGimbalLockFixType(Bone->LowLimit, Bone->HighLimit);
	else
		FixType = None;

	if (FixType == XtoY)
		Bone->Rotation = RotationZ * RotationX * RotationY;
	else
	if (FixType == ZtoY)
		Bone->Rotation = RotationY * RotationZ * RotationX;
	else
		Bone->Rotation = RotationZ * RotationY * RotationX;

	SyncWorldWithCharacter();

	UpdateBoneConstraint(Bone, SavedXAxisBlocked, SavedYAxisBlocked, SavedZAxisBlocked);
}

PhysicsManager::GimbalLockFixType PhysicsManager::GetGimbalLockFixType(vec3 LowLimit, vec3 HighLimit)
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
	if (MinRange == Ranges.x) 
		return XtoY;
	else
	// switch Y -> Z
	if (MinRange == Ranges.z) 
		return ZtoY;
	else {
		assert(Ranges.y == MinRange);
		return None;
	}
}

void PhysicsManager::ApplyGimbalLockFix(vec3& LowLimit, vec3& HighLimit, btTransform& ParentFrame, btTransform& ChildFrame,
	bool& XBlocked, bool& YBlocked, bool &ZBlocked)
{
	GimbalLockFixType FixType = GetGimbalLockFixType(LowLimit, HighLimit);

	// switch Y -> X
	if (FixType == XtoY) {

		ParentFrame.getBasis().setEulerYPR(M_PI_2, 0, 0);
		ChildFrame.getBasis().setEulerYPR(M_PI_2, 0, 0);

		swap(LowLimit.x, LowLimit.y);
		swap(HighLimit.x, HighLimit.y);

		swap(LowLimit.y, HighLimit.y);
		LowLimit.y = -LowLimit.y;
		HighLimit.y = -HighLimit.y;

		swap(XBlocked, YBlocked);
	}
	else
	// switch Y -> Z
	if (FixType == ZtoY) {

		ParentFrame.getBasis().setEulerYPR(0, 0, -M_PI_2);
		ChildFrame.getBasis().setEulerYPR(0, 0, -M_PI_2);

		swap(LowLimit.z, LowLimit.y);
		swap(HighLimit.z, HighLimit.y);

		swap(LowLimit.y, HighLimit.y);
		LowLimit.y = -LowLimit.y;
		HighLimit.y = -HighLimit.y;

		swap(ZBlocked, YBlocked);
	}
}

void PhysicsManager::ReverseGimbalLockFix(vec3 LowLimit, vec3 HighLimit, vec3& Angles)
{
	GimbalLockFixType FixType = GetGimbalLockFixType(LowLimit, HighLimit);

	if (FixType == XtoY) {

		Angles.y = -Angles.y;
		swap(Angles.x, Angles.y);
	}
	else
	if (FixType == ZtoY) {

		Angles.y = -Angles.y;
		swap(Angles.z, Angles.y);
	}
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

	for (; DoneStepCount < StepCount; DoneStepCount++) {

		if (PreSolveCallback != nullptr)
			PreSolveCallback();

		World->stepSimulation(fixed_dt, 0, fixed_dt);

		if (PostSolveCallback != nullptr)
			PostSolveCallback();
	}

	SyncCharacterWithWorld();
}

void PhysicsManager::SyncCharacterWithWorld(void) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	// apply changes to bones
	for (Bone* Bone : Char->Bones)
		Bone->WorldTransform = GetBoneWorldTransform(Bone);

	Char->UpdateRotationsFromWorldTransforms();
}

void PhysicsManager::SyncWorldWithCharacter(void)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();
	Char->UpdateWorldTranforms();

	// apply changes to bones
	for (Bone* Bone : Char->Bones)
		Bone->PhysicBody->setWorldTransform(GLMToBullet(Bone->WorldTransform * Bone->MiddleTranslation));
}

void PhysicsManager::MirrorCharacter(void)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* CurrentBone : Char->Bones) {

		vec3 Angles = GetBoneAngles(CurrentBone);

		if (CurrentBone->Side == Bone::Center) {

			Angles *= vec3(-1, 1, 1);

			SetBoneAngles(CurrentBone, Angles);
		}
		else
		if (CurrentBone->Side == Bone::Left) {
			Bone* OtherBone = Char->FindOtherBone(CurrentBone);
			if (OtherBone != nullptr) {

				vec3 OtherAngles = GetBoneAngles(OtherBone);

				vec3 MirrorVector = vec3(-1, 1, -1);

				SetBoneAngles(CurrentBone, OtherAngles * MirrorVector);
				SetBoneAngles(OtherBone, Angles * MirrorVector);
			}
		}
	}
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

	Body->setActivationState(DISABLE_DEACTIVATION);

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
		if (Selected != nullptr && Selected != SOLID_ID && Selected != NON_SOLID_ID) {

			TouchedBone = Selected;
			WorldPoint = BulletToGLM(RayCallback.m_hitPointWorld);
			WorldNormal = BulletToGLM(RayCallback.m_hitNormalWorld);
		}
	}
}

void PhysicsManager::SetPinpoint(Pinpoint& P, btRigidBody* Body, vec3 LocalPoint, vec3 WorldPoint)
{
	if (Body != P.SrcBody && P.Constraint != nullptr) {
		World->removeConstraint(P.Constraint);
		delete P.Constraint;
		P.Constraint = nullptr;
	}

	P.SrcBody = Body;

	if (Body == nullptr) {

		if (P.DummyBody != nullptr) {
			World->removeRigidBody(P.DummyBody);
			delete P.DummyBody;
			P.DummyBody = nullptr;
		}

		return;
	}

	if (P.DummyBody == nullptr) {
		P.DummyBody = AddStaticBox(mat4(1.0f), {});
		P.DummyBody->setUserPointer((void*)NON_SOLID_ID);
	}

	if (P.Constraint == nullptr) {

		P.Constraint = new btPoint2PointConstraint(*P.DummyBody, *P.SrcBody, GLMToBullet(vec3(0, 0, 0)), GLMToBullet(vec3(0, 0, 0)));

		P.Constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.5f);
		P.Constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f);

		World->addConstraint(P.Constraint);
	}

	P.SrcLocalPoint = LocalPoint;
	P.DestWorldPoint = WorldPoint;

	P.Constraint->setPivotB(GLMToBullet(P.SrcLocalPoint));

	mat4 DestTransform = translate(mat4(1.0f), P.DestWorldPoint);
	P.DummyBody->setWorldTransform(GLMToBullet(DestTransform));
}

mat4 PhysicsManager::GetBoneWorldTransform(Bone* Bone)
{
	btTransform BulletTransfrom = Bone->PhysicBody->getWorldTransform();

	mat4 WorldTransform = BulletToGLM(BulletTransfrom) * inverse(Bone->MiddleTranslation);

	return WorldTransform;
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

	// either one is non solid
	if (Bone0 == PhysicsManager::GetInstance().NON_SOLID_ID || Bone1 == PhysicsManager::GetInstance().NON_SOLID_ID)
		return false;

	// if we have parent->child collision - filter it out
	if (Bone0->Parent == Bone1 || Bone1->Parent == Bone0)
		return false;

	// non parent bones = collision
	return true;
}

// Bullet <-> GLM conversion utils

namespace psm {

	mat4 BulletToGLM(btTransform t) {

		btScalar matrix[16];

		t.getOpenGLMatrix(matrix);

		return mat4(
			matrix[0], matrix[1], matrix[2], matrix[3],
			matrix[4], matrix[5], matrix[6], matrix[7],
			matrix[8], matrix[9], matrix[10], matrix[11],
			matrix[12], matrix[13], matrix[14], matrix[15]);
	}

	vec3 BulletToGLM(btVector3 v) {

		return vec3(v.getX(), v.getY(), v.getZ());
	}

	btTransform GLMToBullet(mat4 m) {

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

	btVector3 GLMToBullet(vec3 v) {

		return btVector3(v.x, v.y, v.z);
	}

}

// Pinpoint

bool PhysicsManager::Pinpoint::IsActive(void)
{
	return Constraint != nullptr;
}
