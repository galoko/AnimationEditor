#include "Form.hpp"

#include <WindowsX.h>

#include <iomanip>
#include <sstream>

#include "Render.hpp"
#include "InputManager.hpp"
#include "AnimationManager.hpp"
#include "ExternalGUI.hpp"
#include "shader.hpp"

#define CheckFormUpdateBlock(PendingFlag) \
	if (UpdateBlockCounter != 0) { \
		PendingFlag = true; \
		return; \
	}

HWND Form::Initialize(HINSTANCE hInstance) {

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProcStaticCallback;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = WindowClass;

	RegisterClassExW(&wcex);

	int Width = Render::GetInstance().Width;
	int Height = Render::GetInstance().Height;

	POINT WindowPos;
	WindowPos.x = (GetSystemMetrics(SM_CXSCREEN) - Width) / 2;
	WindowPos.y = (GetSystemMetrics(SM_CYSCREEN) - Height) / 2;

	WindowHandle = CreateWindowW(WindowClass, Title, WS_POPUP, WindowPos.x, WindowPos.y, Width, Height, nullptr, nullptr, hInstance, nullptr);
	if (WindowHandle == 0)
		throw new runtime_error("Couldn't create window");

	aegSetOpenGLWindow(WindowHandle);
	aegSetButtonCallback(ButtonStaticCallback);
	aegSetCheckBoxCallback(CheckBoxStaticCallback);
	aegSetEditCallback(EditStaticCallback);
	aegSetTrackBarCallback(TrackBarStaticCallback);

	UpdateBlocking();
	UpdatePositionAndAngles();

	return WindowHandle;
}

void Form::Tick(double dt) {

	bool IsInFocusNow = WindowHandle == GetForegroundWindow();
	InputManager::GetInstance().SetFocus(IsInFocusNow);

	InputManager::GetInstance().Tick(dt);

	AnimationManager::GetInstance().Tick(dt);

	// redraw
	RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

LRESULT CALLBACK Form::WndProcStaticCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	return Form::GetInstance().WndProcCallback(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Form::WndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_SIZE: {

		int Width = LOWORD(lParam);
		int Height = HIWORD(lParam);

		glViewport(0, 0, Width, Height);

		RedrawWindow(WindowHandle, NULL, 0, RDW_INVALIDATE | RDW_UPDATENOW);

		break;
	}
	case WM_PAINT: {

		Render::GetInstance().DrawScene();

		static PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);

		break;
	}
	// controls
	case WM_INPUT: {

		UINT RimType;
		HRAWINPUT RawHandle;
		RAWINPUT RawInput;
		UINT RawInputSize, Res;

		RimType = GET_RAWINPUT_CODE_WPARAM(wParam);
		RawHandle = HRAWINPUT(lParam);

		RawInputSize = sizeof(RawInput);
		Res = GetRawInputData(RawHandle, RID_INPUT, &RawInput, &RawInputSize, sizeof(RawInput.header));

		if (Res != (sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE)))
			break;

		if ((RawInput.data.mouse.usFlags & 1) != MOUSE_MOVE_RELATIVE)
			break;

		InputManager::GetInstance().ProcessMouseInput(RawInput.data.mouse.lLastX, RawInput.data.mouse.lLastY);

		break;
	}
	case WM_MOUSEMOVE:

		InputManager::GetInstance().ProcessMouseFormEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_KEYDOWN: 
	case WM_KEYUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:

		InputManager::GetInstance().ProcessKeyboardEvent();
		break;

	case WM_MOUSEWHEEL: {

		int WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		InputManager::GetInstance().ProcessMouseWheelEvent((float)WheelDelta / WHEEL_DELTA);
		break;
	}
	case WM_DESTROY:

		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void Form::ButtonStaticCallback(const wchar_t * Name)
{
	Form::GetInstance().ButtonCallback(Name);
}

void Form::ButtonCallback(const wstring Name)
{
	if (Name == CONSTRAIN_POSITION) {

		InputSelection Selection = InputManager::GetInstance().GetSelection();

		if (Selection.HaveBone())
			if (!AnimationManager::GetInstance().IsBonePositionConstrained(Selection.Bone))
				AnimationManager::GetInstance().ConstrainBonePosition(Selection.Bone, Selection.GetWorldPoint());
			else
				AnimationManager::GetInstance().RemoveBonePositionConstraint(Selection.Bone);
	}
}

void Form::CheckBoxStaticCallback(const wchar_t* Name, bool IsChecked)
{
	Form::GetInstance().CheckBoxCallback(Name, IsChecked);
}

void Form::CheckBoxCallback(const wstring Name, bool IsChecked)
{
	Bone* Bone = InputManager::GetInstance().GetSelection().Bone;
	BlockingInfo Blocking = AnimationManager::GetInstance().GetBoneBlocking(Bone);

	if (Name == X_POS)
		Blocking.XPos = IsChecked;
	else
	if (Name == Y_POS)
		Blocking.YPos = IsChecked;
	else
	if (Name == Z_POS)
		Blocking.ZPos = IsChecked;
	else
	if (Name == X_AXIS)
		Blocking.XAxis = IsChecked;
	else
	if (Name == Y_AXIS)
		Blocking.YAxis = IsChecked;
	else
	if (Name == Z_AXIS)
		Blocking.ZAxis = IsChecked;

	if (Bone != nullptr)
		AnimationManager::GetInstance().SetBoneBlocking(Bone, Blocking);
}

