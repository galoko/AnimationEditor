#include "SerializationManager.hpp"

#include <locale>
#include <codecvt>
#include <algorithm>

#include <tixml2ex.h>

#include "CharacterManager.hpp"
#include "InputManager.hpp"
#include "PoseManager.hpp"
#include "Render.hpp"
#include "Form.hpp"

void SerializationManager::Initialize(const wstring WorkingDirectory)
{
	SettingsFileName = WorkingDirectory + L"\\Settings.xml";

	NextStateHistoryID = 1;

	LoadSettings();

	StartBackgroundThread();
}

void SerializationManager::Serialize(void)
{
	if (!HaveCurrentHistory())
		return;

	SingleSerializedState& State = GetCurrentHistory()->CurrentState;

	if (State.HaveCharState)
		CharacterManager::GetInstance().Serialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Serialize(State.InputState);

	if (State.HavePoseState)
		PoseManager::GetInstance().Serialize(State.PoseState);

	if (State.HaveRenderState)
		Render::GetInstance().Serialize(State.RenderState);
}

void SerializationManager::Deserialize(void)
{
	if (!HaveCurrentHistory())
		return;

	PendingLastTime = GetTickCount64();

	SingleSerializedState& State = GetCurrentHistory()->CurrentState;

	if (State.HaveRenderState)
		Render::GetInstance().Deserialize(State.RenderState);

	if (State.HaveCharState)
		CharacterManager::GetInstance().Deserialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Deserialize(State.InputState);

	if (State.HavePoseState)
		PoseManager::GetInstance().Deserialize(State.PoseState);
}

void SerializationManager::Serialize(SerializeSerializedState& State)
{
	State.AnimationPosition = this->GetAnimationPosition();
	State.AnimationLength = this->GetAnimationLength();
	State.KinematicModeFlag = this->IsInKinematicMode();
	State.PlayAnimaionFlag = this->IsAnimationPlaying();
	State.LoopAnimationFlag = this->IsAnimationLooped();
	State.PlaySpeed = this->GetAnimationPlaySpeed();
}

void SerializationManager::Deserialize(SerializeSerializedState & State)
{
	Form::UpdateLock Lock;

	SetAnimationPosition(State.AnimationPosition);
	SetAnimationLength(State.AnimationLength);
	SetAnimationPlayState(State.PlayAnimaionFlag);
	SetAnimationPlayLoop(State.LoopAnimationFlag);
	SetAnimationPlaySpeed(State.PlaySpeed);
	
	if (State.KinematicModeFlag)
		SetupKinematicMode();
	else
		CancelKinematicMode();

	Form::GetInstance().UpdateTimeline();
}

void SerializationManager::Autosave(bool Delay)
{
	if (IsFileOpen())
		SaveToFile(LastFileName, Delay);
}

void SerializationManager::Tick(double dt)
{
	ULONGLONG Now = GetTickCount64();
	if (LastAutosaveTime + AutosaveInterval < Now)
		Autosave();

	if (IsAnimationPlaying()) {

		float Len = GetAnimationLength();

		float NewPosition = GetAnimationPosition() + (float)dt * PlaySpeed;
		if (IsAnimationLooped()) {
			NewPosition = fmod(NewPosition, Len);
			if (NewPosition < 0)
				NewPosition += Len;
		}
		else
			NewPosition = clamp(NewPosition, 0.0f, Len);

		SetAnimationPosition(NewPosition);

		if (!IsAnimationLooped()) {

			bool IsEnded;

			if (PlaySpeed > 0)
				IsEnded = GetAnimationPosition() >= Len;
			else
				IsEnded = GetAnimationPosition() <= 0;

			if (IsEnded)
				SetAnimationPlayState(false);
		}
	}
}

void SerializationManager::InternalPushStateFrame(bool Forward)
{
	if (!HaveCurrentHistory()) {
		assert(false);
		return;
	}

	SingleSerializedState& State = GetCurrentHistory()->CurrentState;
	State.HaveCharState = true;
	State.HaveInputState = true;
	State.HavePoseState = true;
	State.HaveRenderState = false;
	Serialize();

	vector<SingleSerializedState>& DestStates =
		Forward ? GetCurrentHistory()->FutureStates : GetCurrentHistory()->PreviousStates;

	DestStates.push_back(State);

	LimitFrames(DestStates, MaxFrames);

	State.PendingID = PendingNone;
	PendingLastTime = 0;
}

void SerializationManager::LimitFrames(vector<SingleSerializedState>& Frames, int Limit)
{
	if (Frames.size() > Limit) {
		move(Frames.end() - Limit, Frames.end(), Frames.begin());
		Frames.resize(Limit);
	}
}

void SerializationManager::CreateNewFile(void)
{
	CancelKinematicMode();

	Histories.clear();

	CharacterManager::GetInstance().Reset();

	CreateNewHistory();

	Deserialize();

	SerializeSerializedState State = {};
	Deserialize(State);

	Form::GetInstance().FullUpdate();
}

void SerializationManager::CreateNewHistory(void)
{
	SerializedStateHistory History = {};

	History.ID = NextStateHistoryID++;

	CharacterManager::GetInstance().Serialize(History.CurrentState.CharState);
	History.CurrentState.HaveCharState = true;
	History.CurrentState.HaveInputState = true;
	History.CurrentState.HavePoseState = true;
	History.CurrentState.HaveRenderState = false;

	Histories.push_back(History);
}

