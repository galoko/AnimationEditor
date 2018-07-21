#pragma once

#include <windows.h>

#include <glm/glm.hpp>

using namespace glm;

typedef class InputManager {
private:
	InputManager(void) { };

	BYTE LastKeyboardState[256], CurrentKeyboardState[256];
	LONG MouseX, MouseY;
	bool IsInFocus;

	typedef enum State {
		InteractionMode,
		CameraMode
	} State;

	State State;
	bool IsMouseLockEnforced;

	bool HavePickedPoint;
	vec3 PickedPoint, PlaneNormal;
	float PlaneDistance;

	vec3 SavedCameraPosition;

	void CorrectPlaneDistance(void);
	void SyncPickedPointWithScreenCoord(LONG x, LONG y);

	// Internal processing
	void ProcessMouseLockState(void);
	void ProcessKeyboardInput(double dt);

	// General API
	bool WasPressed(int Key);
	bool IsPressed(int Key);
	bool WasUnpressed(int Key);
	void UpdateKeyboardState(void);
public:
	static InputManager& GetInstance(void) {
		static InputManager Instance;

		return Instance;
	}

	InputManager(InputManager const&) = delete;
	void operator=(InputManager const&) = delete;
	
	void Initialize(HWND WindowHandle);

	void Tick(double dt);
	void SetFocus(bool IsInFocusNow);

	bool GetPickedPoint(vec3& PickedPoint, vec3& PlaneNormal, float& PlaneDistance);

	void ProcessMouseInput(LONG dx, LONG dy);	
	void ProcessMouseFormEvent(LONG x, LONG y);
	void ProcessKeyboardEvent(void);
	void ProcessMouseWheelEvent(float Delta);
} InputManager;