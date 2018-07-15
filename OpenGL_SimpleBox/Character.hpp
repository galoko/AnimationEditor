#pragma once

#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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

	mat4 Rotation, WorldTransform, MiddleTranslation;

	uint32 Depth;

	btRigidBody* PhysicBody;

	Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, Bone* Parent);

	void UpdateWorldTransform(mat4 ParentModel, vec3 ParentSize);
} Bone;

typedef class Character {
private:
	uint32 NextBoneID;

	Bone* GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, wstring Name);
	void GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection);
	void GenerateBones(void);
public:
	vec3 Position;

	Bone* Spine; // also known as Root

	vector<Bone*> Bones; // list for easy iterating

	float FloorZ;

	Character(void);

	void UpdateWorldTranforms(void);

	void UpdateFloorZ(void);
} Character;