int32 SerializationManager::CreateCopyOfCurrentState(void)
{
	Form::UpdateLock Lock;

	CharacterSerializedState CharState;
	CharacterManager::GetInstance().Serialize(CharState);

	CancelKinematicMode();

	SingleSerializedState& State = GetCurrentHistory()->CurrentState;
	State.HaveCharState = true;
	State.HaveInputState = true;
	State.HavePoseState = true;
	State.HaveRenderState = false;
	Serialize();

	SerializedStateHistory Copy = *GetCurrentHistory();

	Copy.ID = NextStateHistoryID++;
	Copy.CurrentState.CharState = CharState;

	Histories.push_back(Copy);

	Deserialize();

	Form::GetInstance().UpdateTimeline();

	return GetCurrentHistoryID();
}

bool SerializationManager::LoadHistoryByID(int32 ID)
{
	if (GetLatestHistory()->ID == ID)
		return true;

	auto LatestHistory = GetLatestHistory();

	auto NextHistory = GetHistoryByID(ID);
	if (NextHistory == Histories.end())
		return false;

	if (HaveCurrentHistory()) {

		auto State = GetCurrentHistory();
		State->CurrentState.HaveCharState = true;
		State->CurrentState.HaveInputState = true;
		State->CurrentState.HavePoseState = true;
		State->CurrentState.HaveRenderState = false;
		Serialize();
	}

	iter_swap(LatestHistory, NextHistory);

	ReloadCurrentHistory();

	return true;
}

uint32 SerializationManager::GetDeletedHistoryCount(void)
{
	uint32 Result = 0;

	for (SerializedStateHistory& History : Histories)
		if (History.IsDeleted)
			Result++;
		else
			break;

	return Result;
}

uint32 SerializationManager::GetHistoryCount(void)
{
	return (int32)Histories.size() - GetDeletedHistoryCount();
}

void SerializationManager::DeleteHistory(int32 ID)
{
	auto History = GetHistoryByID(ID);
	if (History != Histories.end()) {

		bool ShouldReloadCurrentHistory = HaveCurrentHistory() && History->ID == GetCurrentHistoryID();

		History->IsDeleted = true;

		if (GetDeletedHistoryCount() <= 3)
			rotate(Histories.begin(), History, History + 1);
		else
			Histories.erase(History);
	
		if (GetHistoryCount() <= 0) {

			CharacterManager::GetInstance().Reset();
			CreateNewHistory();

			ShouldReloadCurrentHistory = true;
		}

		if (ShouldReloadCurrentHistory) 
			ReloadCurrentHistory();

		Form::GetInstance().UpdateTimeline();
	}
}

void SerializationManager::UndoLastHistoryDeletion(void)
{
	if (!Histories.empty()) {

		auto LastDeletedHistory = Histories.begin();
		if (!LastDeletedHistory->IsDeleted)
			return;

		LastDeletedHistory->IsDeleted = false;

		rotate(LastDeletedHistory, LastDeletedHistory + 1, Histories.end());

		ReloadCurrentHistory();

		Form::GetInstance().UpdateTimeline();
	}
}

void SerializationManager::ReloadCurrentHistory(void)
{
	CancelKinematicMode();

	GetCurrentHistory()->CurrentState.HaveRenderState = false;
	Deserialize();
}

bool SerializationManager::HaveCurrentHistory(void)
{
	return GetCurrentHistoryID() > 0;
}

int32 SerializationManager::GetCurrentHistoryID(void)
{
	if (!IsInKinematicMode())
		return GetCurrentHistory()->ID;
	else
		return 0;
}

void SerializationManager::SetCurrentHistoryByID(int32 ID)
{
	if (ID > 0) {

		CancelKinematicMode();
		LoadHistoryByID(ID);
	}
	else
		SetupKinematicMode();

	Form::GetInstance().UpdateTimeline();
}

void SerializationManager::SetupKinematicMode(void)
{
	if (KinematicModeFlag)
		return;

	// save current state
	auto State = GetCurrentHistory();
	State->CurrentState.HaveCharState = true;
	State->CurrentState.HaveInputState = true;
	State->CurrentState.HavePoseState = true;
	State->CurrentState.HaveRenderState = true;
	Serialize();

	KinematicModeFlag = true;

	ProcessAnimaiton();

	Form::GetInstance().FullUpdate();
}

void SerializationManager::CancelKinematicMode(void)
{
	if (!KinematicModeFlag)
		return;

	Form::UpdateLock Lock;

	SetAnimationPlayState(false);

	KinematicModeFlag = false;

	GetCurrentHistory()->CurrentState.HaveRenderState = false;
	GetCurrentHistory()->CurrentState.InputState.State = None;
	Deserialize();

	Form::GetInstance().FullUpdate();
}

bool SerializationManager::IsInKinematicMode(void)
{
	return KinematicModeFlag;
}

bool SerializationManager::IsFileOpen(void)
{
	return LastFileName != L"";
}

vector<SerializedStateHistory>::iterator SerializationManager::GetLatestHistory(void)
{
	return Histories.end() - 1;
}

vector<SerializedStateHistory>::iterator SerializationManager::GetCurrentHistory(void)
{
	if (!IsInKinematicMode())
		return GetLatestHistory();
	else
		return Histories.end();
}

