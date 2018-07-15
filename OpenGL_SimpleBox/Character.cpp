#include "Character.hpp"

Character::Character(void)
{
	GenerateBones();
}

Bone* Character::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name)
{
	float CmToMeters = 0.01f;

	Bone* Result = new Bone(Parent, Tail, Size * CmToMeters, Offset, Name, NextBoneID++);

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

float Character::GetLowestZResursive(Bone* Current, float CurrentZ, float ParentHeight)
{
	float HeadZ = CurrentZ + Current->Offset.z * ParentHeight;
	float MiddleZ = HeadZ + Current->Tail.z * Current->Size.z * 0.5f;
	float TailZ = MiddleZ - Current->Size.z * 0.5f;

	CurrentZ = min(CurrentZ, HeadZ);
	CurrentZ = min(CurrentZ, MiddleZ);
	CurrentZ = min(CurrentZ, TailZ);

	for (Bone* Child : Current->Childs)
		CurrentZ = min(CurrentZ, GetLowestZResursive(Child, HeadZ, Current->Size.z));

	return CurrentZ;
}

float Character::GetFloorZ(void)
{
	return GetLowestZResursive(this->Spine, 0.0f, 0.0f);
}

void Character::UpdateWorldTranform(void)
{

}

// Bone

Bone::Bone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name, uint32 ID)
{
	this->Parent = Parent;
	if (Parent != nullptr)
		Parent->Childs.push_back(this);

	this->Name = Name;
	this->ID = ID;
	this->Tail = Tail;
	this->Offset = Offset;
	this->Size = Size;
	this->Rotation = mat4(1.0f);
}