#include "InputManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "Render.hpp"
#include "AnimationManager.hpp"
#include "CharacterManager.hpp"
#include "Form.hpp"

void InputManager::Initialize(void) {

}

void InputManager::SetWindow(HWND WindowHandle)
{
	this->WindowHandle = WindowHandle;

	RAWINPUTDEVICE Rid[1] = {};

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = WindowHandle;

	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])))
		throw new runtime_error("Couldn't register raw devices");
}

void InputManager::Tick(double dt) {

	ProcessKeyboardInput(dt);

	if (State == InverseKinematic && Selection.HaveBone())
		AnimationManager::GetInstance().InverseKinematic(Selection.Bone, Selection.LocalPoint, Selection.GetWorldPoint());
}

void InputManager::SetFocus(bool IsInFocusNow) {

	if (IsInFocusNow != IsInFocus) {

		IsInFocus = IsInFocusNow;

		ProcessMouseLockState();
	}
}

InputState InputManager::GetState(void)
{
	return State;
}

InputSelection InputManager::GetSelection(void)
{
	return Selection;
}

void InputManager::ProcessMouseLockState(void)
{
	if (IsCameraMode && IsInFocus) {

		POINT CursorPoint;
		if (!IsMouseLockEnforced && GetCursorPos(&CursorPoint)) {

			RECT Rect = { CursorPoint.x, CursorPoint.y, CursorPoint.x + 1, CursorPoint.y + 1 };
			ClipCursor(&Rect);

			ShowCursor(false);

			IsMouseLockEnforced = true;
		}
	}
	else if (IsMouseLockEnforced) {

		ShowCursor(true);
		ClipCursor(NULL);

		IsMouseLockEnforced = false;

		if (State == InverseKinematic && Selection.HaveBone()) 
			SetCursorToWorldPoint(Selection.GetWorldPoint());
	}
}

void InputManager::ProcessMouseInput(LONG dx, LONG dy) {

	if (IsCameraMode && IsInFocus) {

		const float Speed = 1.0f / 300;
		Render::GetInstance().RotateCamera((float)dx * Speed, (float)dy * Speed);
	}
}

void InputManager::SetState(InputState NewState)
{
	if (State == InverseKinematic && NewState != InverseKinematic)
		AnimationManager::GetInstance().CancelInverseKinematic();

	State = NewState;
}

bool InputManager::IsInteractionMode(void)
{
	return !IsCameraMode;
}

void InputManager::SelectBoneAtScreenPoint(LONG x, LONG y) {

	Form::UpdateLock Lock;

	if (Selection.HaveBone()) {

		CancelSelection();

		if (State != None) 
			return;
	}

	Bone* SelectedBone;
	vec3 WorldPoint, WorldNormal;

	Render::GetInstance().GetBoneFromScreenPoint(MouseX, MouseY, SelectedBone, WorldPoint, WorldNormal);

	if (SelectedBone != nullptr) {

		Selection.Bone = SelectedBone;
		Selection.LocalPoint = inverse(SelectedBone->WorldTransform * SelectedBone->MiddleTranslation) * vec4(WorldPoint, 1);
		Selection.SetWorldPoint(WorldPoint);

		Form::GetInstance().UpdateBlocking();
		Form::GetInstance().UpdatePositionAndAngles();
	}
}

void InputManager::CancelSelection(void) {

	Selection.Bone = nullptr;

	if (State == InverseKinematic)
		AnimationManager::GetInstance().CancelInverseKinematic();

	Form::GetInstance().UpdateBlocking();
	Form::GetInstance().UpdatePositionAndAngles();
}

void InputManager::ProcessCameraMovement(double dt)
{
	const float Speed = 1.0f;

	vec3 Offset = {};
	vec3 Direction = Render::GetInstance().GetLookingDirection();
	vec3 SideDirection = cross(Direction, Render::GetInstance().Up);
	vec3 ForwardDirection = cross(Render::GetInstance().Up, SideDirection);

	if (IsPressed('W'))
		Offset += ForwardDirection;
	if (IsPressed('S'))
		Offset -= ForwardDirection;
	if (IsPressed('D'))
		Offset += SideDirection;
	if (IsPressed('A'))
		Offset -= SideDirection;

	if (length(Offset) > 0) {

		Render::GetInstance().MoveCamera(normalize(Offset) * Speed * (float)dt);

		if (State == InverseKinematic && Selection.HaveBone())
			SetWorldPointToScreePoint(MouseX, MouseY);
	}
}

