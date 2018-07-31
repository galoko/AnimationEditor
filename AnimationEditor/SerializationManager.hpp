#pragma once

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <tinyxml2.h>

#include "ExternalGUI.hpp"

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

typedef struct SingleSerializedState {
	SerializationPendingID PendingID;

	CharacterSerializedState CharState;
	InputSerializedState InputState;
	AnimationSerializedState AnimationState;
	RenderSerializedState RenderState;

	bool HaveCharState, HaveInputState, HaveAnimationState, HaveRenderState;
} SingleSerializedState;

typedef struct SerializedStateHistory {
	int32 ID;
	vector<SingleSerializedState> PreviousStates, FutureStates;
	SingleSerializedState CurrentState;
} SerializedStateHistory;

typedef class SerializationManager {
private:

	ULONGLONG PendingLastTime;

	vector<SerializedStateHistory> Histories;

	int32 NextStateHistoryID;

	wstring LastFileName;
	ULONGLONG LastAutosaveTime;

	const wstring SettingsFileName = L"Settings.xml";

	SerializationManager(void) { };

	void LoadSettings(void);
	void SaveSettings(void);

	void Serialize(void);
	void Deserialize(void);

	void LoadState(SingleSerializedState& State, XMLDocument& Document, XMLNode* Root);
	void SaveState(SingleSerializedState& State, XMLDocument& Document, XMLNode* Root);

	void LoadStates(SerializedStateHistory& States, XMLDocument& Document, XMLNode* Root);
	void SaveStates(SerializedStateHistory& States, XMLDocument& Document, XMLNode* Root);

	void InternalPushStateFrame(bool Forward);
	void LimitFrames(vector<SingleSerializedState>& Frames, int Limit);

	const int AutosaveInterval = 30 * 1000;

	const int MaxBackupCount = 1000;

	const int MaxFrames = 100;

	bool IsFileOpen(void);

	vector<SerializedStateHistory>::iterator GetCurrentHistory(void);
	vector<SerializedStateHistory>::iterator GetHistoryByID(int32 ID);

	void SafeSaveDocumentToFile(XMLDocument& Document, const wstring FileName, int BackupCount);
public:
	static SerializationManager& GetInstance(void) {
		static SerializationManager Instance;

		return Instance;
	}

	SerializationManager(SerializationManager const&) = delete;
	void operator=(SerializationManager const&) = delete;

	void Initialize(void);

	void CreateNewHistories(void);

	void LoadFromFile(const wstring FileName);
	void SaveToFile(const wstring FileName);

	void Autosave(void);

	void Tick(double dt);

	bool LoadHistoryByID(int32 ID);
	int32 GetCurrentHistoryID(void);

	const int PendingTimeout = 250;

	void PushStateFrame(const wstring Sender);
	void PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender);
	void PopAndDeserializeStateFrame(bool Forward);

	vector<TimelineItem> GetTimelineItems(void);
	void SetTimelineItems(vector<TimelineItem> Items);
} SerializationManager;