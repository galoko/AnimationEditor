#include "SerializationManager.hpp"

#include <locale>
#include <codecvt>

#include <tixml2ex.h>

#include "CharacterManager.hpp"
#include "InputManager.hpp"
#include "AnimationManager.hpp"
#include "Render.hpp"

void SerializationManager::Initialize(void)
{
	NextStateHistoryID = 1;

	LoadSettings();
}

void SerializationManager::CreateNewHistories(void)
{
	Histories.clear();

	CharacterManager::GetInstance().GetCharacter()->Reset();

	SerializedStateHistory History = {};

	History.ID = NextStateHistoryID++;

	CharacterManager::GetInstance().Serialize(History.CurrentState.CharState);
	History.CurrentState.HaveCharState = true;
	History.CurrentState.HaveInputState = true;
	History.CurrentState.HaveAnimationState = true;

	Histories.push_back(History);

	Deserialize();
}

void SerializationManager::Serialize(void)
{
	SingleSerializedState& State = GetCurrentHistory()->CurrentState;

	if (State.HaveCharState)
		CharacterManager::GetInstance().Serialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Serialize(State.InputState);

	if (State.HaveAnimationState)
		AnimationManager::GetInstance().Serialize(State.AnimationState);

	if (State.HaveRenderState)
		Render::GetInstance().Serialize(State.RenderState);
}

void SerializationManager::Deserialize(void)
{
	PendingLastTime = GetTickCount64();

	SingleSerializedState& State = GetCurrentHistory()->CurrentState;

	if (State.HaveRenderState)
		Render::GetInstance().Deserialize(State.RenderState);

	if (State.HaveCharState)
		CharacterManager::GetInstance().Deserialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Deserialize(State.InputState);

	if (State.HaveAnimationState)
		AnimationManager::GetInstance().Deserialize(State.AnimationState);
}

void SerializationManager::Autosave(void)
{
	SaveToFile(LastFileName);
}

void SerializationManager::Tick(double dt)
{
	if (IsFileOpen()) {
		ULONGLONG Now = GetTickCount64();
		if (LastAutosaveTime + AutosaveInterval < Now)
			Autosave();
	}
}

