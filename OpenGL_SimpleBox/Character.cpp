#include "Character.hpp"

Character::Character(void)
{
	GenerateBones();
	UpdateWorldTranforms();
	UpdateFloorZ();
	SaveInitialPositions();
}

Bone* Character::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, vec3 LowLimit, vec3 HighLimit, wstring Name)
{
	float CmToMeters = 0.01f;

	Bone* Result = new Bone(NextBoneID++, Name, Offset, Tail, Size * CmToMeters, LowLimit, HighLimit, Parent);

	Bones.push_back(Result);

	return Result;
}

void NormalizeLimit(float* LowLimit, float* HighLimit) {

	if (*LowLimit > *HighLimit) {
		float Temp = *LowLimit;
		*LowLimit = *HighLimit;
		*HighLimit = Temp;
	}
}

void NormalizeLimits(vec3* LowLimit, vec3* HighLimit) {
	NormalizeLimit(&LowLimit->x, &HighLimit->x);
	NormalizeLimit(&LowLimit->y, &HighLimit->y);
	NormalizeLimit(&LowLimit->z, &HighLimit->z);
}

void Character::GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection)
{
	vec3 MirrorVector = cross(MirrorDirection, cross(MirrorDirection, vec3(-1.0f))) - MirrorDirection;

	float MetersToCm = 100.0f;

	vec3 LowLimit = LeftBone->LowLimit * -MirrorVector;
	vec3 HighLimit = LeftBone->HighLimit * -MirrorVector;

	NormalizeLimits(&LowLimit, &HighLimit);

	Bone* RightBone = GenerateBone(RightParent,
		LeftBone->Tail * MirrorVector,
		LeftBone->Size * MetersToCm,
		LeftBone->Offset * MirrorVector, 
		LowLimit, HighLimit, 
		LeftBone->Name + L" Right");

	LeftBone->Name = LeftBone->Name + L" Left";

	for (Bone* LeftChild : LeftBone->Childs)
		GenerateRightSide(LeftChild, RightBone, MirrorDirection);
}

void Character::GenerateBones(void)
{
	Pelvis = GenerateBone(nullptr, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, {}, {}, {}, L"Pelvis");
	Bone* Stomach = GenerateBone(Pelvis, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{ 0, radians(-70.0f), radians(-10.0f) }, { 0, radians(5.0f), radians(10.0f) }, L"Stomach");
	Bone* Chest = GenerateBone(Stomach, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{ 0, radians(-70.0f), radians(-10.0f) }, { 0, radians(10.0f), radians(10.0f) }, L"Chest");

	Bone* Neck = GenerateBone(Chest, { 0, 0, 1, }, { 3.0f, 3.0f, 15.0f }, { 0, 0, 1 }, 
		{ 0, radians(-70.0f), 0 }, { 0, radians(35.0f), 0 }, L"Neck");
	Bone* Head = GenerateBone(Neck, { 0, 0, 0, }, { 15.0f, 15.0f, 20.0f }, { 0, 0, 1 }, 
		// { radians(-45.0f), radians(-70.0f), radians(-90.0f) }, { radians(45.0f), radians(10.0f), radians(90.0f) }, L"Head");
		// { 0, radians(-70.0f), radians(-90.0f) }, { 0, radians(10.0f), radians(90.0f) }, L"Head");
		// { 0, 0, radians(-80.0f) }, { 0, 0, radians(80.0f) }, L"Head");
		// { 0, radians(-30.0f), 0 }, { 0, radians(10.0f), 0 }, L"Head");
		// { radians(-30.0f), 0, 0 }, { radians(30.0f), 0, 0 }, L"Head");
		{ radians(-30.0f), radians(-30.0f), radians(-80.0f) }, { radians(30.0f), radians(10.0f), radians(80.0f) }, L"Head");

	Bone* UpperLeg = GenerateBone(Pelvis, { 0, 0, -1.0f }, { 6.5f, 6.5f, 46.0f }, { 0, 0.5f, 0 }, 
		{ radians(-70.0f), radians(-20.0f), radians(-90.0f) }, { radians(30.0f), radians(110.0f), radians(90.0f) }, L"Upper Leg");
		// { 0, radians(-20.0f), radians(-90.0f) }, { 0, radians(110.0f), radians(90.0f) }, L"Upper Leg");
	Bone* LowerLeg = GenerateBone(UpperLeg, { 0, 0, -1.0f }, { 6.49f, 6.49f, 45.0f }, { 0, 0, -1 }, 
		{ 0, radians(-165.0f), 0 }, { 0, 0, 0 }, L"Lower Leg");
	Bone* Foot = GenerateBone(LowerLeg, { 15.5f / 22.0f, 0, 0 }, { 22.0f, 8.0f, 3.0f }, { 0, 0, -1.175f }, 
		{ radians(-25.0f), radians(-70.0f), radians(-5.0f) }, { radians(25.0f), radians(30.0f), radians(5.0f) }, L"Foot");

	GenerateRightSide(UpperLeg, UpperLeg->Parent, { 0, 1, 0 });

	Bone* UpperArm = GenerateBone(Chest, { 0, 1, 0, }, { 4.5f, 32.0f, 4.5f }, { 0, 0.5f, 1.0f }, 
		{ radians(-70.0f), radians(-80.0f), radians(-110.0f) }, { radians(100.0f), radians(80.0f), radians(45.0f) }, L"Upper Arm");
	Bone* LowerArm = GenerateBone(UpperArm, { 0, 1, 0, }, { 4.49f, 28.0f, 4.49f }, { 0, 1.0f, 0.0f }, { 0, 0, 0 }, { 0, 0, radians(165.0f) }, L"Lower Arm");
	Bone* Hand = GenerateBone(LowerArm, { 0, 1, 0, }, { 3.5f, 15.0f, 1.5f }, { 0, 1.0f, 0.0f }, 
		{ radians(-70.0f), radians(-90.0f), radians(-35.0f) }, { radians(80.0f), radians(45.0f), radians(35.0f) }, L"Hand");

	GenerateRightSide(UpperArm, UpperArm->Parent, { 0, 1, 0 });
}

