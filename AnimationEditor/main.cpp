#include "Form.hpp"

#define GLEW_STATIC
// Windows Header Files:
#include <windows.h>
#include <WindowsX.h>

#include <string>
#include <iostream>
#include <vector>

#include <GL/glew.h>

#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glu.h>			/* OpenGL utilities header file */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"
#include "ExternalGUI.hpp"

void OpenConsole(void) {

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
}

bool ProcessMessages(void) {

	MSG Msg;

	// messages
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

		if (Msg.message == WM_QUIT)
			return false;

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return true;
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OpenConsole();

	InitTime();

	if (!SetupExternalGUI())
		return -1;

	aegInitialize();
	aegSetIdleCallback(Idle);

	HWND Window = Form::GetInstance().CreateMainWindow(hInstance);
	aegSetOpenGLWindow(Window);

	LastTick = GetTime();
	aegRun();

	return 0;
}