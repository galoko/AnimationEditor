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

#define LoadApi(x) TryGetProcAddress(Handle, #x, (void**)&x)

bool SetupExternalGUI(void) {

	HMODULE Handle = LoadLibrary(L"AnimationEditorGUI.dll");
	if (Handle == 0)
		return false;

	if 
		(
		!LoadApi(aegInitialize) ||
		!LoadApi(aegSetIdleCallback) ||
		!LoadApi(aegSetOpenGLWindow) ||
		!LoadApi(aegSetButtonCallback) ||
		!LoadApi(aegRun)
		)
		return false;

	return true;
}