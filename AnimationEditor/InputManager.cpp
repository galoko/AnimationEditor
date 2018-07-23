#include "InputManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "Render.hpp"
#include "AnimationManager.hpp"
#include "CharacterManager.hpp"

void InputManager::Initialize(HWND WindowHandle) {

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
		AnimationManager::GetInstance().InverseKinematic(Selection.Bone, Selection.LocalPoint, Selection.WorldPoint);
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

		if (State == InverseKinematic && Selection.HaveBone()) {

			LONG x, y;
			Render::GetInstance().GetScreenPointFromPoint(Selection.WorldPoint, x, y);

			POINT ScreenPoint = { x, y };
			ClientToScreen(WindowHandle, &ScreenPoint);

			SetCursorPos(ScreenPoint.x, ScreenPoint.y);
		}
	}
}

void InputManager::ProcessMouseInput(LONG dx, LONG dy) {

	if (IsCameraMode && IsInFocus) {

		const float Speed = 1.0f / 300;
		Render::GetInstance().RotateCamera((float)dx * Speed, (float)dy * Speed);
	}
}

bool InputManager::IsInteractionMode(void)
{
	return !IsCameraMode;
}

void InputManager::SelectBoneAtScreenPoint(LONG x, LONG y) {

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
		Selection.LocalPoint = inverse(SelectedBone->WorldTransform) * vec4(WorldPoint, 1);
		Selection.WorldPoint = WorldPoint;
	}
}

void InputManager::CancelSelection(void) {

	Selection.Bone = nullptr;

	if (State == InverseKinematic)
		AnimationManager::GetInstance().CancelInverseKinematic();
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

	if (length(Offset) > 0)
		Render::GetInstance().MoveCamera(normalize(Offset) * Speed * (float)dt);
}

vec3 InputManager::GetPlaneNormal(void)
{	
	switch (PlaneMode) {
	case PlaneCamera:
		return -Render::GetInstance().GetLookingDirection();
	case PlaneX:
		return { 1, 0, 0 };
	case PlaneY:
		return { 0, 1, 0 };
	case PlaneZ:
		return { 0, 0, 1 };
	}
}

void InputManager::SetWorldPointToScreePoint(LONG x, LONG y) {

	vec3 Point, Direction;
	Render::GetInstance().GetPointAndDirectionFromScreenPoint(x, y, Point, Direction);

	vec3 PlaneNormal = GetPlaneNormal();

	float c = dot(Direction, PlaneNormal);
	if (fabs(c) > 10e-7) {

		float t = -(dot(Point, PlaneNormal) - dot(Selection.WorldPoint, PlaneNormal)) / c;
		vec3 P = Point + t * Direction;

		Selection.WorldPoint = P;
	}
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
		CancelSelection();
		State = None;
	}

	if (WasPressed('Q') && State == None)
		State = InverseKinematic;

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

bool InputSelection::HaveBone(void)
{
	return Bone != nullptr;
}
