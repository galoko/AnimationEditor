#include "Character.hpp"

Character::Character(void)
{
	GenerateBones();
	UpdateWorldTranforms();
	UpdateFloorZ();
}

Bone* Character::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name)
{
	float CmToMeters = 0.01f;

	Bone* Result = new Bone(NextBoneID++, Name, Offset, Tail, Size * CmToMeters, Parent);

	Bones.push_back(Result);

	return Result;
}

void Character::GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection)
{
	vec3 MirrorVector = cross(MirrorDirection, cross(MirrorDirection, vec3(-1.0f))) - MirrorDirection;

	float MetersToCm = 100.0f;

	Bone* RightBone = GenerateBone(RightParent,
		LeftBone->Tail * MirrorVector,
		LeftBone->Size * MetersToCm,
		LeftBone->Offset * MirrorVector, LeftBone->Name + L" Right");

	LeftBone->Name = LeftBone->Name + L" Left";

	for (Bone* LeftChild : LeftBone->Childs)
		GenerateRightSide(LeftChild, RightBone, MirrorDirection);
}

void Character::GenerateBones(void)
{
	Spine = GenerateBone(nullptr, { 0, 0, 1.0f }, { 6.5f, 13.0f, 52.8f }, {}, L"Spine");

	Bone* Head = GenerateBone(Spine, { 0, 0, 1, }, { 15.0f, 15.0f, 20.0f }, { 0, 0.0f, 57.8f / 52.8f }, L"Head");

	Bone* UpperLeg = GenerateBone(Spine, { 0, 0, -1.0f }, { 6.5f, 6.5f, 46.0f }, { 0, 0.5f, 0 }, L"Upper Leg");
	Bone* LowerLeg = GenerateBone(UpperLeg, { 0, 0, -1.0f }, { 6.49f, 6.49f, 45.0f }, { 0, 0, -1 }, L"Lower Leg");
	Bone* Foot = GenerateBone(LowerLeg, { 15.5f / 22.0f, 0, 0 }, { 22.0f, 8.0f, 3.0f }, { 0, 0, -1.175f }, L"Foot");

	GenerateRightSide(UpperLeg, UpperLeg->Parent, { 0, 1, 0 });

	Bone* UpperArm = GenerateBone(Spine, { 0, 1, 0, }, { 4.5f, 32.0f, 4.5f }, { 0, 0.5f, 1.0f }, L"Upper Arm");
	Bone* LowerArm = GenerateBone(UpperArm, { 0, 1, 0, }, { 4.49f, 28.0f, 4.49f }, { 0, 1.0f, 0.0f }, L"Lower Arm");
	Bone* Hand = GenerateBone(LowerArm, { 0, 1, 0, }, { 3.5f, 15.0f, 1.5f }, { 0, 1.0f, 0.0f }, L"Hand");

	GenerateRightSide(UpperArm, UpperArm->Parent, { 0, 1, 0 });
}

void Character::UpdateWorldTranforms(void)
{
	mat4 ParentModel = translate(mat4(1.0f), this->Position);

	Spine->UpdateWorldTransform(ParentModel, {});
}

void Character::UpdateFloorZ(void)
{
	FloorZ = 0;

	for (Bone* Bone : this->Bones) {

		mat4 MiddleTransform = Bone->WorldTransform * Bone->MiddleTranslation;

		float Z = MiddleTransform[3].z - Bone->Size.z * 0.5f;

		FloorZ = min(FloorZ, Z);
	}
}

// Bone

Bone::Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, Bone* Parent)
{
	this->ID = ID;
	this->Name = Name;

	this->Offset = Offset;
	this->Tail = Tail;
	this->Size = Size;

	this->Rotation = mat4(1.0f);
	this->MiddleTranslation = translate(mat4(1.0f), this->Tail * this->Size * 0.5f);

	this->Parent = Parent;
	if (Parent != nullptr) {
		Parent->Childs.push_back(this);

		this->Depth = Parent->Depth + 1;
	}
	else
		this->Depth = 0;
}

void Bone::UpdateWorldTransform(mat4 ParentModel, vec3 ParentSize)
{
	mat4 OffsetTranslation = translate(mat4(1.0f), this->Offset * ParentSize);
	this->WorldTransform = ParentModel * OffsetTranslation * this->Rotation;

	for (Bone* Child : this->Childs)
		Child->UpdateWorldTransform(this->WorldTransform, this->Size);
}