vector<SerializedStateHistory>::iterator SerializationManager::GetHistoryByID(int32 ID)
{
	for (std::vector<SerializedStateHistory>::iterator History = Histories.begin(); History != Histories.end(); ++History) {
		if (History->ID == ID)
			return History;
	}

	return Histories.end();
}

wstring ChangeFileExt(const wstring& FileName, const wstring& NewExt) {

	size_t LastSlashPos = FileName.find_last_of(L"\\/");
	size_t LastDotPos = FileName.find_last_of(L".");

	if (LastSlashPos != string::npos && LastDotPos != string::npos && LastDotPos < LastSlashPos)
		LastDotPos = string::npos;

	wstring NoExt;
	if (LastDotPos != string::npos)
		NoExt = FileName.substr(0, LastDotPos);
	else
		NoExt = FileName;

	return NoExt + NewExt;
}

void SerializationManager::SafeSaveDocumentToFile(XMLDocument& Document, const wstring FileName, int BackupCount)
{

	wstring TempFileName = ChangeFileExt(FileName, L".tmp");
	wstring XmlFileName = ChangeFileExt(FileName, L".xml");
	wstring BackupFileName = ChangeFileExt(FileName, L".backup");

	FILE* File = _wfopen(TempFileName.c_str(), L"wb");
	Document.SaveFile(File, false);
	fclose(File);

	BOOL Result;

	Result = MoveFileEx(XmlFileName.c_str(), BackupFileName.c_str(), MOVEFILE_REPLACE_EXISTING) || (GetLastError() == ERROR_FILE_NOT_FOUND);
	if (Result)
		Result = MoveFileEx(TempFileName.c_str(), XmlFileName.c_str(), MOVEFILE_REPLACE_EXISTING);

	if (!Result)
		printf("Failed to rename tmp file\n");
}

void SerializationManager::StartBackgroundThread(void)
{
	HANDLE BackgroundThreadHandle = CreateThread(NULL, 0, BackgroundStaticThreadProc, nullptr, 0, nullptr);
	if (BackgroundThreadHandle == 0)
		printf("Failed to create a thread\n");
}

DWORD SerializationManager::BackgroundStaticThreadProc(LPVOID lpThreadParameter)
{
	SerializationManager::GetInstance().BackgroundThreadProc();
	return 0;
}

void SerializationManager::BackgroundThreadProc(void)
{
	while (true) {

		FileSaveRequest* Request;
		DelayedFileSaveRequests.wait_dequeue(Request);

		ExecuteFileSaveRequest(*Request);

		delete Request;
	}
}

void SerializationManager::ExecuteFileSaveRequest(FileSaveRequest& Request)
{
	XMLDocument Document;

	XMLNode* Root = Document.NewElement("Root");
	Document.InsertFirstChild(Root);

	for (SerializedStateHistory& States : Request.Histories) {

		XMLElement* StatesElement = Document.NewElement("States");
		Root->InsertEndChild(StatesElement);

		SaveStates(States, Document, StatesElement);
	}

	Request.State.SaveToXML(Document, Root);

	SafeSaveDocumentToFile(Document, Request.FileName, MaxBackupCount);
}

void SerializationManager::PushStateFrame(const wstring Sender)
{
	if (!HaveCurrentHistory())
		return;

	printf("PUSH STATE, SENDER: %ws\n", Sender.c_str());

	GetCurrentHistory()->FutureStates.clear();

	InternalPushStateFrame(false);
}

void SerializationManager::PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender)
{
	if (!HaveCurrentHistory())
		return;

	assert(PendingID != PendingNone);

	ULONGLONG Now = GetTickCount64();

	if (GetCurrentHistory()->CurrentState.PendingID != PendingID || PendingLastTime + PendingTimeout <= Now)
		PushStateFrame(L"Pending (" + Sender + L")");

	GetCurrentHistory()->CurrentState.PendingID = PendingID;
	PendingLastTime = Now;
}

void SerializationManager::PopAndDeserializeStateFrame(bool Forward)
{
	if (!HaveCurrentHistory())
		return;

	vector<SingleSerializedState>& StackToUse = Forward ? GetCurrentHistory()->FutureStates : GetCurrentHistory()->PreviousStates;

	if (!StackToUse.empty()) {

		InternalPushStateFrame(!Forward);

		GetCurrentHistory()->CurrentState = StackToUse.back();
		StackToUse.pop_back();

		Deserialize();
	}
}

float SerializationManager::GetAnimationPosition(void)
{
	return AnimationPosition;
}

void SerializationManager::SetAnimationPosition(float Position)
{
	AnimationPosition = Position;

	ProcessAnimaiton();

	Form::GetInstance().UpdateTimeline();
}

float SerializationManager::GetAnimationLength(void)
{
	return AnimationLength;
}

void SerializationManager::SetAnimationLength(float Length)
{
	AnimationLength = std::max(Length, 0.1f);

	ProcessAnimaiton();

	Form::GetInstance().UpdateTimeline();
}

