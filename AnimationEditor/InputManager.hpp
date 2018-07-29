#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glm/glm.hpp>

#include "Character.hpp"
#include "SerializationManager.hpp"

using namespace glm;

typedef enum InputState {
	None,
	InverseKinematic,
	InverseKinematicAutomate,
	BoneRotation
} InputState;

typedef struct InputSelection {
private:
	vec3 WorldPoint;
public:
	Bone* Bone;
	vec3 LocalPoint;

	vec3 GetWorldPoint(void);
	void SetWorldPoint(vec3 WorldPoint);

	bool HaveBone(void);
} InputSelection;

typedef class InputManager {
private:
	InputManager(void) { };

	const int StateSwapMouseTimeout = 50;

	HWND WindowHandle;

	BYTE LastKeyboardState[256], CurrentKeyboardState[256];
	LONG MouseX, MouseY;
	bool IsInFocus, IsMouseLockEnforced, IsCameraMode, WasMoving;
	int SkipMouseFormEventCount;
	ULONGLONG MouseUnlockTime;

	InputState State;

	typedef enum PlaneMode {
		PlaneCamera,
		PlaneX,
		PlaneY,
		PlaneZ
	} PlaneMode;

	PlaneMode PlaneMode;

	InputSelection Selection;

	void SetState(InputState NewState);

	bool IsInteractionMode(void);
	void SelectBoneAtScreenPoint(LONG x, LONG y);
	void CancelSelection(void);
	void ProcessCameraMovement(double dt);

	void SetWorldPointToScreePoint(LONG x, LONG y);
	void SetCursorToWorldPoint(vec3 WorldPoint);

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
	
	void Initialize(void);
	void SetWindow(HWND WindowHandle);

	void Tick(double dt);
	void SetFocus(bool IsInFocusNow);

	static bool IsStateInverseKinematic(InputState State);
	InputState GetState(void);
	InputSelection GetSelection(void);
	vec3 GetPlaneNormal(void);
	void ChangeBoneAngles(Bone* Bone, vec3 Angles);
	void SetupInverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 DestWorldPoint, bool IsAutomatic);

	void ProcessMouseInput(LONG dx, LONG dy);	
	void ProcessMouseFormEvent(LONG x, LONG y);
	void ProcessKeyboardEvent(void);
	void ProcessMouseWheelEvent(float Delta);

	void Serialize(InputSerializedState& State);
	void Deserialize(InputSerializedState& State);
} InputManager;