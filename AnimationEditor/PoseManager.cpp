#include "PoseManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "CharacterManager.hpp"
#include "InputManager.hpp"
#include "Form.hpp"

using namespace psm;

void AnimationManager::Initialize(void) {

	PhysicsManager::GetInstance().PreSolveCallback = bind(&AnimationManager::PhysicsPreSolve, this);

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		Bone->AnimCtx = new AnimationContext();
		Bone->AnimCtx->Blocking = BlockingInfo::GetAllUnblocked();
		Bone->PhysicBody->setDamping(1, 1);
	}
}

void AnimationManager::Tick(double dt) {

	if (SerializationManager::GetInstance().IsInKinematicMode())
		return;

	PhysicsManager::GetInstance().Tick(dt);

	Form::GetInstance().UpdatePositionAndAngles();
}

void AnimationManager::InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint) {

	PhysicsManager::GetInstance().SetPinpoint(IKPinpoint, Bone->PhysicBody, LocalPoint, WorldDestPoint);
}

void AnimationManager::CancelInverseKinematic(void)
{
	PhysicsManager::GetInstance().SetPinpoint(IKPinpoint, nullptr, {}, {});
}

BlockingInfo AnimationManager::GetBoneBlocking(Bone* Bone)
{
	if (Bone != nullptr)
		return Bone->AnimCtx->Blocking;
	else
		return BlockingInfo::GetAllUnblocked();
}

void AnimationManager::SetBoneBlocking(Bone* Bone, BlockingInfo Blocking)
{
	Form::UpdateLock Lock;

	Bone->AnimCtx->Blocking = Blocking;

	PhysicsManager::GetInstance().ChangeObjectMass(Bone->PhysicBody, Blocking.IsFullyBlocked() ? 0 : Bone->Mass);

	Bone->PhysicBody->setLinearFactor(GLMToBullet({ Blocking.XPos ? 1 : 0, Blocking.YPos ? 1 : 0, Blocking.ZPos ? 1 : 0 }));

	PhysicsManager::GetInstance().UpdateBoneConstraint(Bone, !Blocking.XAxis, !Blocking.YAxis, !Blocking.ZAxis);

	Form::GetInstance().UpdateBlocking();
}

bool AnimationManager::IsBonePositionConstrained(Bone* Bone)
{
	return Bone->AnimCtx->Pinpoint.IsActive();
}

void AnimationManager::ConstrainBonePosition(Bone* Bone, vec3 WorldPoint)
{
	vec3 LocalPoint = inverse(Bone->WorldTransform * Bone->MiddleTranslation) * vec4(WorldPoint, 1);

	PhysicsManager::GetInstance().SetPinpoint(Bone->AnimCtx->Pinpoint, Bone->PhysicBody, LocalPoint, WorldPoint);
}

void AnimationManager::RemoveBonePositionConstraint(Bone* Bone)
{
	PhysicsManager::GetInstance().SetPinpoint(Bone->AnimCtx->Pinpoint, nullptr, {}, {});
}

void AnimationManager::BlockEverythingExceptThisBranch(Bone* Parent, Bone* Exception)
{
	Form::UpdateLock Lock;

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

void AnimationManager::Serialize(AnimationSerializedState& State)
{
	State.Contexts.clear();

	Character* Char = CharacterManager::GetInstance().GetCharacter();
	for (Bone* Bone : Char->Bones) {

		SerializedAnimationContext SerializedContext;

		SerializedContext.BoneName = Bone->GetName();

		SerializedContext.Blocking.XPos  = Bone->AnimCtx->Blocking.XPos;
		SerializedContext.Blocking.YPos  = Bone->AnimCtx->Blocking.YPos;
		SerializedContext.Blocking.ZPos  = Bone->AnimCtx->Blocking.ZPos;
																  
		SerializedContext.Blocking.XAxis = Bone->AnimCtx->Blocking.XAxis;
		SerializedContext.Blocking.YAxis = Bone->AnimCtx->Blocking.YAxis;
		SerializedContext.Blocking.ZAxis = Bone->AnimCtx->Blocking.ZAxis;

		SerializedContext.IsActive = Bone->AnimCtx->Pinpoint.IsActive();
		SerializedContext.SrcLocalPoint = Bone->AnimCtx->Pinpoint.SrcLocalPoint;
		SerializedContext.DestWorldPoint = Bone->AnimCtx->Pinpoint.DestWorldPoint;

		State.Contexts.push_back(SerializedContext);
	}
}

void AnimationManager::Deserialize(AnimationSerializedState& State)
{
	Form::UpdateLock Lock;

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		SerializedAnimationContext SerializedContext = {};
		SerializedContext.Blocking.XPos  = true;
		SerializedContext.Blocking.YPos  = true;
		SerializedContext.Blocking.ZPos  = true;
		SerializedContext.Blocking.XAxis = true;
		SerializedContext.Blocking.YAxis = true;
		SerializedContext.Blocking.ZAxis = true;

		for (SerializedAnimationContext& Context : State.Contexts)
			if (Context.BoneName == Bone->GetName()) {
				SerializedContext = Context;
				break;
			}

		BlockingInfo Blocking;

		Blocking.XPos  = SerializedContext.Blocking.XPos;
		Blocking.YPos  = SerializedContext.Blocking.YPos;
		Blocking.ZPos  = SerializedContext.Blocking.ZPos;

		Blocking.XAxis = SerializedContext.Blocking.XAxis;
		Blocking.YAxis = SerializedContext.Blocking.YAxis;
		Blocking.ZAxis = SerializedContext.Blocking.ZAxis;

		SetBoneBlocking(Bone, Blocking);

		PhysicsManager::GetInstance().SetPinpoint(Bone->AnimCtx->Pinpoint, SerializedContext.IsActive ? Bone->PhysicBody : nullptr, 
			SerializedContext.SrcLocalPoint, SerializedContext.DestWorldPoint);
	}

	Form::GetInstance().FullUpdate();
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