void SerializationManager::GetStatesAndTFromPosition(float Position, float Length, CharacterSerializedState*& PrevState, CharacterSerializedState*& NextState, float& t)
{
	vector<CharacterSerializedState*> States;

	for (SerializedStateHistory& History : Histories)
		if (!History.IsDeleted)
			States.push_back(&History.CurrentState.CharState);

	if (States.size() == 0) {
		NextState = nullptr;
		PrevState = nullptr;
		t = 0.0f;
		return;
	}

	if (States.size() == 1) {

		PrevState = States.front();
		NextState = States.back();
		t = 0.0f;
		return;
	}

	struct {
		bool operator()(CharacterSerializedState*& a, CharacterSerializedState*& b) const
		{
			return a->AnimationTimestamp < b->AnimationTimestamp;
		}
	} StateComparer;

	sort(States.begin(), States.end(), StateComparer);

	int StartIndex = 0;
	int EndIndex = (int)States.size() - 1;

	uint32 Pos = (uint32)(Position * 1000.0f);
	uint32 Len = (uint32)(Length * 1000.0f);

	uint32 StartPos = States.front()->AnimationTimestamp;
	uint32 EndPos = States.back()->AnimationTimestamp;

	uint32 StartLen = StartPos;
	uint32 EndLen = Len - EndPos;

	if (Pos < StartPos) {
		PrevState = States.back();
		NextState = States.front();
		t = (EndLen + Pos) / (float)(StartLen + EndLen);
	}
	else
	if (Pos > EndPos) {
		PrevState = States.back();
		NextState = States.front();
		t = (Pos - EndPos) / (float)(StartLen + EndLen);
	} 
	else {

		for (int Index = 1; Index < States.size(); Index++) {

			CharacterSerializedState* State = States[Index];

			if (Pos <= State->AnimationTimestamp) {
				NextState = State;
				PrevState = States[Index - 1];
				t = (Pos - PrevState->AnimationTimestamp) / (float)(NextState->AnimationTimestamp - PrevState->AnimationTimestamp);
				return;
			}
		}

		throw new runtime_error("logical error in GetStatesAndTFromPosition");
	}
}

vec3 lerp(vec3& a, vec3& b, float t) {
	return a * (1 - t) + b * t;
}

void SerializationManager::ProcessAnimaiton(void)
{
	if (IsInKinematicMode()) {

		CharacterSerializedState *PrevState, *NextState;
		float t;

		GetStatesAndTFromPosition(GetAnimationPosition(), GetAnimationLength(), PrevState, NextState, t);

		if (PrevState != nullptr && NextState != nullptr) {

			CharacterSerializedState InterpolatedState = {};
			InterpolatedState.AnimationTimestamp = uint32(GetAnimationPosition() * 1000.0f);

			InterpolatedState.Position = lerp(PrevState->Position, NextState->Position, t);

			for (int Index = 0; Index < PrevState->Bones.size(); Index++) {

				SerializedBone PrevBone = PrevState->Bones[Index];
				SerializedBone NextBone = NextState->Bones[Index];

				SerializedBone InterpolatedBone;
				InterpolatedBone.Name = PrevBone.Name;
				InterpolatedBone.Rotation = slerp(PrevBone.Rotation, NextBone.Rotation, t);

				InterpolatedState.Bones.push_back(InterpolatedBone);
			}

			CharacterManager::GetInstance().Deserialize(InterpolatedState);
		}
	}
}

vector<TimelineItem> SerializationManager::GetTimelineItems(void)
{
	vector<TimelineItem> Result;

	for (SerializedStateHistory& History : Histories) 
		if (!History.IsDeleted) {
			TimelineItem Item = { History.ID, History.CurrentState.CharState.AnimationTimestamp / 1000.0f };
			Result.push_back(Item);
		}

	return Result;
}

void SerializationManager::SetTimelineItems(vector<TimelineItem> Items)
{
	for (TimelineItem& Item : Items) {

		uint32 Timestamp = (uint32)(Item.Position * 1000.0f);

		if (HaveCurrentHistory() && GetCurrentHistoryID() == Item.ID)
			CharacterManager::GetInstance().AnimationTimestamp = Timestamp;

		auto History = GetHistoryByID(Item.ID);
		if (History != Histories.end())
			History->CurrentState.CharState.AnimationTimestamp = Timestamp;
	}
}

void SerializationManager::SetAnimationPlayState(bool PlayAnimation)
{
	PlayAnimaionFlag = PlayAnimation;

	if (IsAnimationPlaying()) {

		SetupKinematicMode();

		if (!IsAnimationLooped()) {

			float Pos = GetAnimationPosition();
			float Len = GetAnimationLength();

			if (PlaySpeed > 0) {

				if (Pos >= Len)
					SetAnimationPosition(0.0f);
			}
			else {

				if (Pos <= 0)
					SetAnimationPosition(Len);
			}
		}
	}
}

bool SerializationManager::IsAnimationPlaying(void)
{
	return PlayAnimaionFlag;
}

void SerializationManager::SetAnimationPlayLoop(bool LoopAnimation)
{
	LoopAnimationFlag = LoopAnimation;
}

bool SerializationManager::IsAnimationLooped(void)
{
	return LoopAnimationFlag;
}

float SerializationManager::GetAnimationPlaySpeed(void)
{
	return PlaySpeed;
}

void SerializationManager::SetAnimationPlaySpeed(float Speed)
{
	PlaySpeed = Speed;

	Form::GetInstance().UpdateTimeline();
}

// Utils

wstring s2ws(const string& str)
{
	wstring Result(str.begin(), str.end());
	return Result;
}

string ws2s(const wstring& wstr)
{
	string Result(wstr.begin(), wstr.end());
	return Result;
}

