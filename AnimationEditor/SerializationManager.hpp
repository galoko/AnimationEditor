#pragma once

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

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

	uint32 AnimationTimestamp;

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

typedef enum SerializationPendingID {
	PendingNone,
	PendingAnglesTrackBar,
	PendingInverseKinematic
} SerializationPendingID;

typedef struct CompleteSerializedState {
	SerializationPendingID PendingID;

	CharacterSerializedState CharState;
	InputSerializedState InputState;
	AnimationSerializedState AnimationState;
	RenderSerializedState RenderState;

	bool HaveCharState, HaveInputState, HaveAnimationState, HaveRenderState;
} CompleteSerializedState;

typedef struct CompleteSerializedStates {
	vector<CompleteSerializedState> PreviousStates, FutureStates;
	CompleteSerializedState CurrentState;
} CompleteSerializedStates;

typedef class SerializationManager {
private:

	SerializationPendingID CurrentPendingID;
	ULONGLONG PendingLastTime;

	vector<CompleteSerializedState> StateFrames, ForwardStateFrames;

	vector<CompleteSerializedStates> OtherStates;

	SerializationManager(void) { };

	void Serialize(CompleteSerializedState& State);
	void Deserialize(CompleteSerializedState& State);

	void LoadState(CompleteSerializedState& State, XMLDocument& Document, XMLNode* Root);
	void SaveState(CompleteSerializedState& State, XMLDocument& Document, XMLNode* Root);

	void LoadStates(CompleteSerializedStates& States, XMLDocument& Document, XMLNode* Root);
	void SaveStates(CompleteSerializedStates& States, XMLDocument& Document, XMLNode* Root);

	void InternalPushStateFrame(bool Forward);
public:
	static SerializationManager& GetInstance(void) {
		static SerializationManager Instance;

		return Instance;
	}

	SerializationManager(SerializationManager const&) = delete;
	void operator=(SerializationManager const&) = delete;

	void LoadFromFile(const wstring FileName);
	void SaveToFile(const wstring FileName);

	void Tick(double dt);

	const int PendingTimeout = 250;

	void PushStateFrame(const wstring Sender);
	void PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender);
	void PopAndDeserializeStateFrame(bool Forward);

} SerializationManager;