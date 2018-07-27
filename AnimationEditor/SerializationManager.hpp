#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <tinyxml2.h>

using namespace std;
using namespace glm;
using namespace tinyxml2;

typedef struct SerializedBone {
	wstring Name;
	quat Rotation;
} SerializedBone;

typedef struct CharacterSerializedState {
	vec3 Position;
	vector<SerializedBone> Bones;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} CharacterSerializedState;

typedef struct InputSerializedState {

	uint32 State, PlaneMode;

	wstring BoneName;
	vec3 LocalPoint, WorldPoint;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} InputSerializedState;

typedef struct SerializedBlockingInfo {
	bool XAxis, YAxis, ZAxis, XPos, YPos, ZPos;
} SerializedBlockingInfo;

typedef struct SerializedAnimationContext {
	wstring BoneName;
	SerializedBlockingInfo Blocking;
	// pin point
	bool IsActive;
	vec3 SrcLocalPoint, DestWorldPoint;
} SerializedAnimationContext;

typedef struct AnimationSerializedState {
	vector<SerializedAnimationContext> Contexts;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} AnimationSerializedState;

typedef struct RenderSerializedState {
	vec3 CameraPosition;
	float CameraAngleX, CameraAngleZ;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} RenderSerializedState;

typedef struct CompleteSerializedState {

	CharacterSerializedState CharState;
	InputSerializedState InputState;
	AnimationSerializedState AnimationState;
	RenderSerializedState RenderState;

	bool HaveCharState, HaveInputState, HaveAnimationState, HaveRenderState;
} CompleteSerializedState;

typedef class SerializationManager {
private:
	SerializationManager(void) { };
public:
	static SerializationManager& GetInstance(void) {
		static SerializationManager Instance;

		return Instance;
	}

	SerializationManager(SerializationManager const&) = delete;
	void operator=(SerializationManager const&) = delete;

	void Serialize(CompleteSerializedState& State);
	void Deserialize(CompleteSerializedState& State);

	void LoadFromFile(CompleteSerializedState& State, const wstring FileName);
	void SaveToFile(CompleteSerializedState& State, const wstring FileName);
} SerializationManager;