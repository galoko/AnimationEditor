#include "AnimationManager.hpp"

#include "PhysicsManager.hpp"
#include "CharacterManager.hpp"

using namespace psm;

typedef struct AnimationManagerInfo {

	bool IsBlocked;

	btPoint2PointConstraint* PositionConstraint;
	btGeneric6DofConstraint* RotationConstraint;

} AnimationManagerInfo;

void AnimationManager::Initialize(void) {

	PhysicsManager::GetInstance().PreSolveCallback = bind(&AnimationManager::PhysicsPreSolve, this);

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones)
		Bone->PhysicBody->setDamping(1, 1);
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

bool AnimationManager::IsBoneBlocked(Bone* Bone)
{
	return false;
}

void AnimationManager::PhysicsPreSolve(void) {

}