void Form::EditStaticCallback(const wchar_t* Name, const wchar_t* Text)
{
	Form::GetInstance().EditCallback(Name, Text);
}

void Form::EditCallback(const wstring Name, const wstring Text)
{
	float Value = stof(Text, nullptr);

	if (Name == X_ANGLE_INPUT || Name == Y_ANGLE_INPUT || Name == Z_ANGLE_INPUT) {

		float Angle = radians(Value);

		Bone* Bone = InputManager::GetInstance().GetSelection().Bone;
		if (Bone == nullptr)
			return;

		vec3 Angles = PhysicsManager::GetInstance().GetBoneAngles(Bone);

		if (Name == X_ANGLE_INPUT)
			Angles.x = Angle;
		else
		if (Name == Y_ANGLE_INPUT)
			Angles.y = Angle;
		else
		if (Name == Z_ANGLE_INPUT)
			Angles.z = Angle;

		PhysicsManager::GetInstance().SetBoneAngles(Bone, Angles);

		InputManager::GetInstance().RecalcSelectedWorldPoint();
	}
}

void Form::TrackBarStaticCallback(const wchar_t* Name, float t)
{
	Form::GetInstance().TrackBarCallback(Name, t);
}

void Form::TrackBarCallback(const wstring Name, float t)
{
	if (Name == X_AXIS_BAR || Name == Y_AXIS_BAR || Name == Z_AXIS_BAR) {

		Bone* Bone = InputManager::GetInstance().GetSelection().Bone;
		if (Bone == nullptr)
			return;

		vec3 LowLimit = Bone->LowLimit;
		vec3 HighLimit = Bone->HighLimit;

		vec3 Angles = PhysicsManager::GetInstance().GetBoneAngles(Bone);

		if (Name == X_AXIS_BAR)
			Angles.x = LowLimit.x + (HighLimit.x - LowLimit.x) * t;
		else
		if (Name == Y_AXIS_BAR)
			Angles.y = LowLimit.y + (HighLimit.y - LowLimit.y) * t;
		else
		if (Name == Z_AXIS_BAR)
			Angles.z = LowLimit.z + (HighLimit.z - LowLimit.z) * t;

		PhysicsManager::GetInstance().SetBoneAngles(Bone, Angles);

		InputManager::GetInstance().RecalcSelectedWorldPoint();
	}
}

void Form::UpdateBlocking(void)
{
	CheckFormUpdateBlock(IsBlockingUpdatePending);

	InputSelection Selection = InputManager::GetInstance().GetSelection();
	if (!Selection.HaveBone()) {

		aegSetEnabled(CONSTRAIN_POSITION, false);
		aegSetEnabled(X_POS,  false);
		aegSetEnabled(Y_POS,  false);
		aegSetEnabled(Z_POS,  false);
		aegSetEnabled(X_AXIS, false);
		aegSetEnabled(Y_AXIS, false);
		aegSetEnabled(Z_AXIS, false);

		aegSetChecked(X_POS,  true);
		aegSetChecked(Y_POS,  true);
		aegSetChecked(Z_POS,  true);
		aegSetChecked(X_AXIS, true);
		aegSetChecked(Y_AXIS, true);
		aegSetChecked(Z_AXIS, true);

		return;
	}

	aegSetEnabled(CONSTRAIN_POSITION, true);

	BlockingInfo Blocking = AnimationManager::GetInstance().GetBoneBlocking(Selection.Bone);
	vec3 Angles = PhysicsManager::GetInstance().GetBoneAngles(Selection.Bone);

	aegSetEnabled(X_POS, true);
	aegSetEnabled(Y_POS, true);
	aegSetEnabled(Z_POS, true);

	aegSetChecked(X_POS, Blocking.XPos);
	aegSetChecked(Y_POS, Blocking.YPos);
	aegSetChecked(Z_POS, Blocking.ZPos);

	if (!isnan(Angles.x)) {
		aegSetEnabled(X_AXIS, true);
		aegSetChecked(X_AXIS, Blocking.XAxis);
	}
	else {
		aegSetEnabled(X_AXIS, false);
		aegSetChecked(X_AXIS, false);
	}

	if (!isnan(Angles.y)) {
		aegSetEnabled(Y_AXIS, true);
		aegSetChecked(Y_AXIS, Blocking.YAxis);
	}
	else {
		aegSetEnabled(Y_AXIS, false);
		aegSetChecked(Y_AXIS, false);
	}

	if (!isnan(Angles.z)) {
		aegSetEnabled(Z_AXIS, true);
		aegSetChecked(Z_AXIS, Blocking.ZAxis);
	}
	else {
		aegSetEnabled(Z_AXIS, false);
		aegSetChecked(Z_AXIS, false);
	}
}

wstring f2ws(float f, int precision) {

	wstringstream StringStream;

	StringStream << fixed << setprecision(precision) << f;

	return StringStream.str();
}

