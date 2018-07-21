#include "InputManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "Character.hpp"
#include "Render.hpp"
#include "PhysicsManager.hpp"

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
}

void InputManager::SetFocus(bool IsInFocusNow) {

	if (IsInFocusNow != IsInFocus) {

		IsInFocus = IsInFocusNow;

		ProcessMouseLockState();
	}
}

bool InputManager::GetPickedPoint(vec3& PickedPoint, vec3& PlaneNormal, float& PlaneDistance)
{
	if (HavePickedPoint) {

		PickedPoint = this->PickedPoint;
		PlaneNormal = this->PlaneNormal;
		PlaneDistance = this->PlaneDistance;

		return true;
	}
	else {
		return false;
	}
}

void InputManager::ProcessMouseLockState(void)
{
	if (State == CameraMode && IsInFocus) {

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

		if (HavePickedPoint) {

			LONG x, y;
			Render::GetInstance().GetScreenPointFromPoint(PickedPoint, x, y);

			POINT ScreenPoint = { x, y };
			ClientToScreen(WindowHandle, &ScreenPoint);

			SetCursorPos(ScreenPoint.x, ScreenPoint.y);
		}
	}
}

void InputManager::ProcessMouseInput(LONG dx, LONG dy) {

	if (State == CameraMode && IsInFocus) {

		const float Speed = 1.0f / 300;
		Render::GetInstance().RotateCamera((float)dx * Speed, (float)dy * Speed);

		PlaneNormal = -Render::GetInstance().GetLookingDirection();
		PlaneDistance = dot(PlaneNormal, PickedPoint);
		/*
		if (HavePickedPoint)
			CorrectPlaneDistance();
		*/
	}
}

void InputManager::CorrectPlaneDistance(void) {

	vec3 CameraPosition = Render::GetInstance().GetCameraPosition();

	float OldDistancePlaneToCamera = dot(CameraPosition, PlaneNormal) - PlaneDistance;

	PlaneNormal = -Render::GetInstance().GetLookingDirection();

	vec3 NewPlaneCenter = CameraPosition - PlaneNormal * OldDistancePlaneToCamera;

	PlaneDistance = dot(NewPlaneCenter, PlaneNormal);

	SyncPickedPointWithScreenCoord(MouseX, MouseY);
}

void InputManager::SyncPickedPointWithScreenCoord(LONG x, LONG y) {

	vec3 Point, Direction;
	Render::GetInstance().GetPointAndDirectionFromScreenPoint(x, y, Point, Direction);

	float c = dot(Direction, PlaneNormal);
	if (c < 0) {

		float t = -(dot(Point, PlaneNormal) - PlaneDistance) / c;
		vec3 P = Point + t * Direction;

		PickedPoint = P;
	}
}

void InputManager::ProcessKeyboardInput(double dt) {

	if (WasPressed(VK_RBUTTON)) {

		SavedCameraPosition = Render::GetInstance().GetCameraPosition();

		State = CameraMode;
		ProcessMouseLockState();
	}
	else
	if (WasUnpressed(VK_RBUTTON)) {

		State = InteractionMode;
		ProcessMouseLockState();
	}

	if (WasPressed(VK_LBUTTON) && State == InteractionMode) {

		Bone* SelectedBone;
		vec3 WorldPoint, WorldNormal;

		Render::GetInstance().GetBoneFromScreenPoint(MouseX, MouseY, SelectedBone, WorldPoint, WorldNormal);

		if (SelectedBone != nullptr) {
			PickedPoint = WorldPoint;
			HavePickedPoint = true;

			PlaneNormal = -Render::GetInstance().GetLookingDirection();
			PlaneDistance = dot(PlaneNormal, PickedPoint);
		}
		else {
			HavePickedPoint = false;
		}
	}

	if (State == CameraMode) {
		const float Speed = 10.0f;
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

		if (dot(Offset, Offset) > 0) {

			Render::GetInstance().MoveCamera(normalize(Offset) * Speed * (float)dt);
		}
	}

	memcpy(LastKeyboardState, CurrentKeyboardState, sizeof(LastKeyboardState));
}

void InputManager::ProcessMouseFormEvent(LONG x, LONG y) {

	MouseX = x;
	MouseY = y;
	
	if (HavePickedPoint && State == InteractionMode) {

		PlaneNormal = -Render::GetInstance().GetLookingDirection();

		SyncPickedPointWithScreenCoord(MouseX, MouseY);
	}
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