vec3 InputManager::GetPlaneNormal(void)
{	
	vec3 CameraNormal = -Render::GetInstance().GetLookingDirection();

	vec3 Result = { 0, 0, 0 };

	switch (PlaneMode) {
	case PlaneCamera:
		Result = CameraNormal;
		break;
	case PlaneX:
		Result = { 1, 0, 0 };
		break;
	case PlaneY:
		Result = { 0, 1, 0 };
		break;
	case PlaneZ:
		Result = { 0, 0, 1 };
		break;
	}

	float Direction = dot(CameraNormal, Result) > 0 ? 1.0f : -1.0f;

	return Result * Direction;
}

void InputManager::RecalcSelectedWorldPoint(void)
{
	if (Selection.HaveBone()) {

		Bone* Bone = Selection.Bone;

		vec3 WorldPoint = Bone->WorldTransform * Bone->MiddleTranslation * vec4(Selection.LocalPoint, 1);

		Selection.SetWorldPoint(WorldPoint);
	}
}

void InputManager::SetWorldPointToScreePoint(LONG x, LONG y) {

	vec3 Point, Direction;
	Render::GetInstance().GetPointAndDirectionFromScreenPoint(x, y, Point, Direction);

	vec3 PlaneNormal = GetPlaneNormal();

	float c = dot(Direction, PlaneNormal);
	if (fabs(c) > 10e-7) {

		float t = -(dot(Point, PlaneNormal) - dot(Selection.GetWorldPoint(), PlaneNormal)) / c;
		vec3 P = Point + t * Direction;

		Selection.SetWorldPoint(P);
	}
}

void InputManager::SetCursorToWorldPoint(vec3 WorldPoint)
{
	LONG x, y;
	Render::GetInstance().GetScreenPointFromPoint(Selection.GetWorldPoint(), x, y);

	POINT ScreenPoint = { x, y };
	ClientToScreen(WindowHandle, &ScreenPoint);

	SetCursorPos(ScreenPoint.x, ScreenPoint.y);
}

void InputManager::ProcessKeyboardInput(double dt) {

	if (WasPressed(VK_RBUTTON)) {

		IsCameraMode = true;
		ProcessMouseLockState();
	}
	else
	if (WasUnpressed(VK_RBUTTON)) {

		IsCameraMode = false;
		ProcessMouseLockState();
	}

	if (WasPressed(VK_LBUTTON) && IsInteractionMode()) 
		SelectBoneAtScreenPoint(MouseX, MouseY);

	if (WasPressed(VK_ESCAPE)) {

		if (State == None)
			CancelSelection();
		else
			SetState(None);
	}

	if (WasPressed('Q')) {
		if (State == None) {
			SetCursorToWorldPoint(Selection.GetWorldPoint());
			SetState(InverseKinematic);
		}
		else
		if (State == InverseKinematic) 
			SetState(None);		
	}

	if (WasPressed('R') && Selection.HaveBone()) {

		if (IsPressed(VK_LCONTROL))
			AnimationManager::GetInstance().BlockEverythingExceptThisBranch(Selection.Bone->Parent, Selection.Bone);
		else {

			BlockingInfo Blocking = AnimationManager::GetInstance().GetBoneBlocking(Selection.Bone);
			if (Blocking.IsFullyBlocked())
				Blocking = BlockingInfo::GetAllUnblocked();
			else
				Blocking = BlockingInfo::GetAllBlocked();

			AnimationManager::GetInstance().SetBoneBlocking(Selection.Bone, Blocking);
		}
	}

	if (WasPressed(VK_F5)) {
		CompleteSerializedState SavedState;
		SerializationManager::GetInstance().Serialize(SavedState);
		SerializationManager::GetInstance().SaveToFile(SavedState, L"H:\\test.xml");
	}

	if (WasPressed(VK_F9)) {
		CompleteSerializedState SavedState;
		SerializationManager::GetInstance().LoadFromFile(SavedState, L"H:\\test.xml");
		SerializationManager::GetInstance().Deserialize(SavedState);
	}

	if (WasPressed('P'))
		AnimationManager::GetInstance().UnblockAllBones();

	if (WasPressed('K') && Selection.HaveBone()) {

		BlockingInfo Blocking = AnimationManager::GetInstance().GetBoneBlocking(Selection.Bone);
		
		Blocking.XAxis = false;
		Blocking.YAxis = false;
		Blocking.ZAxis = false;

		AnimationManager::GetInstance().SetBoneBlocking(Selection.Bone, Blocking);
	}

	if (WasPressed('Z'))
		PlaneMode = PlaneZ;
	if (WasPressed('X'))
		PlaneMode = PlaneX;
	if (WasPressed('C'))
		PlaneMode = PlaneY;
	if (WasPressed('V'))
		PlaneMode = PlaneCamera;

	ProcessCameraMovement(dt);

	// erase previous state
	memcpy(LastKeyboardState, CurrentKeyboardState, sizeof(LastKeyboardState));
}

