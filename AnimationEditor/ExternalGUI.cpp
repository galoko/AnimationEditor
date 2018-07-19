#include <windows.h>

#include "ExternalGUI.hpp"

bool TryGetProcAddress(HMODULE Handle, LPCSTR Name, void** Address) {

	*Address = GetProcAddress(Handle, Name);
	return *Address != nullptr;
}

void(__stdcall *aegInitialize)(void);
void(__stdcall *aegSetIdleCallback)(IdleCallback Callback);
void(__stdcall *aegSetOpenGLWindow)(HWND Window);
void(__stdcall *aegSetButtonCallback)(const wchar_t* Name, ButtonCallback Callback);
void(__stdcall *aegRun)(void);

bool SetupExternalGUI(void) {

	HMODULE Handle = LoadLibrary(L"AnimationEditorGUI.dll");
	if (Handle == 0)
		return false;

	if (
		!TryGetProcAddress(Handle, "aegInitialize", (void**)&aegInitialize) ||
		!TryGetProcAddress(Handle, "aegSetIdleCallback", (void**)&aegSetIdleCallback) ||
		!TryGetProcAddress(Handle, "aegSetOpenGLWindow", (void**)&aegSetOpenGLWindow) ||
		!TryGetProcAddress(Handle, "aegSetButtonCallback", (void**)&aegSetButtonCallback) ||
		!TryGetProcAddress(Handle, "aegRun", (void**)&aegRun)
		)
		return false;

	return true;
}