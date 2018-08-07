#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>

#include "ExternalGUI.hpp"
#include "CharacterManager.hpp"
#include "Form.hpp"
#include "Render.hpp"
#include "InputManager.hpp"
#include "PhysicsManager.hpp"
#include "PoseManager.hpp"

void OpenConsole(void) {

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
}

static LARGE_INTEGER Freq;

void InitTime(void) {
	QueryPerformanceFrequency(&Freq);
}

LONGLONG GetTime(void) {

	LARGE_INTEGER Result;

	QueryPerformanceCounter(&Result);

	return Result.QuadPart;
}

double TimeToSeconds(LONGLONG Time) {

	return (double)Time / (double)Freq.QuadPart;
}

static LONGLONG LastTick;

void __stdcall Idle(void) {

	// time calculation
	LONGLONG Now = GetTime();
	double dt = TimeToSeconds(Now - LastTick);
	LastTick = Now;

	// actual draw call
	Form::GetInstance().Tick(dt);
}

wstring GetWorkingDirectory(void) {

	TCHAR NPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, NPath);

	return NPath;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	wstring WorkingDirectory = GetWorkingDirectory();

	OpenConsole();

	InitTime();

	if (!SetupExternalGUI())
		return -1;

	aegInitialize();
	aegSetIdleCallback(Idle);

	CharacterManager::GetInstance().Initialize();

	InputManager::GetInstance().Initialize();
	HWND WindowHandle = Form::GetInstance().Initialize(hInstance);

	InputManager::GetInstance().SetWindow(WindowHandle);

	Render::GetInstance().Initialize(WindowHandle);
	PhysicsManager::GetInstance().Initialize();
	PoseManager::GetInstance().Initialize();

	SerializationManager::GetInstance().Initialize(WorkingDirectory);

	LastTick = GetTime();
	aegRun();

	aegFinalize();

	UnloadExternalGUI();

	return 0;
}