void SerializationManager::InternalPushStateFrame(bool Forward)
{
	SingleSerializedState& State = GetCurrentHistory()->CurrentState;
	State.HaveCharState = true;
	State.HaveInputState = true;
	State.HaveAnimationState = true;
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

bool SerializationManager::LoadHistoryByID(int32 ID)
{
	auto CurrentHistory = GetCurrentHistory();

	if (CurrentHistory->ID == ID)
		return true;

	auto NextHistory = GetHistoryByID(ID);
	if (NextHistory == Histories.end())
		return false;

	iter_swap(CurrentHistory, NextHistory);

	GetCurrentHistory()->CurrentState.HaveRenderState = false;
	Deserialize();

	return true;
}

int32 SerializationManager::GetCurrentHistoryID(void)
{
	return GetCurrentHistory()->ID;
}

bool SerializationManager::IsFileOpen(void)
{
	return LastFileName != L"";
}

vector<SerializedStateHistory>::iterator SerializationManager::GetCurrentHistory(void)
{
	return Histories.end() - 1;
}

vector<SerializedStateHistory>::iterator SerializationManager::GetHistoryByID(int32 ID)
{
	for (std::vector<SerializedStateHistory>::iterator History = Histories.begin(); History != Histories.end(); ++History) {
		if (History->ID == ID)
			return History;
	}

	return Histories.end();
}

void SerializationManager::SafeSaveDocumentToFile(XMLDocument& Document, const wstring FileName, int BackupCount)
{
	wstring FileNameNoExt = FileName.substr(0, FileName.find_last_of(L"."));

	wstring TempFileName = FileNameNoExt + L".tmp";
	wstring XmlFileName = FileNameNoExt + L".xml";
	wstring BackupFileName = FileNameNoExt + L".backup";

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

void SerializationManager::PushStateFrame(const wstring Sender)
{
	printf("PUSH STATE, SENDER: %ws\n", Sender.c_str());

	GetCurrentHistory()->FutureStates.clear();

	InternalPushStateFrame(false);
}

void SerializationManager::PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender)
{
	assert(PendingID != PendingNone);

	ULONGLONG Now = GetTickCount64();

	if (GetCurrentHistory()->CurrentState.PendingID != PendingID || PendingLastTime + PendingTimeout <= Now)
		PushStateFrame(L"Pending (" + Sender + L")");

	GetCurrentHistory()->CurrentState.PendingID = PendingID;
	PendingLastTime = Now;
}

void SerializationManager::PopAndDeserializeStateFrame(bool Forward)
{
	vector<SingleSerializedState>& StackToUse = Forward ? GetCurrentHistory()->FutureStates : GetCurrentHistory()->PreviousStates;

	if (!StackToUse.empty()) {

		InternalPushStateFrame(!Forward);

		GetCurrentHistory()->CurrentState = StackToUse.back();
		StackToUse.pop_back();

		Deserialize();
	}
}

vector<TimelineItem> SerializationManager::GetTimelineItems(void)
{
	vector<TimelineItem> Result;

	for (SerializedStateHistory& History : Histories) {
		TimelineItem Item = { History.ID, History.CurrentState.CharState.AnimationTimestamp / 1000.0f };
		Result.push_back(Item);
	}

	return Result;
}

void SerializationManager::SetTimelineItems(vector<TimelineItem> Items)
{
	int32 CurrentID = GetCurrentHistory()->ID;

	for (TimelineItem& Item : Items) {

		uint32 Timestamp = (uint32)(Item.Position * 1000.0f);

		if (Item.ID == CurrentID)
			CharacterManager::GetInstance().AnimationTimestamp = Timestamp;

		auto History = GetHistoryByID(Item.ID);
		if (History != Histories.end())
			History->CurrentState.CharState.AnimationTimestamp = Timestamp;
	}
}

// Utils

wstring s2ws(const string& str)
{
	using convert_typeX = codecvt_utf8<wchar_t>;
	wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string ws2s(const wstring& wstr)
{
	using convert_typeX = codecvt_utf8<wchar_t>;
	wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

float attribute_float_value(XMLElement* Element, const char* AttributeName) {

	float Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryFloatValue(&Result) != XML_SUCCESS)
		Result = 0.0f;

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

void SerializationManager::LoadFromFile(const wstring FileName)
{
	FILE* File = _wfopen(FileName.c_str(), L"rb");
	if (File == nullptr)
		return;

	XMLDocument Document;
	Document.LoadFile(File);
	fclose(File);

	float Position = 0.0f;
	bool KinematicModeFlag = false;

	Histories.clear();

	XMLNode* Root = Document.FirstChildElement("Root");
	if (Root != nullptr) {		

		for (auto StatesElement : selection(Root->ToElement(), "States")) {

			SerializedStateHistory States;

			LoadStates(States, Document, StatesElement);

			Histories.push_back(States);
		}

		if (!Histories.empty())
			Deserialize();

		XMLElement* AnimationElement = Root->FirstChildElement("Animation");
		if (AnimationElement != nullptr) {

			Position = attribute_float_value(AnimationElement, "Position");
			KinematicModeFlag = attribute_bool_value(AnimationElement, "KinematicMode", false);
		}
	}

	AnimationManager::GetInstance().SetAnimationState(Position, KinematicModeFlag ? 0 : GetCurrentHistoryID());

	if (LastFileName != FileName) {

		LastFileName = FileName;

		SaveSettings();
	}

	LastAutosaveTime = GetTickCount64();
}

void SerializationManager::SaveToFile(const wstring FileName)
{
	SingleSerializedState& State = GetCurrentHistory()->CurrentState;
	State.HaveCharState = true;
	State.HaveInputState = true;
	State.HaveAnimationState = true;
	State.HaveRenderState = true;
	Serialize();

	XMLDocument Document;

	XMLNode* Root = Document.NewElement("Root");
	Document.InsertFirstChild(Root);

	for (SerializedStateHistory States : Histories) {

		XMLElement* StatesElement = Document.NewElement("States");
		Root->InsertEndChild(StatesElement);

		SaveStates(States, Document, StatesElement);
	}

	XMLElement* AnimationState = Document.NewElement("Animation");
	Root->InsertEndChild(AnimationState);
	AnimationState->SetAttribute("Position", AnimationManager::GetInstance().GetAnimationPosition());
	AnimationState->SetAttribute("KinematicMode", AnimationManager::GetInstance().IsInKinematicMode());

	SafeSaveDocumentToFile(Document, FileName, MaxBackupCount);

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

	if (IsFileOpen())
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
	State.HaveAnimationState = false;
	State.HaveRenderState = false;

	XMLElement* Pending = Root->FirstChildElement("Pending");
	if (Pending != nullptr)
		State.PendingID = (SerializationPendingID)attribute_uint32_value(Pending, "ID");

	if (State.CharState.LoadFromXML(Document, Root))
		State.HaveCharState = true;

	if (State.InputState.LoadFromXML(Document, Root))
		State.HaveInputState = true;

	if (State.AnimationState.LoadFromXML(Document, Root))
		State.HaveAnimationState = true;

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
	if (State.HaveAnimationState)
		State.AnimationState.SaveToXML(Document, Root);
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

	States.ID = NextStateHistoryID++;
}

void SerializationManager::SaveStates(SerializedStateHistory& States, XMLDocument& Document, XMLNode* Root)
{
	for (SingleSerializedState State : States.PreviousStates) {

		XMLNode* PreviousState = Document.NewElement("PreviousState");
		Root->InsertEndChild(PreviousState);

		SaveState(State, Document, PreviousState);
	}

	XMLNode* CurrentState = Document.NewElement("CurrentState");
	Root->InsertEndChild(CurrentState);

	SaveState(States.CurrentState, Document, CurrentState);

	for (SingleSerializedState State : States.FutureStates) {

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

	for (SerializedBone Bone : Bones) {

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

// AnimationSerializedState

void AnimationSerializedState::SaveToXML(XMLDocument& Document, XMLNode* Root)
{
	XMLNode* AnimationState = Document.NewElement("AnimationState");

	for (SerializedAnimationContext Context : Contexts) {

		XMLElement* ContextElement = Document.NewElement("Context");
		ContextElement->SetAttribute("Name", ws2s(Context.BoneName).c_str());
		AnimationState->InsertEndChild(ContextElement);

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

	Root->InsertEndChild(AnimationState);
}

bool AnimationSerializedState::LoadFromXML(XMLDocument& Document, XMLNode* Root)
{
	*this = { };

	XMLNode* AnimationState = Root->FirstChildElement("AnimationState");
	if (AnimationState == nullptr)
		return false;

	for (auto BoneElement : selection(AnimationState->ToElement(), "Context")) {

		SerializedAnimationContext SerializedContext = {};

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