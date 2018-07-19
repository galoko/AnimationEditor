#include "Character.hpp"

Character::Character(void)
{
	GenerateBones();
	UpdateWorldTranforms();
	UpdateFloorZ();
	SaveInitialPositions();
	UpdateRotationsFromWorldTransforms();
}

Bone* Character::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, vec3 LowLimit, vec3 HighLimit, wstring Name)
{
	float CmToMeters = 0.01f;

	Bone* Result = new Bone(NextBoneID++, Name, Offset, Tail, Size * CmToMeters, 
		vec3(radians(LowLimit.x), radians(LowLimit.y), radians(LowLimit.z)),
		vec3(radians(HighLimit.x), radians(HighLimit.y), radians(HighLimit.z)),
		Parent);

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

	Bone* RightBone = new Bone(NextBoneID++, LeftBone->Name + L" Right", 
		LeftBone->Offset * MirrorVector, LeftBone->Tail * MirrorVector, LeftBone->Size,
		LowLimit, HighLimit, RightParent);

	Bones.push_back(RightBone);

	LeftBone->Name = LeftBone->Name + L" Left";

	for (Bone* LeftChild : LeftBone->Childs)
		GenerateRightSide(LeftChild, RightBone, MirrorDirection);
}

void Character::GenerateBones(void)
{
	Pelvis = GenerateBone(nullptr, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, {}, {}, {}, L"Pelvis");
	Bone* Stomach = GenerateBone(Pelvis, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{    0,  -70,  -10 }, 
		{    0,    5,   10 }, L"Stomach");
	Bone* Chest = GenerateBone(Stomach, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{    0,  -70,  -10 }, 
		{    0,   10,   10 }, L"Chest");

	Bone* Neck = GenerateBone(Chest, { 0, 0, 1, }, { 3.0f, 3.0f, 15.0f }, { 0, 0, 1 }, 
		{    0,  -70,    0 }, 
		{    0,   35,    0 }, L"Neck");
	Bone* Head = GenerateBone(Neck, { 0, 0, 0, }, { 15.0f, 15.0f, 20.0f }, { 0, 0, 1 }, 
		{  -30,  -30,  -80 }, 
		{   30,   10,   80 }, L"Head");

	Bone* UpperLeg = GenerateBone(Pelvis, { 0, 0, -1.0f }, { 6.5f, 6.5f, 46.0f }, { 0, 0.5f, 0 }, 
		{  -70,  -20,  -90 }, 
		{   30,  130,   90 }, L"Upper Leg");
	Bone* LowerLeg = GenerateBone(UpperLeg, { 0, 0, -1.0f }, { 6.49f, 6.49f, 45.0f }, { 0, 0, -1 }, 
		{    0, -165,    0 }, 
		{    0,    0,    0 }, L"Lower Leg");
	Bone* Foot = GenerateBone(LowerLeg, { 15.5f / 22.0f, 0, 0 }, { 22.0f, 8.0f, 3.0f }, { 0, 0, -1.175f }, 
		{  -25,  -70,   -5 }, 
		{   25,   30,    5 }, L"Foot");

	GenerateRightSide(UpperLeg, UpperLeg->Parent, { 0, 1, 0 });

	Bone* UpperArm = GenerateBone(Chest, { 0, 1, 0, }, { 4.5f, 32.0f, 4.5f }, { 0, 0.5f, 1 },
		{  -65,  -80,  -45 }, 
		{  110,   80,  110 }, L"Upper Arm");
	Bone* LowerArm = GenerateBone(UpperArm, { 0, 1, 0, }, { 4.49f, 28.0f, 4.49f }, { 0, 1, 0 }, 
		{    0,    0,    0 }, 
		{    0,    0,  165 }, L"Lower Arm");
	Bone* Hand = GenerateBone(LowerArm, { 0, 1, 0, }, { 3.5f, 15.0f, 1.5f }, { 0, 1, 0 }, 
		{  -70,  -90,  -35 }, 
		{   80,   45,   35 }, L"Hand");

	GenerateRightSide(UpperArm, UpperArm->Parent, { 0, 1, 0 });
}

void Character::UpdateWorldTranforms(void)
{
	mat4 ParentModel = translate(mat4(1.0f), this->Position);

	Pelvis->UpdateWorldTransform(ParentModel, {});
}

void Character::UpdateRotationsFromWorldTransforms(void)
{
	this->Position = Pelvis->UpdateRotationFromWorldTransform(mat4(1.0f));
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

vec3 Bone::UpdateRotationFromWorldTransform(mat4 ParentWorldRotation)
{
	vec3 Scale, Translation, Skew;
	quat Orientation;
	vec4 Perspective;

	decompose(WorldTransform, Scale, Orientation, Translation, Skew, Perspective);

	mat4 Rotation = mat4_cast(Orientation);

	this->Rotation = inverse(ParentWorldRotation) * Rotation;

	for (Bone* Child : this->Childs)
		Child->UpdateRotationFromWorldTransform(Rotation);

	return Translation;
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