void InputManager::ProcessMouseFormEvent(LONG x, LONG y) {

	MouseX = x;
	MouseY = y;

	if (State == None && IsPressed(VK_LBUTTON))
		SelectBoneAtScreenPoint(MouseX, MouseY);
	
	if (State == InverseKinematic && Selection.HaveBone())
		SetWorldPointToScreePoint(MouseX, MouseY);
}

void InputManager::ProcessKeyboardEvent(void) {

	UpdateKeyboardState();
	ProcessKeyboardInput(0);
}

void InputManager::ProcessMouseWheelEvent(float Delta) {

	if (State == InverseKinematic && Selection.HaveBone()) {

		vec3 PlaneNormal = GetPlaneNormal();

		Selection.SetWorldPoint(Selection.GetWorldPoint() - PlaneNormal * 0.05f * Delta);
		
		SetCursorToWorldPoint(Selection.GetWorldPoint());
	}
}

void InputManager::Serialize(InputSerializedState& State)
{
	State.State = (uint32)this->State;
	State.PlaneMode = (uint32)this->PlaneMode;

	State.BoneName = Selection.HaveBone() ? Selection.Bone->Name : L"";
	State.LocalPoint = Selection.LocalPoint;
	State.WorldPoint = Selection.GetWorldPoint();
}

void InputManager::Deserialize(InputSerializedState& State)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	this->PlaneMode = (enum PlaneMode)State.PlaneMode;

	Selection.Bone = State.BoneName != L"" ? Char->FindBone(State.BoneName) : nullptr;
	Selection.LocalPoint = State.LocalPoint;
	Selection.SetWorldPoint(State.WorldPoint);

	if (Selection.HaveBone())
		SetState((InputState)State.State);
	else
		SetState(None);

	if (this->State != None && Selection.HaveBone())
		SetCursorToWorldPoint(Selection.GetWorldPoint());
}

// General API

#define IsKeyPressed(x, y) ((x[y] & 0x80) != 0)

bool InputManager::WasPressed(int Key)
{
	return !IsKeyPressed(LastKeyboardState, Key) && IsKeyPressed(CurrentKeyboardState, Key);
}

bool InputManager::IsPressed(int Key)
{
	return IsKeyPressed(CurrentKeyboardState, Key);
}

bool InputManager::WasUnpressed(int Key)
{
	return IsKeyPressed(LastKeyboardState, Key) && !IsKeyPressed(CurrentKeyboardState, Key);
}

void InputManager::UpdateKeyboardState(void)
{
	GetKeyState(0);
	GetKeyboardState(CurrentKeyboardState);
}

// InputSelection

vec3 InputSelection::GetWorldPoint(void)
{
	return WorldPoint;
}

void InputSelection::SetWorldPoint(vec3 WorldPoint)
{
	const float BoxSize = 3.0f;

	WorldPoint.x = max(-BoxSize, min(WorldPoint.x, BoxSize));
	WorldPoint.y = max(-BoxSize, min(WorldPoint.y, BoxSize));
	WorldPoint.z = max(-BoxSize, min(WorldPoint.z, BoxSize));

	this->WorldPoint = WorldPoint;
}

bool InputSelection::HaveBone(void)
{
	return Bone != nullptr;
}
