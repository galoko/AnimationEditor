#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <btBulletDynamicsCommon.h>

using namespace std;
using namespace glm;

typedef class Bone {

public:
	uint32 ID;
	wstring Name;

	vec3 Offset, Tail, Size;

	Bone* Parent;
	vector<Bone*> Childs;

	mat4 Rotation, WorldTransform;

	btRigidBody* PhysicBody;

	Bone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name, uint32 ID);
} Bone;

typedef class Character {
private:
	uint32 NextBoneID;

	float GetLowestZResursive(Bone* Bone, float CurrentZ, float ParentHeight);

	Bone* GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name);
	void GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection);
	void GenerateBones(void);
public:
	vec3 Position;

	Bone* Spine; // also known as Root

	vector<Bone*> Bones; // list for easy iterating

	Character(void);

	float GetFloorZ(void);

	void UpdateWorldTranform(void);
} Character;