float attribute_float_value(XMLElement* Element, const char* AttributeName, float DefaultValue = 0.0f) {

	float Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryFloatValue(&Result) != XML_SUCCESS)
		Result = DefaultValue;

	return Result;
}

bool attribute_bool_value(XMLElement* Element, const char* AttributeName, bool DefaultValue = true) {

	bool Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryBoolValue(&Result) != XML_SUCCESS)
		Result = DefaultValue;

	return Result;
}

uint32 attribute_uint32_value(XMLElement* Element, const char* AttributeName) {

	uint32 Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryUnsignedValue(&Result) != XML_SUCCESS)
		Result = true;

	return Result;
}

void SerializationManager::LoadFromFile(wstring FileName)
{
	Form::UpdateLock Lock;

	// fast cleanup
	Histories.clear();

	FILE* File = _wfopen(FileName.c_str(), L"rb");
	if (File != nullptr) {

		XMLDocument Document;
		Document.LoadFile(File);
		fclose(File);

		XMLNode* Root = Document.FirstChildElement("Root");
		if (Root != nullptr) {

			for (auto StatesElement : selection(Root->ToElement(), "States")) {

				SerializedStateHistory States;

				LoadStates(States, Document, StatesElement);

				Histories.push_back(States);
			}

			struct {
				bool operator()(SerializedStateHistory& a, SerializedStateHistory& b) const
				{
					if (a.IsDeleted && !b.IsDeleted)
						return true;

					if (!a.IsDeleted && b.IsDeleted)
						return false;

					return a.ID < b.ID;
				}
			} HistoryComparer;

			sort(Histories.begin(), Histories.end(), HistoryComparer);

			if (!Histories.empty()) {

				GetCurrentHistory()->CurrentState.InputState.State = None;
				Deserialize();
			}

			SerializeSerializedState State;
			if (State.LoadFromXML(Document, Root))
				Deserialize(State);
		}

		LastAutosaveTime = GetTickCount64();
	}

	if (Histories.empty()) {

		CharacterManager::GetInstance().Reset();
		CreateNewFile();

		FileName = L"";
	}

	if (LastFileName != FileName) {

		LastFileName = FileName;

		SaveSettings();
	}

	Form::GetInstance().UpdateTimeline();
}

void SerializationManager::SaveToFile(wstring FileName, bool Delay)
{
	FileName = ChangeFileExt(FileName, L".xml");

	if (HaveCurrentHistory()) {

		SingleSerializedState& State = GetCurrentHistory()->CurrentState;
		State.HaveCharState = true;
		State.HaveInputState = true;
		State.HavePoseState = true;
		State.HaveRenderState = true;
		Serialize();
	}
	else {

		SingleSerializedState& State = GetLatestHistory()->CurrentState;

		Render::GetInstance().Serialize(State.RenderState);
		State.HaveRenderState = true;
	}

	if (Delay) {
		FileSaveRequest* Request = new FileSaveRequest();
		Request->Histories = Histories;
		Serialize(Request->State);
		Request->FileName = FileName;

		DelayedFileSaveRequests.enqueue(Request);
	}
	else {
		FileSaveRequest Request;
		Request.Histories = Histories;
		Serialize(Request.State);
		Request.FileName = FileName;

		ExecuteFileSaveRequest(Request);
	}

	if (LastFileName != FileName) {

		LastFileName = FileName;

		SaveSettings();
	}

	LastAutosaveTime = GetTickCount64();
}

void SerializationManager::LoadSettings(void)
{
	FILE* File = _wfopen(SettingsFileName.c_str(), L"rb");
	if (File == nullptr)
		return;

	XMLDocument Document;
	Document.LoadFile(File);
	fclose(File);

	XMLNode* Settings = Document.FirstChildElement("Settings");
	if (Settings != nullptr) {

		XMLElement* LastFile = Settings->FirstChildElement("LastFile");
		if (LastFile != nullptr)
			LastFileName = s2ws(attribute_value(LastFile, "Name"));
	}

	LoadFromFile(LastFileName);
}

void SerializationManager::SaveSettings(void)
{
	XMLDocument Document;

	XMLNode* Settings = Document.NewElement("Settings");
	Document.InsertFirstChild(Settings);

	XMLElement* LastFile = Document.NewElement("LastFile");
	Settings->InsertEndChild(LastFile);
	LastFile->SetAttribute("Name", ws2s(LastFileName).c_str());

	SafeSaveDocumentToFile(Document, SettingsFileName, 0);
}

void SerializationManager::LoadState(SingleSerializedState& State, XMLDocument& Document, XMLNode* Root)
{
	State.HaveCharState = false;
	State.HaveInputState = false;
	State.HavePoseState = false;
	State.HaveRenderState = false;

	XMLElement* Pending = Root->FirstChildElement("Pending");
	if (Pending != nullptr)
		State.PendingID = (SerializationPendingID)attribute_uint32_value(Pending, "ID");

	if (State.CharState.LoadFromXML(Document, Root))
		State.HaveCharState = true;

	if (State.InputState.LoadFromXML(Document, Root))
		State.HaveInputState = true;

	if (State.PoseState.LoadFromXML(Document, Root))
		State.HavePoseState = true;

	if (State.RenderState.LoadFromXML(Document, Root))
		State.HaveRenderState = true;
}

