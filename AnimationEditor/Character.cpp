#include "Character.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

Character::Character(void)
{
	GenerateBones();
	UpdateWorldTranforms();
	UpdateFloorZ();
	CalculateJointLocations();
}

Bone* Character::GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, vec3 LowLimit, vec3 HighLimit, vec3 LogicalDirection, wstring Name)
{
	float CmToMeters = 0.01f;

	Bone* Result = new Bone(NextBoneID++, Name, Offset, Tail, Size * CmToMeters, 
		vec3(radians(LowLimit.x), radians(LowLimit.y), radians(LowLimit.z)),
		vec3(radians(HighLimit.x), radians(HighLimit.y), radians(HighLimit.z)),
		LogicalDirection,
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

vec3 GetMirrorVector(vec3 MirrorDirection) {
	return cross(MirrorDirection, cross(MirrorDirection, vec3(-1.0f))) - MirrorDirection;
}

void Character::GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection)
{
	vec3 MirrorVector = GetMirrorVector(MirrorDirection);

	float MetersToCm = 100.0f;

	vec3 LowLimit = LeftBone->LowLimit * -MirrorVector;
	vec3 HighLimit = LeftBone->HighLimit * -MirrorVector;

	NormalizeLimits(&LowLimit, &HighLimit);

	Bone* RightBone = new Bone(NextBoneID++, LeftBone->GetOriginalName(),
		LeftBone->Offset * MirrorVector, LeftBone->Tail * MirrorVector, LeftBone->Size,
		LowLimit, HighLimit, LeftBone->LogicalDirection * MirrorVector, RightParent);

	Bones.push_back(RightBone);

	RightBone->Side = Bone::Right;
	LeftBone->Side = Bone::Left;

	for (Bone* LeftChild : LeftBone->Childs)
		GenerateRightSide(LeftChild, RightBone, MirrorDirection);
}

void Character::GenerateBones(void)
{
	Pelvis = GenerateBone(nullptr, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 0 },  
		{ -180, -89, -180 },
		{  180,  89,  180 }, 
		{ 1, 0, 0 }, L"Pelvis");

	Bone* Stomach = GenerateBone(Pelvis, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{  -10,  -70,  -10 }, 
		{   10,    5,   10 },
		{ 1, 0, 0 }, L"Stomach");
	Bone* Chest = GenerateBone(Stomach, { 0, 0, 1.0f }, { 6.5f, 13.0f, 17.6f }, { 0, 0, 1 }, 
		{  -10,  -70,  -10 },
		{   10,   10,   10 },
		{ 1, 0, 0 }, L"Chest");

	Bone* Neck = GenerateBone(Chest, { 0, 0, 1, }, { 3.0f, 3.0f, 15.0f }, { 0, 0, 1 }, 
		{    0,  -70,    0 }, 
		{    0,   35,    0 }, 
		{ 0, 0, 0 }, L"Neck");
	Bone* Head = GenerateBone(Neck, { 0, 0, 0, }, { 15.0f, 15.0f, 20.0f }, { 0, 0, 1 }, 
		{  -30,  -30,  -80 }, 
		{   30,   10,   80 }, 
		{ 1, 0, 0 }, L"Head");

	Bone* UpperLeg = GenerateBone(Pelvis, { 0, 0, -1.0f }, { 6.5f, 6.5f, 46.0f }, { 0, 0.5f, 0 }, 
		{  -70,  -20,  -90 }, 
		{   30,  130,   90 }, 
		{ 1, 0, 0 }, L"Upper Leg");
	Bone* LowerLeg = GenerateBone(UpperLeg, { 0, 0, -1.0f }, { 6.49f, 6.49f, 45.0f }, { 0, 0, -1 }, 
		{    0, -165,    0 }, 
		{    0,    0,    0 }, 
		{ 1, 0, 0 }, L"Lower Leg");
	Bone* Foot = GenerateBone(LowerLeg, { 15.5f / 22.0f, 0, 0 }, { 22.0f, 8.0f, 3.0f }, { 0, 0, -1.175f }, 
		{  -25,  -70,   -5 }, 
		{   25,   45,    5 }, 
		{ 0, 0, 1 }, L"Foot");

	GenerateRightSide(UpperLeg, UpperLeg->Parent, { 0, 1, 0 });

	Bone* UpperArm = GenerateBone(Chest, { 0, 1, 0, }, { 4.5f, 32.0f, 4.5f }, { 0, 0.85f, 1 },
		{  -65,  -80,  -45 }, 
		{  110,   80,  110 }, 
		{ 1, 0, 0 }, L"Upper Arm");
	Bone* LowerArm = GenerateBone(UpperArm, { 0, 1, 0, }, { 4.49f, 28.0f, 4.49f }, { 0, 1, 0 }, 
		{    0,    0,    0 }, 
		{    0,    0,  165 }, 
		{ 0, 0, 1 }, L"Lower Arm");
	Bone* Hand = GenerateBone(LowerArm, { 0, 1, 0, }, { 3.5f, 15.0f, 1.5f }, { 0, 1, 0 }, 
		{  -70,  -80,  -35 }, 
		{   90,   90,   35 }, 
		{ 0, 0, -1 }, L"Hand");

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

void Character::Reset(void)
{
	Position = vec3(0.0f);

	for (Bone* Bone : Bones)
		Bone->Rotation = mat4(1.0f);

	UpdateWorldTranforms();
}

void Character::CalculateJointLocations(void)
{
	for (Bone* Child : Bones) {

		Bone* Parent = Child->Parent;
		if (Parent == nullptr)
			continue;

		vec4 Zero = vec4(0, 0, 0, 1);

		vec3 ChildHead = Child->WorldTransform * Zero;
		vec3 ChildPosition = Child->WorldTransform * Child->MiddleTranslation * Zero;
		vec3 ParentPosition = Parent->WorldTransform * Parent->MiddleTranslation * Zero;

		Child->JointLocalPoint = ChildHead - ChildPosition;
		Child->ParentJointLocalPoint = ChildHead - ParentPosition;
	}
}

Bone* Character::FindBone(const wstring Name)
{
	for (Bone* Bone : Bones)
		if (Bone->GetName().find(Name) != -1)
			return Bone;
	return nullptr;
}

Bone* Character::FindOtherBone(Bone* CurrentBone)
{
	for (Bone* Bone : Bones) 
		if (Bone->GetOriginalName() == CurrentBone->GetOriginalName() && Bone->Side != CurrentBone->Side)
			return Bone;	
	return nullptr;
}

// Bone

Bone::Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, vec3 LowLimit, vec3 HighLimit, vec3 LogicalDirection, Bone* Parent)
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

	this->LogicalDirection = LogicalDirection;

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

wstring Bone::GetName(void)
{
	wstring Prefix;

	switch (Side) {
	case Left:
		Prefix = L"Left ";
		break;
	case Right:
		Prefix = L"Right ";
		break;
	default:
		Prefix = L"";
		break;
	}

	return Prefix + GetOriginalName();
}

wstring Bone::GetOriginalName(void)
{
	return Name;
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