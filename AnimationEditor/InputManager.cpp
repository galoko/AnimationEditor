#include "InputManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Character.hpp"
#include "Render.hpp"
#include "PhysicsManager.hpp"

using namespace glm;

void InputManager::Initialize(HWND WindowHandle) {

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

void InputManager::ProcessMouseLockState(void)
{
	if (IsMouseLocked && IsInFocus) {

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
	}
}

void InputManager::ProcessMouseInput(LONG dx, LONG dy) {

	if (IsMouseLocked && IsInFocus) {
		const float Speed = 1.0f / 300;
		Render::GetInstance().RotateCamera((float)dx * Speed, (float)dy * Speed);
	}
}

void InputManager::ProcessKeyboardInput(double dt) {

	if (WasPressed(VK_RBUTTON)) {
		IsMouseLocked = true;
		ProcessMouseLockState();
	}
	else
	if (WasUnpressed(VK_RBUTTON)) {
		IsMouseLocked = false;
		ProcessMouseLockState();
	}

	if (WasPressed(VK_LBUTTON) && !IsMouseLocked) {

		Bone* SelectedBone;
		vec3 WorldPoint, WorldNormal;

		Render::GetInstance().GetBoneFromScreenPoint(MouseX, MouseY, SelectedBone, WorldPoint, WorldNormal);

		if (SelectedBone != nullptr) {
			printf("Selected: %ws\n", SelectedBone->Name.c_str());
		}
	}

	if (IsMouseLocked) {
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
	// ...?
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