void SerializationManager::SaveState(SingleSerializedState& State, XMLDocument& Document, XMLNode* Root)
{
	XMLElement* PendingState = Document.NewElement("Pending");
	PendingState->SetAttribute("ID", (uint32)State.PendingID);
	Root->InsertEndChild(PendingState);

	if (State.HaveCharState)
		State.CharState.SaveToXML(Document, Root);
	if (State.HaveInputState)
		State.InputState.SaveToXML(Document, Root);
	if (State.HavePoseState)
		State.PoseState.SaveToXML(Document, Root);
	if (State.HaveRenderState)
		State.RenderState.SaveToXML(Document, Root);
}

void SerializationManager::LoadStates(SerializedStateHistory& States, XMLDocument& Document, XMLNode* Root)
{
	XMLElement* CurrentStateElement = Root->FirstChildElement("CurrentState");
	if (CurrentStateElement != nullptr) {

		LoadState(States.CurrentState, Document, CurrentStateElement);

		States.PreviousStates.clear();
		for (auto PreviousStateElement : selection(Root->ToElement(), "PreviousState")) {

			SingleSerializedState PreviousState;

			LoadState(PreviousState, Document, PreviousStateElement);

			States.PreviousStates.push_back(PreviousState);
		}

		LimitFrames(States.PreviousStates, MaxFrames);

		States.FutureStates.clear();
		for (auto FutureStateElement : selection(Root->ToElement(), "FutureState")) {

			SingleSerializedState FutureState;

			LoadState(FutureState, Document, FutureStateElement);

			States.FutureStates.push_back(FutureState);
		}
		LimitFrames(States.FutureStates, MaxFrames);
	}
	else
		States = { };

	States.IsDeleted = attribute_bool_value(Root->ToElement(), "IsDeleted", false);

	States.ID = NextStateHistoryID++;
}

void SerializationManager::SaveStates(SerializedStateHistory& States, XMLDocument& Document, XMLNode* Root)
{
	Root->ToElement()->SetAttribute("IsDeleted", States.IsDeleted);

	for (SingleSerializedState& State : States.PreviousStates) {

		XMLNode* PreviousState = Document.NewElement("PreviousState");
		Root->InsertEndChild(PreviousState);

		SaveState(State, Document, PreviousState);
	}

	XMLNode* CurrentState = Document.NewElement("CurrentState");
	Root->InsertEndChild(CurrentState);

	SaveState(States.CurrentState, Document, CurrentState);

	for (SingleSerializedState& State : States.FutureStates) {

		XMLNode* FutureState = Document.NewElement("FutureState");
		Root->InsertEndChild(FutureState);

		SaveState(State, Document, FutureState);
	}
}

// CharacterSerializedState

void CharacterSerializedState::SaveToXML(XMLDocument& Document, XMLNode *Root)
{
	XMLNode* CharState = Document.NewElement("CharState");

	XMLElement* Animation = Document.NewElement("Animation");
	Animation->SetAttribute("Timestamp", this->AnimationTimestamp);
	CharState->InsertEndChild(Animation);

	XMLElement* Position = Document.NewElement("Position");
	Position->SetAttribute("X", this->Position.x);
	Position->SetAttribute("Y", this->Position.y);
	Position->SetAttribute("Z", this->Position.z);
	CharState->InsertEndChild(Position);

	for (SerializedBone& Bone : Bones) {

		XMLElement* BoneElement = Document.NewElement("Bone");
		BoneElement->SetAttribute("Name", ws2s(Bone.Name).c_str());
		CharState->InsertEndChild(BoneElement);

		XMLElement* Rotation = Document.NewElement("Rotation");
		Rotation->SetAttribute("X", Bone.Rotation.x);
		Rotation->SetAttribute("Y", Bone.Rotation.y);
		Rotation->SetAttribute("Z", Bone.Rotation.z);
		Rotation->SetAttribute("W", Bone.Rotation.w);
		BoneElement->InsertEndChild(Rotation);
	}

	Root->InsertEndChild(CharState);
}

bool CharacterSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = { };

	XMLNode* CharState = Root->FirstChildElement("CharState");
	if (CharState == nullptr)
		return false;

	XMLElement* Animation = CharState->FirstChildElement("Animation");
	if (Animation != nullptr)
		this->AnimationTimestamp = attribute_uint32_value(Animation, "Timestamp");

	XMLElement* Position = CharState->FirstChildElement("Position");
	if (Position != nullptr) {
		this->Position.x = attribute_float_value(Position, "X");
		this->Position.y = attribute_float_value(Position, "Y");
		this->Position.z = attribute_float_value(Position, "Z");
	}

	for (auto BoneElement : selection(CharState->ToElement(), "Bone")) {

		SerializedBone SerializedBone = { };

		SerializedBone.Name = s2ws(attribute_value(BoneElement, "Name"));

		XMLElement* Rotation = BoneElement->FirstChildElement("Rotation");
		if (Rotation != nullptr) {
			SerializedBone.Rotation.x = attribute_float_value(Rotation, "X");
			SerializedBone.Rotation.y = attribute_float_value(Rotation, "Y");
			SerializedBone.Rotation.z = attribute_float_value(Rotation, "Z");
			SerializedBone.Rotation.w = attribute_float_value(Rotation, "W");
		}

		this->Bones.push_back(SerializedBone);
	}

	return true;
}

// InputSerializedState

