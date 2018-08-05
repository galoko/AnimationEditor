#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <btBulletDynamicsCommon.h>

using namespace std;
using namespace glm;

struct AnimationContext;

typedef class Bone {
private:
	wstring Name;
public:
	uint32 ID;

	vec3 Offset, Tail, Size, LowLimit, HighLimit;

	Bone* Parent;
	vector<Bone*> Childs;

	mat4 Rotation, WorldTransform, MiddleTranslation;

	vec3 LogicalDirection;

	uint32 Depth;

	typedef enum BoneSide {
		Center, 
		Left,
		Right
	} BoneSide;

	BoneSide Side;

	float Mass;

	vec3 JointLocalPoint, ParentJointLocalPoint;
	bool XAxisBlocked, YAxisBlocked, ZAxisBlocked;

	btRigidBody* PhysicBody;
	btTypedConstraint* PhysicConstraint;

	AnimationContext* AnimCtx;

	bool IsFixed(void);
	bool IsOnlyXRotation(void);
	bool IsOnlyYRotation(void);
	bool IsOnlyZRotation(void);

	Bone(uint32 ID, wstring Name, vec3 Offset, vec3 Tail, vec3 Size, vec3 LowLimit, vec3 HighLimit, vec3 LogicalDirection, Bone* Parent);

	void UpdateWorldTransform(mat4 ParentModel, vec3 ParentSize);
	vec3 UpdateRotationFromWorldTransform(mat4 ParentWorldRotation);

	wstring GetName(void);
	wstring GetOriginalName(void);
} Bone;

typedef class Character {
private:
	uint32 NextBoneID;

	Bone* GenerateBone(Bone* Parent, vec3 Tail, vec3 Size, vec3 Offset, vec3 LowLimit, vec3 HighLimit, vec3 LogicalDirection, wstring Name);
	void GenerateRightSide(Bone* LeftBone, Bone* RightParent, vec3 MirrorDirection);
	void GenerateBones(void);
	void CalculateJointLocations(void);
public:
	vec3 Position;

	Bone* Pelvis; // also known as Root

	vector<Bone*> Bones; // list for easy iterating

	float FloorZ;

	Character(void);

	void UpdateWorldTranforms(void);
	void UpdateRotationsFromWorldTransforms(void);

	void UpdateFloorZ(void);

	void Reset(void);

	Bone* FindBone(const wstring Name);
	Bone* FindOtherBone(Bone* CurrentBone);
} Character;