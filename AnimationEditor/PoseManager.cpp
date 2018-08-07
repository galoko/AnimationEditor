#include "PoseManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "CharacterManager.hpp"
#include "InputManager.hpp"
#include "Form.hpp"

using namespace psm;

void PoseManager::Initialize(void) {

	PhysicsManager::GetInstance().PreSolveCallback = bind(&PoseManager::PhysicsPreSolve, this);

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		Bone->PoseCtx = new PoseContext();
		Bone->PoseCtx->Blocking = BlockingInfo::GetAllUnblocked();
		Bone->PhysicBody->setDamping(1, 1);
	}
}

void PoseManager::Tick(double dt) {

	if (SerializationManager::GetInstance().IsInKinematicMode())
		return;

	PhysicsManager::GetInstance().Tick(dt);

	Form::GetInstance().UpdatePositionAndAngles();
}

void PoseManager::InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint) {

	PhysicsManager::GetInstance().SetPinpoint(IKPinpoint, Bone->PhysicBody, LocalPoint, WorldDestPoint);
}

void PoseManager::CancelInverseKinematic(void)
{
	PhysicsManager::GetInstance().SetPinpoint(IKPinpoint, nullptr, {}, {});
}

BlockingInfo PoseManager::GetBoneBlocking(Bone* Bone)
{
	if (Bone != nullptr)
		return Bone->PoseCtx->Blocking;
	else
		return BlockingInfo::GetAllUnblocked();
}

void PoseManager::SetBoneBlocking(Bone* Bone, BlockingInfo Blocking)
{
	Form::UpdateLock Lock;

	Bone->PoseCtx->Blocking = Blocking;

	PhysicsManager::GetInstance().ChangeObjectMass(Bone->PhysicBody, Blocking.IsFullyBlocked() ? 0 : Bone->Mass);

	Bone->PhysicBody->setLinearFactor(GLMToBullet({ Blocking.XPos ? 1 : 0, Blocking.YPos ? 1 : 0, Blocking.ZPos ? 1 : 0 }));

	PhysicsManager::GetInstance().UpdateBoneConstraint(Bone, !Blocking.XAxis, !Blocking.YAxis, !Blocking.ZAxis);

	Form::GetInstance().UpdateBlocking();
}

bool PoseManager::IsBonePositionConstrained(Bone* Bone)
{
	return Bone->PoseCtx->Pinpoint.IsActive();
}

void PoseManager::ConstrainBonePosition(Bone* Bone, vec3 WorldPoint)
{
	vec3 LocalPoint = inverse(Bone->WorldTransform * Bone->MiddleTranslation) * vec4(WorldPoint, 1);

	PhysicsManager::GetInstance().SetPinpoint(Bone->PoseCtx->Pinpoint, Bone->PhysicBody, LocalPoint, WorldPoint);
}

void PoseManager::RemoveBonePositionConstraint(Bone* Bone)
{
	PhysicsManager::GetInstance().SetPinpoint(Bone->PoseCtx->Pinpoint, nullptr, {}, {});
}

void PoseManager::BlockEverythingExceptThisBranch(Bone* Parent, Bone* Exception)
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

void PoseManager::UnblockAllBones(void)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	BlockingInfo AllUnblocked = BlockingInfo::GetAllUnblocked();

	for (Bone* Bone : Char->Bones)
		SetBoneBlocking(Bone, AllUnblocked);
}

void PoseManager::Serialize(PoseSerializedState& State)
{
	State.Contexts.clear();

	Character* Char = CharacterManager::GetInstance().GetCharacter();
	for (Bone* Bone : Char->Bones) {

		SerializedPoseContext SerializedContext;

		SerializedContext.BoneName = Bone->GetName();

		SerializedContext.Blocking.XPos  = Bone->PoseCtx->Blocking.XPos;
		SerializedContext.Blocking.YPos  = Bone->PoseCtx->Blocking.YPos;
		SerializedContext.Blocking.ZPos  = Bone->PoseCtx->Blocking.ZPos;
																  
		SerializedContext.Blocking.XAxis = Bone->PoseCtx->Blocking.XAxis;
		SerializedContext.Blocking.YAxis = Bone->PoseCtx->Blocking.YAxis;
		SerializedContext.Blocking.ZAxis = Bone->PoseCtx->Blocking.ZAxis;

		SerializedContext.IsActive = Bone->PoseCtx->Pinpoint.IsActive();
		SerializedContext.SrcLocalPoint = Bone->PoseCtx->Pinpoint.SrcLocalPoint;
		SerializedContext.DestWorldPoint = Bone->PoseCtx->Pinpoint.DestWorldPoint;

		State.Contexts.push_back(SerializedContext);
	}
}

void PoseManager::Deserialize(PoseSerializedState& State)
{
	Form::UpdateLock Lock;

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {

		SerializedPoseContext SerializedContext = {};
		SerializedContext.Blocking.XPos  = true;
		SerializedContext.Blocking.YPos  = true;
		SerializedContext.Blocking.ZPos  = true;
		SerializedContext.Blocking.XAxis = true;
		SerializedContext.Blocking.YAxis = true;
		SerializedContext.Blocking.ZAxis = true;

		for (SerializedPoseContext& Context : State.Contexts)
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

		PhysicsManager::GetInstance().SetPinpoint(Bone->PoseCtx->Pinpoint, SerializedContext.IsActive ? Bone->PhysicBody : nullptr, 
			SerializedContext.SrcLocalPoint, SerializedContext.DestWorldPoint);
	}

	Form::GetInstance().FullUpdate();
}

void PoseManager::PhysicsPreSolve(void) {

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
