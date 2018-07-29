#include "SerializationManager.hpp"

#include <locale>
#include <codecvt>

#include <tixml2ex.h>

#include "CharacterManager.hpp"
#include "InputManager.hpp"
#include "AnimationManager.hpp"
#include "Render.hpp"

void SerializationManager::Serialize(CompleteSerializedState& State)
{
	State.PendingID = CurrentPendingID;

	if (State.HaveCharState)
		CharacterManager::GetInstance().Serialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Serialize(State.InputState);

	if (State.HaveAnimationState)
		AnimationManager::GetInstance().Serialize(State.AnimationState);

	if (State.HaveRenderState)
		Render::GetInstance().Serialize(State.RenderState);
}

void SerializationManager::Deserialize(CompleteSerializedState& State)
{
	CurrentPendingID = State.PendingID;
	PendingLastTime = GetTickCount64();

	if (State.HaveRenderState)
		Render::GetInstance().Deserialize(State.RenderState);

	if (State.HaveCharState)
		CharacterManager::GetInstance().Deserialize(State.CharState);

	if (State.HaveInputState)
		InputManager::GetInstance().Deserialize(State.InputState);

	if (State.HaveAnimationState)
		AnimationManager::GetInstance().Deserialize(State.AnimationState);
}

void SerializationManager::LoadFromFile(const wstring FileName)
{
	FILE* File = _wfopen(FileName.c_str(), L"rb");
	if (File == nullptr)
		return;

	XMLDocument Document;

	Document.LoadFile(File);

	XMLNode* Root = Document.FirstChildElement("Root");
	if (Root != nullptr) {

		OtherStates.clear();

		for (auto StatesElement : selection(Root->ToElement(), "States")) {

			CompleteSerializedStates States;

			LoadStates(States, Document, StatesElement);

			OtherStates.push_back(States);
		}

		if (!OtherStates.empty()) {

			CompleteSerializedStates ActiveStates = OtherStates.back();
			OtherStates.pop_back();

			Deserialize(ActiveStates.CurrentState);
			StateFrames = ActiveStates.PreviousStates;
			ForwardStateFrames = ActiveStates.FutureStates;
		}
	}

	fclose(File);
}

void SerializationManager::SaveToFile(const wstring FileName)
{
	XMLDocument Document;

	XMLNode* Root = Document.NewElement("Root");
	Document.InsertFirstChild(Root);

	for (CompleteSerializedStates States : OtherStates) {

		XMLElement* StatesElement = Document.NewElement("States");
		Root->InsertEndChild(StatesElement);

		SaveStates(States, Document, StatesElement);
	}

	CompleteSerializedStates ActiveStates;

	ActiveStates.PreviousStates = StateFrames;
	ActiveStates.FutureStates = ForwardStateFrames;

	ActiveStates.CurrentState.HaveCharState = true;
	ActiveStates.CurrentState.HaveInputState = true;
	ActiveStates.CurrentState.HaveAnimationState = true;
	ActiveStates.CurrentState.HaveRenderState = true;
	Serialize(ActiveStates.CurrentState);

	XMLElement* StatesElement = Document.NewElement("States");
	Root->InsertEndChild(StatesElement);
	SaveStates(ActiveStates, Document, StatesElement);

	FILE* File = _wfopen(FileName.c_str(), L"wb");
	Document.SaveFile(File, false);
	fclose(File);
}

void SerializationManager::Tick(double dt)
{

}

void SerializationManager::InternalPushStateFrame(bool Forward)
{
	CompleteSerializedState State;

	State.HaveCharState = true;
	State.HaveInputState = true;
	State.HaveAnimationState = true;
	State.HaveRenderState = false;

	Serialize(State);

	if (Forward)
		ForwardStateFrames.push_back(State);
	else
		StateFrames.push_back(State);

	CurrentPendingID = PendingNone;
	PendingLastTime = 0;
}

void SerializationManager::PushStateFrame(const wstring Sender)
{
	printf("PUSH STATE, SENDER: %ws\n", Sender.c_str());

	ForwardStateFrames.clear();

	InternalPushStateFrame(false);
}

void SerializationManager::PushPendingStateFrame(SerializationPendingID PendingID, const wstring Sender)
{
	assert(PendingID != PendingNone);

	ULONGLONG Now = GetTickCount64();

	if (CurrentPendingID != PendingID || PendingLastTime + PendingTimeout <= Now)
		PushStateFrame(L"Pending (" + Sender + L")");

	CurrentPendingID = PendingID;
	PendingLastTime = Now;
}

void SerializationManager::PopAndDeserializeStateFrame(bool Forward)
{
	vector<CompleteSerializedState>* StackToUse = Forward ? &ForwardStateFrames : &StateFrames;

	if (!StackToUse->empty()) {

		InternalPushStateFrame(!Forward);

		CompleteSerializedState State = StackToUse->back();
		StackToUse->pop_back();

		Deserialize(State);
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

bool attribute_bool_value(XMLElement* Element, const char* AttributeName) {

	bool Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryBoolValue(&Result) != XML_SUCCESS)
		Result = true;

	return Result;
}

uint32 attribute_uint32_value(XMLElement* Element, const char* AttributeName) {

	uint32 Result;

	const XMLAttribute* Attribute = Element->FindAttribute(AttributeName);
	if (Attribute == nullptr || Attribute->QueryUnsignedValue(&Result) != XML_SUCCESS)
		Result = true;

	return Result;
}

void SerializationManager::LoadState(CompleteSerializedState& State, XMLDocument& Document, XMLNode* Root)
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

void SerializationManager::SaveState(CompleteSerializedState& State, XMLDocument& Document, XMLNode* Root)
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

void SerializationManager::LoadStates(CompleteSerializedStates& States, XMLDocument& Document, XMLNode* Root)
{
	XMLElement* CurrentStateElement = Root->FirstChildElement("CurrentState");
	if (CurrentStateElement != nullptr) {

		LoadState(States.CurrentState, Document, CurrentStateElement);

		States.PreviousStates.clear();
		for (auto PreviousStateElement : selection(Root->ToElement(), "PreviousState")) {

			CompleteSerializedState PreviousState;

			LoadState(PreviousState, Document, PreviousStateElement);

			States.PreviousStates.push_back(PreviousState);
		}

		States.FutureStates.clear();
		for (auto FutureStateElement : selection(Root->ToElement(), "FutureState")) {

			CompleteSerializedState FutureState;

			LoadState(FutureState, Document, FutureStateElement);

			States.FutureStates.push_back(FutureState);
		}
	}
}

void SerializationManager::SaveStates(CompleteSerializedStates& States, XMLDocument& Document, XMLNode* Root)
{
	for (CompleteSerializedState State : States.PreviousStates) {

		XMLNode* PreviousState = Document.NewElement("PreviousState");
		Root->InsertEndChild(PreviousState);

		SaveState(State, Document, PreviousState);
	}

	XMLNode* CurrentState = Document.NewElement("CurrentState");
	Root->InsertEndChild(CurrentState);

	SaveState(States.CurrentState, Document, CurrentState);

	for (CompleteSerializedState State : States.FutureStates) {

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