#include "AnimationManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "PhysicsManager.hpp"
#include "CharacterManager.hpp"

using namespace psm;

void AnimationManager::Initialize(void) {

	PhysicsManager::GetInstance().PreSolveCallback = bind(&AnimationManager::PhysicsPreSolve, this);

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		Bone->AnimInfo = new AnimationManagerInfo();
		Bone->AnimInfo->Blocking = BlockingInfo::GetAllUnblocked();
		Bone->PhysicBody->setDamping(1, 1);
	}
}

void AnimationManager::Tick(double dt) {

	PhysicsManager::GetInstance().Tick(dt);
}

void AnimationManager::InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint) {

	PhysicsManager::GetInstance().SetIKConstraint(Bone->PhysicBody, LocalPoint, WorldDestPoint);
}

void AnimationManager::CancelInverseKinematic(void)
{
	PhysicsManager::GetInstance().SetIKConstraint(nullptr, {}, {});
}

BlockingInfo AnimationManager::GetBoneBlocking(Bone* Bone)
{
	if (Bone != nullptr)
		return Bone->AnimInfo->Blocking;
	else
		return BlockingInfo::GetAllUnblocked();
}

void AnimationManager::SetBoneBlocking(Bone* Bone, BlockingInfo Blocking)
{
	Bone->AnimInfo->Blocking = Blocking;

	PhysicsManager::GetInstance().ChangeObjectMass(Bone->PhysicBody, Blocking.IsFullyBlocked() ? 0 : Bone->Mass);

	Bone->PhysicBody->setLinearFactor(GLMToBullet({ Blocking.XPos ? 1 : 0, Blocking.YPos ? 1 : 0, Blocking.ZPos ? 1 : 0 }));
	Bone->PhysicBody->setAngularFactor(GLMToBullet({ Blocking.XAxis ? 1 : 0, Blocking.YAxis ? 1 : 0, Blocking.ZAxis ? 1 : 0 }));
}

bool AnimationManager::IsBonePositionConstrained(Bone* Bone)
{
	return Bone->AnimInfo->PositionConstraint != nullptr;
}

void AnimationManager::ConstrainBonePosition(Bone* Bone, vec3 WorldPoint)
{
	if (IsBonePositionConstrained(Bone))
		RemoveBonePositionConstraint(Bone);

	if (Bone->AnimInfo->ConstraintDummy == nullptr)
		Bone->AnimInfo->ConstraintDummy = PhysicsManager::GetInstance().AddStaticNonSolidBox(mat4(1.0f), {});

	mat4 PositionTransform = translate(mat4(1.0f), WorldPoint);
	Bone->AnimInfo->ConstraintDummy->setWorldTransform(GLMToBullet(PositionTransform));

	btPoint2PointConstraint* Constraint;

	vec3 LocalPoint = inverse(Bone->WorldTransform * Bone->MiddleTranslation) * vec4(WorldPoint, 1);

	Constraint = new btPoint2PointConstraint(*Bone->AnimInfo->ConstraintDummy, *Bone->PhysicBody, GLMToBullet(vec3(0)), GLMToBullet(LocalPoint));

	Constraint->setParam(BT_CONSTRAINT_STOP_CFM, 0.5f);
	Constraint->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f);

	btDiscreteDynamicsWorld* World = PhysicsManager::GetInstance().GetWorld();
	World->addConstraint(Constraint);

	Bone->AnimInfo->PositionConstraint = Constraint;
}

void AnimationManager::RemoveBonePositionConstraint(Bone* Bone)
{
	if (IsBonePositionConstrained(Bone)) {

		btDiscreteDynamicsWorld* World =  PhysicsManager::GetInstance().GetWorld();
		World->removeConstraint(Bone->AnimInfo->PositionConstraint);

		delete Bone->AnimInfo->PositionConstraint;
		Bone->AnimInfo->PositionConstraint = nullptr;
	}
}

void AnimationManager::BlockEverythingExceptThisBranch(Bone* Parent, Bone* Exception)
{
	if (Parent == nullptr)
		return;

	BlockingInfo AllBlocked = BlockingInfo::GetAllBlocked();

	SetBoneBlocking(Parent, AllBlocked);

	for (Bone* Child : Parent->Childs) {

		if (Child == Exception)
			continue;

		SetBoneBlocking(Child, AllBlocked);

		BlockEverythingExceptThisBranch(Child, nullptr);
	}
}

void AnimationManager::UnblockAllBones(void)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	BlockingInfo AllUnblocked = BlockingInfo::GetAllUnblocked();

	for (Bone* Bone : Char->Bones)
		SetBoneBlocking(Bone, AllUnblocked);
}

void AnimationManager::PhysicsPreSolve(void) {

}

// BlockingInfo

bool BlockingInfo::IsFullyBlocked(void)
{
	return !XPos && !YPos && !ZPos && !XAxis && !YAxis && !ZAxis;
}

BlockingInfo BlockingInfo::GetAllBlocked(void)
{
	BlockingInfo AllBlocked;

	AllBlocked.XPos  = false;
	AllBlocked.YPos  = false;
	AllBlocked.ZPos  = false;
						 
	AllBlocked.XAxis = false;
	AllBlocked.YAxis = false;
	AllBlocked.ZAxis = false;

	return AllBlocked;
}

BlockingInfo BlockingInfo::GetAllUnblocked(void)
{
	BlockingInfo AllUnblocked;

	AllUnblocked.XPos  = true;
	AllUnblocked.YPos  = true;
	AllUnblocked.ZPos  = true;

	AllUnblocked.XAxis = true;
	AllUnblocked.YAxis = true;
	AllUnblocked.ZAxis = true;

	return AllUnblocked;
}