void Form::UpdatePositionAndAngles(void)
{
	CheckFormUpdateBlock(IsPositionsAndAnglesUpdatePending);

	InputSelection Selection = InputManager::GetInstance().GetSelection();
	if (!Selection.HaveBone()) {

		aegSetEnabled(X_POS_INPUT, false);
		aegSetEnabled(Y_POS_INPUT, false);
		aegSetEnabled(Z_POS_INPUT, false);
		aegSetEnabled(X_ANGLE_INPUT, false);
		aegSetEnabled(Y_ANGLE_INPUT, false);
		aegSetEnabled(Z_ANGLE_INPUT, false);
		aegSetEnabled(X_AXIS_BAR, false);
		aegSetEnabled(Y_AXIS_BAR, false);
		aegSetEnabled(Z_AXIS_BAR, false);

		aegSetText(X_POS_INPUT, L"");
		aegSetText(Y_POS_INPUT, L"");
		aegSetText(Z_POS_INPUT, L"");
		aegSetText(X_ANGLE_INPUT, L"");
		aegSetText(Y_ANGLE_INPUT, L"");
		aegSetText(Z_ANGLE_INPUT, L"");

		aegSetPosition(X_AXIS_BAR, 0);
		aegSetPosition(Y_AXIS_BAR, 0);
		aegSetPosition(Z_AXIS_BAR, 0);

		return;
	}

	aegSetEnabled(X_POS_INPUT,   true);
	aegSetEnabled(Y_POS_INPUT,   true);
	aegSetEnabled(Z_POS_INPUT,   true);
	
	aegSetEnabled(Y_ANGLE_INPUT, true);
	aegSetEnabled(Z_ANGLE_INPUT, true);

	vec3 LowLimit = Selection.Bone->LowLimit;
	vec3 HighLimit = Selection.Bone->HighLimit;

	vec3 WorldPoint = (Selection.Bone->WorldTransform * Selection.Bone->MiddleTranslation) * vec4(0, 0, 0, 1); // to cm
	WorldPoint *= 100.0f; // to cm

	vec3 Angles = PhysicsManager::GetInstance().GetBoneAngles(Selection.Bone);

	aegSetText(X_POS_INPUT, f2ws(WorldPoint.x, 3).c_str());
	aegSetText(Y_POS_INPUT, f2ws(WorldPoint.y, 3).c_str());
	aegSetText(Z_POS_INPUT, f2ws(WorldPoint.z, 3).c_str());

	float LimitLen;

	LimitLen = HighLimit.x - LowLimit.x;
	if (!isnan(Angles.x) && LimitLen > 0) {

		aegSetEnabled(X_ANGLE_INPUT, true);
		aegSetText(X_ANGLE_INPUT, f2ws(degrees(Angles.x), 2).c_str());		
		aegSetEnabled(X_AXIS_BAR, true);
		aegSetPosition(X_AXIS_BAR, (Angles.x - LowLimit.x) / LimitLen);
	}
	else {
		aegSetEnabled(X_ANGLE_INPUT, false);
		aegSetText(X_ANGLE_INPUT, L"");
		aegSetEnabled(X_AXIS_BAR, false);
		aegSetPosition(X_AXIS_BAR, 0);
	}
	
	LimitLen = HighLimit.y - LowLimit.y;
	if (!isnan(Angles.y) && LimitLen > 0) {
	
		aegSetEnabled(Y_ANGLE_INPUT, true);
		aegSetText(Y_ANGLE_INPUT, f2ws(degrees(Angles.y), 2).c_str());
		aegSetEnabled(Y_AXIS_BAR, true);
		aegSetPosition(Y_AXIS_BAR, (Angles.y - LowLimit.y) / LimitLen);
	}
	else {
		aegSetEnabled(Y_ANGLE_INPUT, false);
		aegSetText(Y_ANGLE_INPUT, L"");
		aegSetEnabled(Y_AXIS_BAR, false);
		aegSetPosition(Y_AXIS_BAR, 0);
	}

	LimitLen = HighLimit.z - LowLimit.z;
	if (!isnan(Angles.z) && LimitLen > 0) {
		
		aegSetEnabled(Z_ANGLE_INPUT, true);
		aegSetText(Z_ANGLE_INPUT, f2ws(degrees(Angles.z), 2).c_str());
		aegSetEnabled(Z_AXIS_BAR, true);
		aegSetPosition(Z_AXIS_BAR, (Angles.z - LowLimit.z) / LimitLen);
	}
	else {
		aegSetEnabled(Z_ANGLE_INPUT, false);
		aegSetText(Z_ANGLE_INPUT, L"");
		aegSetEnabled(Z_AXIS_BAR, false);
		aegSetPosition(Z_AXIS_BAR, 0);
	}
}

void Form::ProcessPendingUpdates(void)
{
	if (IsBlockingUpdatePending) {
		IsBlockingUpdatePending = false;
		UpdateBlocking();
	}
	if (IsPositionsAndAnglesUpdatePending) {
		IsPositionsAndAnglesUpdatePending = false;
		UpdatePositionAndAngles();
	}
}