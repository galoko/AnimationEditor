#pragma once

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <tinyxml2.h>

#include "blockingconcurrentqueue.h"

#include "ExternalGUI.hpp"

using namespace std;
using namespace glm;
using namespace tinyxml2;
using namespace moodycamel;

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

typedef struct SerializedPoseContext {
	wstring BoneName;
	SerializedBlockingInfo Blocking;
	// pin point
	bool IsActive;
	vec3 SrcLocalPoint, DestWorldPoint;
} SerializedPoseContext;

typedef struct PoseSerializedState {
	vector<SerializedPoseContext> Contexts;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} PoseSerializedState;

typedef struct RenderSerializedState {
	vec3 CameraPosition;
	float CameraAngleX, CameraAngleZ;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} RenderSerializedState;

typedef struct SerializeSerializedState {
	float AnimationPosition, AnimationLength, PlaySpeed;
	bool KinematicModeFlag, PlayAnimaionFlag, LoopAnimationFlag;

	void SaveToXML(XMLDocument& Document, XMLNode *Root);
	bool LoadFromXML(XMLDocument& Document, XMLNode *Root);
} SerializeSerializedState;

typedef enum SerializationPendingID {
	PendingNone,
	PendingAnglesTrackBar,
	PendingInverseKinematic,
	PendingAnimationState
} SerializationPendingID;

typedef struct SingleSerializedState {
	SerializationPendingID PendingID;

	CharacterSerializedState CharState;
	InputSerializedState InputState;
	PoseSerializedState PoseState;
	RenderSerializedState RenderState;

	bool HaveCharState, HaveInputState, HavePoseState, HaveRenderState;
} SingleSerializedState;

typedef struct SerializedStateHistory {
	int32 ID;
	bool IsDeleted;

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

	wstring SettingsFileName;

	float AnimationPosition, AnimationLength, PlaySpeed;
	bool KinematicModeFlag, PlayAnimaionFlag, LoopAnimationFlag;

	typedef struct FileSaveRequest {
		vector<SerializedStateHistory> Histories;
		SerializeSerializedState State;

		wstring FileName;
	} FileSaveRequest;

	BlockingConcurrentQueue<FileSaveRequest*> DelayedFileSaveRequests;

	SerializationManager(void) { };

	void LoadSettings(void);
	void SaveSettings(void);

	void Serialize(void);
	void Deserialize(void);

	void Serialize(SerializeSerializedState& State);
	void Deserialize(SerializeSerializedState& State);

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

	vector<SerializedStateHistory>::iterator GetLatestHistory(void);
	vector<SerializedStateHistory>::iterator GetCurrentHistory(void);
	vector<SerializedStateHistory>::iterator GetHistoryByID(int32 ID);

	void GetStatesAndTFromPosition(float Position, float Length, CharacterSerializedState*& PrevState, CharacterSerializedState*& NextState, float& t);
	void ProcessAnimaiton(void);

	void SafeSaveDocumentToFile(XMLDocument& Document, const wstring FileName, int BackupCount);

	void StartBackgroundThread(void);
	static DWORD WINAPI BackgroundStaticThreadProc(LPVOID lpThreadParameter);
	void BackgroundThreadProc(void);
	void ExecuteFileSaveRequest(FileSaveRequest& Request);
public:
	static SerializationManager& GetInstance(void) {
		static SerializationManager Instance;

		return Instance;
	}

	SerializationManager(SerializationManager const&) = delete;
	void operator=(SerializationManager const&) = delete;

	void Initialize(const wstring WorkingDirectory);

	void LoadFromFile(wstring FileName);
	void SaveToFile(wstring FileName, bool Delay);

	void Autosave(bool Delay = true);

	void Tick(double dt);

	void CreateNewFile(void);
	void CreateNewHistory(void);
	int32 CreateCopyOfCurrentState(void);
	bool LoadHistoryByID(int32 ID);
	uint32 GetDeletedHistoryCount(void);
	uint32 GetHistoryCount(void);
	void DeleteHistory(int32 ID);
	void UndoLastHistoryDeletion(void);
	void ReloadCurrentHistory(void);
	bool HaveCurrentHistory(void);
	int32 GetCurrentHistoryID(void);
	void SetCurrentHistoryByID(int32 ID);
	
	void SetupKinematicMode(void);
	void CancelKinematicMode(void);
	bool IsInKinematicMode(void);

	const int PendingTimeout = 250;

	void PushStateFrame(const wstring Sender);
	void PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender);
	void PopAndDeserializeStateFrame(bool Forward);
	
	float GetAnimationPosition(void);
	void SetAnimationPosition(float Position);

	float GetAnimationLength(void);
	void SetAnimationLength(float Length);

	vector<TimelineItem> GetTimelineItems(void);
	void SetTimelineItems(vector<TimelineItem> Items);

	void SetAnimationPlayState(bool PlayAnimation);
	bool IsAnimationPlaying(void);
	void SetAnimationPlayLoop(bool LoopAnimation);
	bool IsAnimationLooped(void);
	float GetAnimationPlaySpeed(void);
	void SetAnimationPlaySpeed(float Speed);
} SerializationManager;