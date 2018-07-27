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

		aegSetEnabled(X_POS_INPUT,   false);
		aegSetEnabled(Y_POS_INPUT,   false);
		aegSetEnabled(Z_POS_INPUT,   false);
		aegSetEnabled(X_ANGLE_INPUT, false);
		aegSetEnabled(Y_ANGLE_INPUT, false);
		aegSetEnabled(Z_ANGLE_INPUT, false);

		return;
	}

	aegSetEnabled(CONSTRAIN_POSITION, true);

	aegSetEnabled(X_POS,  true);
	aegSetEnabled(Y_POS,  true);
	aegSetEnabled(Z_POS,  true);
	aegSetEnabled(X_AXIS, true);
	aegSetEnabled(Y_AXIS, true);
	aegSetEnabled(Z_AXIS, true);

	aegSetEnabled(X_POS_INPUT,   true);
	aegSetEnabled(Y_POS_INPUT,   true);
	aegSetEnabled(Z_POS_INPUT,   true);
	aegSetEnabled(X_ANGLE_INPUT, true);
	aegSetEnabled(Y_ANGLE_INPUT, true);
	aegSetEnabled(Z_ANGLE_INPUT, true);

	BlockingInfo Blocking = AnimationManager::GetInstance().GetBoneBlocking(Selection.Bone);

	aegSetChecked(X_POS,  Blocking.XPos);
	aegSetChecked(Y_POS,  Blocking.YPos);
	aegSetChecked(Z_POS,  Blocking.ZPos);
	aegSetChecked(X_AXIS, Blocking.XAxis);
	aegSetChecked(Y_AXIS, Blocking.YAxis);
	aegSetChecked(Z_AXIS, Blocking.ZAxis);
}

void Form::UpdatePositionAndAngles(void)
{
	InputSelection Selection = InputManager::GetInstance().GetSelection();
	if (!Selection.HaveBone()) {

		aegSetChecked(X_POS,  true);
		aegSetChecked(Y_POS,  true);
		aegSetChecked(Z_POS,  true);
		aegSetChecked(X_AXIS, true);
		aegSetChecked(Y_AXIS, true);
		aegSetChecked(Z_AXIS, true);

		aegSetText(X_POS_INPUT,   L"");
		aegSetText(Y_POS_INPUT,   L"");
		aegSetText(Z_POS_INPUT,   L"");
		aegSetText(X_ANGLE_INPUT, L"");
		aegSetText(Y_ANGLE_INPUT, L"");
		aegSetText(Z_ANGLE_INPUT, L"");

		return;
	}

	vec3 WorldPoint = (Selection.Bone->WorldTransform * Selection.Bone->MiddleTranslation) * vec4(0, 0, 0, 1); // to cm
	WorldPoint *= 100.0f;

	wstringstream StringStream;

	StringStream.str(L"");
	StringStream << fixed << setprecision(3) << WorldPoint.x;
	wstring XPos = StringStream.str();

	StringStream.str(L"");
	StringStream << fixed << setprecision(3) << WorldPoint.y;
	wstring YPos = StringStream.str();

	StringStream.str(L"");
	StringStream << fixed << setprecision(3) << WorldPoint.z;
	wstring ZPos = StringStream.str();

	quat Q = quat_cast(Selection.Bone->Rotation);
	vec3 Angles = eulerAngles(Q);

	StringStream.str(L"");
	StringStream << fixed << setprecision(2) << degrees(Angles.x);
	wstring XAngle = StringStream.str();

	StringStream.str(L"");
	StringStream << fixed << setprecision(2) << degrees(Angles.y);
	wstring YAngle = StringStream.str();

	StringStream.str(L"");
	StringStream << fixed << setprecision(2) << degrees(Angles.z);
	wstring ZAngle = StringStream.str();

	aegSetText(X_POS_INPUT, XPos.c_str());
	aegSetText(Y_POS_INPUT, YPos.c_str());
	aegSetText(Z_POS_INPUT, ZPos.c_str());

	aegSetText(X_ANGLE_INPUT, XAngle.c_str());
	aegSetText(Y_ANGLE_INPUT, YAngle.c_str());
	aegSetText(Z_ANGLE_INPUT, ZAngle.c_str());

}

void Form::ProcessPendingUpdates(void)
{
	if (IsBlockingUpdatePending) {
		IsBlockingUpdatePending = false;
		UpdateBlocking();
	}
}