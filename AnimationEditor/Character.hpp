#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <btBulletDynamicsCommon.h>

using namespace std;
using namespace glm;

typedef class Bone {
public:
	uint32 ID;
	wstring Name;

	vec3 Offset, Tail, Size, LowLimit, HighLimit;

	Bone* Parent;
	vector<Bone*> Childs;

	mat4 Rotation, WorldTransform, MiddleTranslation;

	uint32 Depth;

	float Mass;

	btRigidBody* PhysicBody;

	bool IsLocked;

	vec3 InitialPosition;

	bool IsFixed(void);
	bool IsOnlyXRotation(void);
	bool IsOnlyYRotation(void);
	bool IsOnlyZRotation(void);

	Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, vec3 LowLimit, vec3 HighLimit, Bone* Parent);

	void UpdateWorldTransform(mat4 ParentModel, vec3 ParentSize);
	vec3 UpdateRotationFromWorldTransform(mat4 ParentWorldRotation);

	void SaveInitialPosition(void);
} Bone;

typedef class Character {
private:
	uint32 NextBoneID;

	Bone* GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, vec3 LowLimit, vec3 HighLimit, wstring Name);
	void GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection);
	void GenerateBones(void);

	void SaveInitialPositions(void);
public:
	vec3 Position;

	Bone* Pelvis; // also known as Root

	vector<Bone*> Bones; // list for easy iterating

	float FloorZ;

	Character(void);

	void UpdateWorldTranforms(void);
	void UpdateRotationsFromWorldTransforms(void);

	void UpdateFloorZ(void);

	Bone* FindBone(const wstring Name);

	void LockEverythingBut(vector<wstring> BoneNames);
} Character;