void InputSerializedState::SaveToXML(XMLDocument & Document, XMLNode* Root)
{
	XMLNode* InputState = Document.NewElement("InputState");

	XMLElement* BoneElement = Document.NewElement("Bone");
	BoneElement->SetAttribute("Name", ws2s(this->BoneName).c_str());
	InputState->InsertEndChild(BoneElement);

	XMLElement* StateElement = Document.NewElement("State");
	StateElement->SetAttribute("Value", this->State);
	InputState->InsertEndChild(StateElement);

	XMLElement* PlaneModeElement = Document.NewElement("PlaneMode");
	PlaneModeElement->SetAttribute("Value", this->PlaneMode);
	InputState->InsertEndChild(PlaneModeElement);

	XMLElement* LocalPoint = Document.NewElement("LocalPoint");
	LocalPoint->SetAttribute("X", this->LocalPoint.x);
	LocalPoint->SetAttribute("Y", this->LocalPoint.y);
	LocalPoint->SetAttribute("Z", this->LocalPoint.z);
	InputState->InsertEndChild(LocalPoint);

	XMLElement* WorldPoint = Document.NewElement("WorldPoint");
	WorldPoint->SetAttribute("X", this->WorldPoint.x);
	WorldPoint->SetAttribute("Y", this->WorldPoint.y);
	WorldPoint->SetAttribute("Z", this->WorldPoint.z);
	InputState->InsertEndChild(WorldPoint);

	Root->InsertEndChild(InputState);
}

bool InputSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = { };

	XMLNode* InputState = Root->FirstChildElement("InputState");
	if (InputState == nullptr)
		return false;

	XMLElement* Bone = InputState->FirstChildElement("Bone");
	if (Bone != nullptr)
		this->BoneName = s2ws(attribute_value(Bone, "Name"));

	XMLElement* StateElement = InputState->FirstChildElement("State");
	if (StateElement != nullptr)
		this->State = attribute_uint32_value(StateElement, "Value");

	XMLElement* PlaneModeElement = InputState->FirstChildElement("PlaneMode");
	if (PlaneModeElement != nullptr)
		this->PlaneMode = attribute_uint32_value(PlaneModeElement, "Value");

	XMLElement* LocalPoint = InputState->FirstChildElement("LocalPoint");
	if (LocalPoint != nullptr) {
		this->LocalPoint.x = attribute_float_value(LocalPoint, "X");
		this->LocalPoint.y = attribute_float_value(LocalPoint, "Y");
		this->LocalPoint.z = attribute_float_value(LocalPoint, "Z");
	}

	XMLElement* WorldPoint = InputState->FirstChildElement("WorldPoint");
	if (WorldPoint != nullptr) {
		this->WorldPoint.x = attribute_float_value(WorldPoint, "X");
		this->WorldPoint.y = attribute_float_value(WorldPoint, "Y");
		this->WorldPoint.z = attribute_float_value(WorldPoint, "Z");
	}

	return true;
}

// PoseSerializedState

void PoseSerializedState::SaveToXML(XMLDocument& Document, XMLNode* Root)
{
	XMLNode* PoseState = Document.NewElement("PoseState");

	for (SerializedPoseContext& Context : Contexts) {

		XMLElement* ContextElement = Document.NewElement("Context");
		ContextElement->SetAttribute("Name", ws2s(Context.BoneName).c_str());
		PoseState->InsertEndChild(ContextElement);

		XMLElement* Blocking = Document.NewElement("Blocking");
		Blocking->SetAttribute("XPos", Context.Blocking.XPos);
		Blocking->SetAttribute("YPos", Context.Blocking.YPos);
		Blocking->SetAttribute("ZPos", Context.Blocking.ZPos);
		Blocking->SetAttribute("XAxis", Context.Blocking.XAxis);
		Blocking->SetAttribute("YAxis", Context.Blocking.YAxis);
		Blocking->SetAttribute("ZAxis", Context.Blocking.ZAxis);
		ContextElement->InsertEndChild(Blocking);

		XMLElement* IsActive = Document.NewElement("Pinpoint");
		IsActive->SetAttribute("IsActive", Context.IsActive);
		ContextElement->InsertEndChild(IsActive);

		XMLElement* SrcLocalPoint = Document.NewElement("SrcLocalPoint");
		SrcLocalPoint->SetAttribute("X", Context.SrcLocalPoint.x);
		SrcLocalPoint->SetAttribute("Y", Context.SrcLocalPoint.y);
		SrcLocalPoint->SetAttribute("Z", Context.SrcLocalPoint.z);
		ContextElement->InsertEndChild(SrcLocalPoint);

		XMLElement* DestWorldPoint = Document.NewElement("DestWorldPoint");
		DestWorldPoint->SetAttribute("X", Context.DestWorldPoint.x);
		DestWorldPoint->SetAttribute("Y", Context.DestWorldPoint.y);
		DestWorldPoint->SetAttribute("Z", Context.DestWorldPoint.z);
		ContextElement->InsertEndChild(DestWorldPoint);
	}

	Root->InsertEndChild(PoseState);
}