void Character::UpdateWorldTranforms(void)
{
	mat4 ParentModel = translate(mat4(1.0f), this->Position);

	Pelvis->UpdateWorldTransform(ParentModel, {});
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

Bone* Character::FindBone(const wstring Name)
{
	for (Bone* Bone : this->Bones)
		if (Bone->Name.find(Name) != -1)
			return Bone;
	return nullptr;
}

void Character::LockEverythingBut(vector<wstring> BoneNames)
{
	for (Bone* Bone : Bones) {
		
		bool IsInUnlockList = false;
		for (const wstring Name : BoneNames) 
			if (Bone->Name.find(Name) != -1) {
				IsInUnlockList = true;
				break;
			}

		Bone->IsLocked = !IsInUnlockList;
	}
}

void Character::SaveInitialPositions(void)
{
	for (Bone* Bone : Bones)
		Bone->SaveInitialPosition();
}

// Bone

Bone::Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, vec3 LowLimit, vec3 HighLimit, Bone* Parent)
{
	this->ID = ID;
	this->Name = Name;

	this->Offset = Offset;
	this->Tail = Tail;
	this->Size = Size;

	this->LowLimit = LowLimit;
	this->HighLimit = HighLimit;

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

void Bone::SaveInitialPosition(void)
{
	InitialPosition = WorldTransform * MiddleTranslation * vec4(0, 0, 0, 1);
}

bool Bone::IsFixed(void)
{
	return (LowLimit.x == 0 && LowLimit.y == 0 && LowLimit.z == 0 && HighLimit.x == 0 && HighLimit.y == 0 && HighLimit.z == 0);
}

bool Bone::IsOnlyXRotation(void)
{
	return ((LowLimit.x != 0 || HighLimit.x != 0) && LowLimit.y == 0 && LowLimit.z == 0 && HighLimit.y == 0 && HighLimit.z == 0);
}

bool Bone::IsOnlyYRotation(void)
{
	return ((LowLimit.y != 0 || HighLimit.y != 0) && LowLimit.x == 0 && LowLimit.z == 0 && HighLimit.x == 0 && HighLimit.z == 0);
}

bool Bone::IsOnlyZRotation(void)
{
	return ((LowLimit.z != 0 || HighLimit.z != 0) && LowLimit.y == 0 && LowLimit.x == 0 && HighLimit.y == 0 && HighLimit.x == 0);
}