bool PoseSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = { };

	XMLNode* PoseState = Root->FirstChildElement("PoseState");
	if (PoseState == nullptr)
		return false;

	for (auto BoneElement : selection(PoseState->ToElement(), "Context")) {

		SerializedPoseContext SerializedContext = {};

		SerializedContext.BoneName = s2ws(attribute_value(BoneElement, "Name"));

		XMLElement* Blocking = BoneElement->FirstChildElement("Blocking");
		if (Blocking != nullptr) {
			SerializedContext.Blocking.XPos = attribute_bool_value(Blocking, "XPos");
			SerializedContext.Blocking.YPos = attribute_bool_value(Blocking, "YPos");
			SerializedContext.Blocking.ZPos = attribute_bool_value(Blocking, "ZPos");
			SerializedContext.Blocking.XAxis = attribute_bool_value(Blocking, "XAxis");
			SerializedContext.Blocking.YAxis = attribute_bool_value(Blocking, "YAxis");
			SerializedContext.Blocking.ZAxis = attribute_bool_value(Blocking, "ZAxis");
		}

		XMLElement* Pinpoint = BoneElement->FirstChildElement("Pinpoint");
		if (Pinpoint != nullptr)
			SerializedContext.IsActive = attribute_bool_value(Pinpoint, "IsActive");

		XMLElement* SrcLocalPoint = BoneElement->FirstChildElement("SrcLocalPoint");
		if (SrcLocalPoint != nullptr) {
			SerializedContext.SrcLocalPoint.x = attribute_float_value(SrcLocalPoint, "X");
			SerializedContext.SrcLocalPoint.y = attribute_float_value(SrcLocalPoint, "Y");
			SerializedContext.SrcLocalPoint.z = attribute_float_value(SrcLocalPoint, "Z");
		}

		XMLElement* DestWorldPoint = BoneElement->FirstChildElement("DestWorldPoint");
		if (DestWorldPoint != nullptr) {
			SerializedContext.DestWorldPoint.x = attribute_float_value(DestWorldPoint, "X");
			SerializedContext.DestWorldPoint.y = attribute_float_value(DestWorldPoint, "Y");
			SerializedContext.DestWorldPoint.z = attribute_float_value(DestWorldPoint, "Z");
		}

		this->Contexts.push_back(SerializedContext);
	}

	return true;
}

// RenderSerializedState

void RenderSerializedState::SaveToXML(XMLDocument& Document, XMLNode* Root)
{
	XMLNode* RenderState = Document.NewElement("RenderState");

	XMLElement* CameraPosition = Document.NewElement("CameraPosition");
	CameraPosition->SetAttribute("X", this->CameraPosition.x);
	CameraPosition->SetAttribute("Y", this->CameraPosition.y);
	CameraPosition->SetAttribute("Z", this->CameraPosition.z);
	RenderState->InsertEndChild(CameraPosition);

	XMLElement* CameraAngle = Document.NewElement("CameraAngle");
	CameraAngle->SetAttribute("X", degrees(this->CameraAngleX));
	CameraAngle->SetAttribute("Z", degrees(this->CameraAngleZ));
	RenderState->InsertEndChild(CameraAngle);

	Root->InsertEndChild(RenderState);
}

bool RenderSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = {};

	XMLNode* RenderState = Root->FirstChildElement("RenderState");
	if (RenderState == nullptr)
		return false;

	XMLElement* CameraPosition = RenderState->FirstChildElement("CameraPosition");
	if (CameraPosition != nullptr) {
		this->CameraPosition.x = attribute_float_value(CameraPosition, "X");
		this->CameraPosition.y = attribute_float_value(CameraPosition, "Y");
		this->CameraPosition.z = attribute_float_value(CameraPosition, "Z");
	}

	XMLElement* CameraAngle = RenderState->FirstChildElement("CameraAngle");
	if (CameraAngle != nullptr) {
		this->CameraAngleX = radians(attribute_float_value(CameraAngle, "X"));
		this->CameraAngleZ = radians(attribute_float_value(CameraAngle, "Z"));
	}

	return true;
}

// SerializeSerializedState

void SerializeSerializedState::SaveToXML(XMLDocument& Document, XMLNode* Root)
{
	XMLNode* SerializeState = Document.NewElement("SerializeState");

	XMLElement* Animation = Document.NewElement("Animation");
	Animation->SetAttribute("Position", this->AnimationPosition);
	Animation->SetAttribute("Length", this->AnimationLength);
	Animation->SetAttribute("KinematicMode", this->KinematicModeFlag);
	Animation->SetAttribute("Loop", this->LoopAnimationFlag);
	Animation->SetAttribute("Play", this->PlayAnimaionFlag);
	Animation->SetAttribute("Speed", this->PlaySpeed);
	SerializeState->InsertEndChild(Animation);

	Root->InsertEndChild(SerializeState);
}

bool SerializeSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = {};

	XMLNode* SerializeState = Root->FirstChildElement("SerializeState");
	if (SerializeState == nullptr)
		return false;

	XMLElement* Animation = SerializeState->FirstChildElement("Animation");
	if (Animation != nullptr) {
		this->AnimationPosition = attribute_float_value(Animation, "Position");
		this->AnimationLength = attribute_float_value(Animation, "Length");
		this->KinematicModeFlag = attribute_bool_value(Animation, "KinematicMode", false);
		this->LoopAnimationFlag = attribute_bool_value(Animation, "Loop", false);
		this->PlayAnimaionFlag = attribute_bool_value(Animation, "Play", false);
		this->PlaySpeed = attribute_float_value(Animation, "Speed", 1);
	}

	return true;
}
